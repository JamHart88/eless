/*
 * Copyright (C) 1984-2015  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Low level character input from the input file.
 * We use these special purpose routines which optimize moving
 * both forward and backward from the current read pointer.
 */

#include "ch.hpp"

#include "filename.hpp"
#include "forwback.hpp"
#include "ifile.hpp"
#include "less.hpp"
#include "os.hpp"
#include "output.hpp"
#include "prompt.hpp"
#include "screen.hpp"

#if HAVE_STAT_INO
#include <sys/stat.h>
extern dev_t curr_dev;
extern ino_t curr_ino;
#endif

typedef position_t blocknum_t;

int ignore_eoi;

/*
 * Pool of buffers holding the most recently used blocks of the input file.
 * The buffer pool is kept as a doubly-linked circular list,
 * in order from most- to least-recently used.
 * The circular list is anchored by the file state "thisfile".
 */
struct bufnode {
    struct bufnode *next, *prev;
    struct bufnode *hnext, *hprev;
};

const int LBUFSIZE = 8192;

struct buf {
    struct bufnode node;
    blocknum_t block;
    unsigned int datasize;
    unsigned char data[LBUFSIZE];
};
#define bufnode_buf(bn) ((struct buf*)(bn))

/*
 * The file state is maintained in a filestate structure.
 * A pointer to the filestate is kept in the ifile structure.
 */
const int BUFHASH_SIZE = 1024;

struct filestate {
    struct bufnode buflist;
    struct bufnode hashtbl[BUFHASH_SIZE];
    int file;
    int flags;
    position_t fpos;
    int nbufs;
    blocknum_t block;
    unsigned int offset;
    position_t fsize;
};

// thisfile->buflist.next is the HEAD of the chain
// thisfile->buflist.prev is the TAIL of the chain
#define END_OF_CHAIN (&thisfile->buflist)
#define END_OF_HCHAIN(h) (&thisfile->hashtbl[h])
#define BUFHASH(blk) ((blk) & (BUFHASH_SIZE - 1))

/*
 * Macros to manipulate the list of buffers in thisfile->buflist.
 */
#define FOR_BUFS(bn) \
    for ((bn) = thisfile->buflist.next; (bn) != END_OF_CHAIN; (bn) = (bn)->next)

#define BUF_RM(bn)                 \
    (bn)->next->prev = (bn)->prev; \
    (bn)->prev->next = (bn)->next;

#define BUF_INS_HEAD(bn)       \
    (bn)->next = thisfile->buflist.next;   \
    (bn)->prev = END_OF_CHAIN; \
    thisfile->buflist.next->prev = (bn);   \
    thisfile->buflist.next = (bn);

#define BUF_INS_TAIL(bn)       \
    (bn)->next = END_OF_CHAIN; \
    (bn)->prev = thisfile->buflist.prev;   \
    thisfile->buflist.prev->next = (bn);   \
    thisfile->buflist.prev = (bn);

/*
 * Macros to manipulate the list of buffers in thisfile->hashtbl[n].
 */
#define FOR_BUFS_IN_CHAIN(h, bn)                                      \
    for ((bn) = thisfile->hashtbl[h].hnext; (bn) != END_OF_HCHAIN(h); \
         (bn) = (bn)->hnext)

#define BUF_HASH_RM(bn)               \
    (bn)->hnext->hprev = (bn)->hprev; \
    (bn)->hprev->hnext = (bn)->hnext;

#define BUF_HASH_INS(bn, h)                   \
    (bn)->hnext = thisfile->hashtbl[h].hnext; \
    (bn)->hprev = END_OF_HCHAIN(h);           \
    thisfile->hashtbl[h].hnext->hprev = (bn); \
    thisfile->hashtbl[h].hnext = (bn);

static struct filestate* thisfile;
static int ch_ungotchar = -1;
static int maxbufs = -1;

extern int autobuf;
extern int sigs;
extern int follow_mode;
extern char helpdata[];
extern int size_helpdata;
#if LOGFILE
extern int logfile;
extern char* namelogfile;
#endif

static int ch_addbuf();

/*
 * Get the character pointed to by the read pointer.
 */
int ch_get()
{
    struct buf* bp;
    struct bufnode* bn;
    int n;
    bool slept = false;
    int h;
    position_t pos;
    position_t len;

    if (thisfile == nullptr)
        return (EOI);

    /*
     * Quick check for the common case where
     * the desired char is in the head buffer.
     */
    if (thisfile->buflist.next != END_OF_CHAIN) {
        bp = bufnode_buf(thisfile->buflist.next);
        if (thisfile->block == bp->block && thisfile->offset < bp->datasize)
            return bp->data[thisfile->offset];
    }

    /*
     * Look for a buffer holding the desired block.
     */
    h = BUFHASH(thisfile->block);
    FOR_BUFS_IN_CHAIN(h, bn)
    {
        bp = bufnode_buf(bn);
        if (bp->block == thisfile->block) {
            if (thisfile->offset >= bp->datasize)
                /*
                 * Need more data in this buffer.
                 */
                break;
            goto found;
        }
    }
    if (bn == END_OF_HCHAIN(h)) {
        /*
         * Block is not in a buffer.
         * Take the least recently used buffer
         * and read the desired block into it.
         * If the LRU buffer has data in it,
         * then maybe allocate a new buffer.
         */
        if (thisfile->buflist.prev == END_OF_CHAIN || bufnode_buf(thisfile->buflist.prev)->block != -1) {
            /*
             * There is no empty buffer to use.
             * Allocate a new buffer if:
             * 1. We can't seek on this file and -b is not in effect; or
             * 2. We haven't allocated the max buffers for this file yet.
             */
            if ((autobuf && !(thisfile->flags & CH_CANSEEK)) || (maxbufs < 0 || thisfile->nbufs < maxbufs))
                if (ch_addbuf())
                    /*
                     * Allocation failed: turn off autobuf.
                     */
                    autobuf = OPT_OFF;
        }
        bn = thisfile->buflist.prev;
        bp = bufnode_buf(bn);
        BUF_HASH_RM(bn); /* Remove from old hash chain. */
        bp->block = thisfile->block;
        bp->datasize = 0;
        BUF_HASH_INS(bn, h); /* Insert into new hash chain. */
    }

read_more:
    pos = (thisfile->block * LBUFSIZE) + bp->datasize; // NOLINT(clang-analyzer-core.NullDereference)
    if ((len = ch_length()) != NULL_POSITION && pos >= len)
        /*
         * At end of file.
         */
        return (EOI);

    if (pos != thisfile->fpos) {
        /*
         * Not at the correct position: must seek.
         * If input is a pipe, we're in trouble (can't seek on a pipe).
         * Some data has been lost: just return "?".
         */
        if (!(thisfile->flags & CH_CANSEEK))
            return ('?');
        if (lseek(thisfile->file, (off_t)pos, SEEK_SET) == BAD_LSEEK) {
            error((char*)"seek error", NULL_PARG);
            clear_eol();
            return (EOI);
        }
        thisfile->fpos = pos;
    }

    /*
     * Read the block.
     * If we read less than a full block, that's ok.
     * We use partial block and pick up the rest next time.
     */
    if (ch_ungotchar != -1) {
        bp->data[bp->datasize] = ch_ungotchar;
        n = 1;
        ch_ungotchar = -1;
    } else if (thisfile->flags & CH_HELPFILE) {
        bp->data[bp->datasize] = helpdata[thisfile->fpos];
        n = 1;
    } else {
        n = iread(thisfile->file, &bp->data[bp->datasize],
            (unsigned int)(LBUFSIZE - bp->datasize));
    }

    if (n == READ_INTR)
        return (EOI);
    if (n < 0) {
        {
            error((char*)"read error", NULL_PARG);
            clear_eol();
        }
        n = 0;
    }

#if LOGFILE
    /*
     * If we have a log file, write the new data to it.
     */
    if (logfile >= 0 && n > 0)
        ignore_result(write(logfile, (char*)&bp->data[bp->datasize], n));
#endif

    thisfile->fpos += n;
    bp->datasize += n;

    /*
     * If we have read to end of file, set thisfile->fsize to indicate
     * the position of the end of file.
     */
    if (n == 0) {
        thisfile->fsize = pos;
        if (ignore_eoi) {
            /*
             * We are ignoring EOF.
             * Wait a while, then try again.
             */
            if (!slept) {
                parg_t parg;
                parg.p_string = wait_message();
                ierror((char*)"%s", parg);
            }

            sleep(1);
            slept = true;

#if HAVE_STAT_INO
            if (follow_mode == FOLLOW_NAME) {
                /* See whether the file's i-number has changed,
                 * or the file has shrunk.
                 * If so, force the file to be closed and
                 * reopened. */
                struct stat st;
                position_t curr_pos = ch_tell();
                int r = stat(ifile::getCurrentIfile()->getFilename(), &st);
                if (r == 0 && (st.st_ino != curr_ino || st.st_dev != curr_dev || (curr_pos != NULL_POSITION && st.st_size < curr_pos))) {
                    /* remake_display and reopen the file. */
                    screen_trashed = TRASHED_AND_REOPEN_FILE;
                    return (EOI);
                }
            }
#endif
        }
        if (sigs)
            return (EOI);
    }

found:
    if (thisfile->buflist.next != bn) {
        /*
         * Move the buffer to the head of the buffer chain.
         * This orders the buffer chain, most- to least-recently used.
         */
        BUF_RM(bn);
        BUF_INS_HEAD(bn);

        /*
         * Move to head of hash chain too.
         */
        BUF_HASH_RM(bn);
        BUF_HASH_INS(bn, h);
    }

    if (thisfile->offset >= bp->datasize)
        /*
         * After all that, we still don't have enough data.
         * Go back and try again.
         */
        goto read_more;

    return (bp->data[thisfile->offset]);
}

/*
 * ch_ungetchar is a rather kludgy and limited way to push
 * a single char onto an input file descriptor.
 */

void ch_ungetchar(int c)
{
    if (c != -1 && ch_ungotchar != -1)
        error((char*)"ch_ungetchar overrun", NULL_PARG);
    ch_ungotchar = c;
}

#if LOGFILE
/*
 * Close the logfile.
 * If we haven't read all of standard input into it, do that now.
 */

void end_logfile()
{
    static bool tried = false;

    if (logfile < 0)
        return;
    if (!tried && thisfile->fsize == NULL_POSITION) {
        tried = true;
        ierror((char*)"Finishing logfile", NULL_PARG);
        while (ch_forw_get() != EOI)
            if (is_abort_signal(sigs))
                break;
    }
    close(logfile);
    logfile = -1;
    namelogfile = nullptr;
}

/*
 * Start a log file AFTER less has already been running.
 * Invoked from the - command; see toggle_option().
 * Write all the existing buffered data to the log file.
 */

void sync_logfile()
{
    struct buf* bp;
    struct bufnode* bn;
    bool warned = false;
    blocknum_t block;
    blocknum_t nblocks;

    nblocks = (thisfile->fpos + LBUFSIZE - 1) / LBUFSIZE;
    for (block = 0; block < nblocks; block++) {
        bool wrote = false;
        FOR_BUFS(bn)
        {
            bp = bufnode_buf(bn);
            if (bp->block == block) {
                ignore_result(write(logfile, (char*)bp->data, bp->datasize));
                wrote = true;
                break;
            }
        }
        if (!wrote && !warned) {
            error((char*)"Warning: log file is incomplete", NULL_PARG);
            warned = true;
        }
    }
}

#endif

/*
 * Determine if a specific block is currently in one of the buffers.
 */
static bool buffered(blocknum_t block)
{
    struct buf* bp;
    struct bufnode* bn;
    int h;

    h = BUFHASH(block);
    FOR_BUFS_IN_CHAIN(h, bn)
    {
        bp = bufnode_buf(bn);
        if (bp->block == block)
            return (true);
    }
    return (false);
}

/*
 * Seek to a specified position in the file.
 * Return 0 if successful, non-zero if can't seek there.
 */

int ch_seek(position_t pos)
{
    blocknum_t new_block;
    position_t len;

    if (thisfile == nullptr)
        return (0);

    len = ch_length();
    if (pos < ch_zero || (len != NULL_POSITION && pos > len))
        return (1);

    new_block = pos / LBUFSIZE;
    if (!(thisfile->flags & CH_CANSEEK) && pos != thisfile->fpos && !buffered(new_block)) {
        if (thisfile->fpos > pos)
            return (1);
        while (thisfile->fpos < pos) {
            if (ch_forw_get() == EOI)
                return (1);
            if (is_abort_signal(sigs))
                return (1);
        }
        return (0);
    }
    /*
     * Set read pointer.
     */
    thisfile->block = new_block;
    thisfile->offset = pos % LBUFSIZE;
    return (0);
}

/*
 * Seek to the end of the file.
 */

int ch_end_seek()
{
    position_t len;

    if (thisfile == nullptr)
        return (0);

    if (thisfile->flags & CH_CANSEEK)
        thisfile->fsize = filesize(thisfile->file);

    len = ch_length();
    if (len != NULL_POSITION)
        return (ch_seek(len));

    /*
     * Do it the slow way: read till end of data.
     */
    while (ch_forw_get() != EOI)
        if (is_abort_signal(sigs))
            return (1);
    return (0);
}

/*
 * Seek to the last position in the file that is currently buffered.
 */

int ch_end_buffer_seek()
{
    struct buf* bp;
    struct bufnode* bn;
    position_t buf_pos;
    position_t end_pos;

    if (thisfile == nullptr || (thisfile->flags & CH_CANSEEK))
        return (ch_end_seek());

    end_pos = 0;
    FOR_BUFS(bn)
    {
        bp = bufnode_buf(bn);
        buf_pos = (bp->block * LBUFSIZE) + bp->datasize;
        if (buf_pos > end_pos)
            end_pos = buf_pos;
    }

    return (ch_seek(end_pos));
}

/*
 * Seek to the beginning of the file, or as close to it as we can get.
 * We may not be able to seek there if input is a pipe and the
 * beginning of the pipe is no longer buffered.
 */

int ch_beg_seek()
{
    struct bufnode* bn;
    struct bufnode* firstbn;

    /*
     * Try a plain ch_seek first.
     */
    if (ch_seek(ch_zero) == 0)
        return (0);

    /*
     * Can't get to position 0.
     * Look thru the buffers for the one closest to position 0.
     */
    firstbn = thisfile->buflist.next;
    if (firstbn == END_OF_CHAIN)
        return (1);
    FOR_BUFS(bn)
    {
        if (bufnode_buf(bn)->block < bufnode_buf(firstbn)->block)
            firstbn = bn;
    }
    thisfile->block = bufnode_buf(firstbn)->block;
    thisfile->offset = 0;
    return (0);
}

/*
 * Return the length of the file, if known.
 */
position_t ch_length()
{
    if (thisfile == nullptr)
        return (NULL_POSITION);
    if (ignore_eoi)
        return (NULL_POSITION);
    if (thisfile->flags & CH_HELPFILE)
        return (size_helpdata);
    if (thisfile->flags & CH_NODATA)
        return (0);
    return (thisfile->fsize);
}

/*
 * Return the current position in the file.
 */
position_t ch_tell()
{
    if (thisfile == nullptr)
        return (NULL_POSITION);
    return (thisfile->block * LBUFSIZE) + thisfile->offset;
}

/*
 * Get the current char and post-increment the read pointer.
 */

int ch_forw_get()
{
    int c;

    if (thisfile == nullptr)
        return (EOI);
    c = ch_get();
    if (c == EOI)
        return (EOI);
    if (thisfile->offset < LBUFSIZE - 1)
        thisfile->offset++;
    else {
        thisfile->block++;
        thisfile->offset = 0;
    }
    return (c);
}

/*
 * Pre-decrement the read pointer and get the new current char.
 */

int ch_back_get()
{
    if (thisfile == nullptr)
        return (EOI);
    if (thisfile->offset > 0)
        thisfile->offset--;
    else {
        if (thisfile->block <= 0)
            return (EOI);
        if (!(thisfile->flags & CH_CANSEEK) && !buffered(thisfile->block - 1))
            return (EOI);
        thisfile->block--;
        thisfile->offset = LBUFSIZE - 1;
    }
    return (ch_get());
}

/*
 * Set max amount of buffer space.
 * bufspace is in units of 1024 bytes.  -1 mean no limit.
 */

void ch_setbufspace(int bufspace)
{
    if (bufspace < 0)
        maxbufs = -1;
    else {
        maxbufs = ((bufspace * 1024) + LBUFSIZE - 1) / LBUFSIZE;
        if (maxbufs < 1)
            maxbufs = 1;
    }
}

/*
 * Flush (discard) any saved file state, including buffer contents.
 */

void ch_flush()
{
    struct bufnode* bn;

    if (thisfile == nullptr)
        return;

    if (!(thisfile->flags & CH_CANSEEK)) {
        /*
         * If input is a pipe, we don't flush buffer contents,
         * since the contents can't be recovered.
         */
        thisfile->fsize = NULL_POSITION;
        return;
    }

    /*
     * Initialize all the buffers.
     */
    FOR_BUFS(bn) { bufnode_buf(bn)->block = -1; }

    /*
     * Figure out the size of the file, if we can.
     */
    thisfile->fsize = filesize(thisfile->file);

    /*
     * Seek to a known position: the beginning of the file.
     */
    thisfile->fpos = 0;
    thisfile->block = 0; /* thisfile->fpos / LBUFSIZE; */
    thisfile->offset = 0; /* thisfile->fpos % LBUFSIZE; */

#if 1
    /*
     * This is a kludge to workaround a Linux kernel bug: files in
     * /proc have a size of 0 according to fstat() but have readable
     * data.  They are sometimes, but not always, seekable.
     * Force them to be non-seekable here.
     */
    if (thisfile->fsize == 0) {
        thisfile->fsize = NULL_POSITION;
        thisfile->flags &= ~CH_CANSEEK;
    }
#endif

    if (lseek(thisfile->file, (off_t)0, SEEK_SET) == BAD_LSEEK) {
        /*
         * Warning only; even if the seek fails for some reason,
         * there's a good chance we're at the beginning anyway.
         * {{ I think this is bogus reasoning. }}
         */
        error((char*)"seek error to 0", NULL_PARG);
    }
}

/*
 * Allocate a new buffer.
 * The buffer is added to the tail of the buffer chain.
 */
static int ch_addbuf()
{
    struct buf* bp;
    struct bufnode* bn;

    /*
     * Allocate and initialize a new buffer and link it
     * onto the tail of the buffer list.
     */
    bp = (struct buf*)calloc(1, sizeof(struct buf));
    if (bp == nullptr)
        return (1);
    thisfile->nbufs++;
    bp->block = -1;
    bn = &bp->node;

    BUF_INS_TAIL(bn);
    BUF_HASH_INS(bn, 0);
    return (0);
}

/*
 *
 */
static void init_hashtbl()
{
    int h;

    for (h = 0; h < BUFHASH_SIZE; h++) {
        thisfile->hashtbl[h].hnext = END_OF_HCHAIN(h);
        thisfile->hashtbl[h].hprev = END_OF_HCHAIN(h);
    }
}

/*
 * Delete all buffers for this file.
 */
static void ch_delbufs()
{
    struct bufnode* bn;

    while (thisfile->buflist.next != END_OF_CHAIN) {
#ifndef CLANGTIDY
        // Clang-tidy has problems with this code
        bn = thisfile->buflist.next;
        BUF_RM(bn);
        free(bufnode_buf(bn));
#endif
    }
    thisfile->nbufs = 0;
    init_hashtbl();
}

/*
 * Is it possible to seek on a file descriptor?
 */

int seekable(int f) { 
    return (lseek(f, (off_t)1, SEEK_SET) != BAD_LSEEK); 
}

/*
 * Force EOF to be at the current read position.
 * This is used after an ignore_eof read, during which the EOF may change.
 */

void ch_set_eof() { 
    thisfile->fsize = thisfile->fpos; 
}

/*
 * Initialize file state for a new file.
 */

void ch_init(int f, int flags)
{
    /*
     * See if we already have a filestate for this file.
     */
    thisfile = (struct filestate*)ifile::getCurrentIfile()->getFilestate();
    if (thisfile == nullptr) {
        /*
         * Allocate and initialize a new filestate.
         */
        thisfile = (struct filestate*)calloc(1, sizeof(struct filestate));
        thisfile->buflist.next = thisfile->buflist.prev = END_OF_CHAIN;
        thisfile->nbufs = 0;
        thisfile->flags = 0;
        thisfile->fpos = 0;
        thisfile->block = 0;
        thisfile->offset = 0;
        thisfile->file = -1;
        thisfile->fsize = NULL_POSITION;
        thisfile->flags = flags;
        init_hashtbl();
        /*
         * Try to seek; set CH_CANSEEK if it works.
         */
        if ((flags & CH_CANSEEK) && !seekable(f))
            thisfile->flags &= ~CH_CANSEEK;
        ifile::getCurrentIfile()->setFilestate((void*)thisfile);
    }
    if (thisfile->file == -1)
        thisfile->file = f;
    ch_flush();
}

/*
 * Close a filestate.
 */

void ch_close()
{
    bool keepstate = false;

    if (thisfile == nullptr)
        return;

    if (thisfile->flags & (CH_CANSEEK | CH_POPENED | CH_HELPFILE)) {
        /*
         * We can seek or re-open, so we don't need to keep buffers.
         */
        ch_delbufs();
    } else
        keepstate = true;
    if (!(thisfile->flags & CH_KEEPOPEN)) {
        /*
         * We don't need to keep the file descriptor open
         * (because we can re-open it.)
         * But don't really close it if it was opened via popen(),
         * because pclose() wants to close it.
         */
        if (!(thisfile->flags & (CH_POPENED | CH_HELPFILE)))
            close(thisfile->file);
        thisfile->file = -1;
    } else
        keepstate = true;
    if (!keepstate) {
        /*
         * We don't even need to keep the filestate structure.
         */
        free(thisfile);
        thisfile = nullptr;
        ifile::getCurrentIfile()->setFilestate((void*)nullptr);
    }
}

/*
 * Return thisfile->flags for the current file.
 */

int ch_getflags()
{
    if (thisfile == nullptr)
        return (0);
    return (thisfile->flags);
}
