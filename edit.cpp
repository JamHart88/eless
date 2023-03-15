/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

#include "edit.hpp"
#include "ch.hpp"
#include "cmdbuf.hpp"
#include "command.hpp"
#include "filename.hpp"
#include "ifile.hpp"
#include "less.hpp"
#include "linenum.hpp"
#include "mark.hpp"
#include "os.hpp"
#include "output.hpp"
#include "position.hpp"
#include "search.hpp"
#include "utils.hpp"

// Only for debugging
#include "debug.hpp"

#if HAVE_STAT
#include <sys/stat.h>
#endif

// TODO: Move to namespaces
int fd0 = 0;

extern bool new_file;
extern int errmsgs;
extern int cbufs;
extern char* every_first_cmd;
extern bool any_display;
extern int force_open;
extern int is_tty;
extern struct scrpos initial_scrpos;
extern void* ml_examine;

#if HAVE_STAT_INO
dev_t curr_dev;
ino_t curr_ino;
#endif

namespace edit {

/*
 * Textlist functions deal with a list of words separated by spaces.
 * init_textlist sets up a textlist structure.
 * forw_textlist uses that structure to iterate thru the list of
 * words, returning each one as a standard null-terminated string.
 * back_textlist does the same, but runs thru the list backwards.
 */
void init_textlist(struct textlist* tlist, char* str)
{
    char* s;
    int meta_quoted = 0;
    int delim_quoted = 0;
    char* esc = filename::get_meta_escape();
    int esclen = (int)strlen(esc);

    tlist->string = utils::skipsp(str);
    tlist->endstring = tlist->string + strlen(tlist->string);
    for (s = str; s < tlist->endstring; s++) {
        if (meta_quoted) {
            meta_quoted = 0;
        } else if (esclen > 0 && s + esclen < tlist->endstring && strncmp(s, esc, esclen) == 0) {
            meta_quoted = 1;
            s += esclen - 1;
        } else if (delim_quoted) {
            if (*s == less::Settings::closequote)
                delim_quoted = 0;
        } else /* (!delim_quoted) */
        {
            if (*s == less::Settings::openquote)
                delim_quoted = 1;
            else if (*s == ' ')
                *s = '\0';
        }
    }
}

char* forw_textlist(struct textlist* tlist, char* prev)
{
    char* s;

    /*
     * prev == nullptr means return the first word in the list.
     * Otherwise, return the word after "prev".
     */
    if (prev == nullptr)
        s = tlist->string;
    else
        s = prev + strlen(prev);
    if (s >= tlist->endstring)
        return (nullptr);
    while (*s == '\0')
        s++;
    if (s >= tlist->endstring)
        return (nullptr);
    return (s);
}

char* back_textlist(struct textlist* tlist, char* prev)
{
    char* s;

    /*
     * prev == nullptr means return the last word in the list.
     * Otherwise, return the word before "prev".
     */
    if (prev == nullptr)
        s = tlist->endstring;
    else if (prev <= tlist->string)
        return (nullptr);
    else
        s = prev - 1;
    while (*s == '\0')
        s--;
    if (s <= tlist->string)
        return (nullptr);
    while (s[-1] != '\0' && s > tlist->string)
        s--;
    return (s);
}

/*
 * Close a pipe opened via popen.
 */
static void close_pipe(FILE* pipefd)
{
    if (pipefd == nullptr)
        return;
    pclose(pipefd);
}

/*
 * Close the current input file.
 */
static void close_file(void)
{
    struct scrpos scrpos;
    int chflags;
    FILE* altpipe;
    char* altfilename;

    if (ifile::getCurrentIfile() == nullptr)
        return;

    /*
     * Save the current position so that we can return to
     * the same position if we edit this file again.
     */
    get_scrpos(&scrpos, TOP);
    if (scrpos.pos != NULL_POSITION) {
        ifile::getCurrentIfile()->setPos(scrpos);
        lastmark();
    }
    /*
     * Close the file descriptor, unless it is a pipe.
     */
    chflags = ch::getflags();
    ch::close();
    /*
     * If we opened a file using an alternate name,
     * do special stuff to close it.
     */
    altfilename = ifile::getCurrentIfile()->getAltfilename();
    if (altfilename != nullptr) {
        altpipe = (FILE*)ifile::getCurrentIfile()->getAltpipe();
        if (altpipe != nullptr && !(chflags & CH_KEEPOPEN)) {
            close_pipe(altpipe);
            ifile::getCurrentIfile()->setAltpipe(nullptr);
        }
        filename::close_altfile(altfilename, ifile::getCurrentIfile()->getFilename());
        ifile::getCurrentIfile()->setAltfilename(nullptr);
    }
    ifile::setCurrentIfile(nullptr);

#if HAVE_STAT_INO
    curr_ino = curr_dev = 0;
#endif
}

/*
 * Edit a new file (given its name).
 * Filename == "-" means standard input.
 * Filename == nullptr means just close the current file.
 */
int edit(const char* filename)
{
    if (filename == nullptr)
        return (edit_ifile(nullptr));
    return (edit_ifile(ifile::getIfile(filename)));
}

/*
 * Edit a new file (given its IFILE).
 * requestedIfile == nullptr means just close the current file.
 */
int edit_ifile(ifile::Ifile* requestedIfile)
{
    int f;
    int answer;
    int no_display;
    int chflags;
    char* filename;
    char* open_filename;
    char* alt_filename;
    void* altpipe;
    ifile::Ifile* curr_ifile = ifile::getCurrentIfile();
    ifile::Ifile* was_curr_ifile;
    parg_t parg;

    if (requestedIfile == curr_ifile) {
        /*
         * Already have the correct file open.
         */
        return (0);
    }

    /*
     * We must close the currently open file now.
     * This is necessary to make the filename::open_altfile/filename::close_altfile pairs
     * nest properly (or rather to avoid nesting at all).
     * {{ Some stupid implementations of popen() mess up if you do:
     *    fA = popen("A"); fB = popen("B"); pclose(fA); pclose(fB); }}
     */
    ch::end_logfile();

    was_curr_ifile = save_curr_ifile();
    if (curr_ifile != nullptr) {
        chflags = ch::getflags();
        close_file();
        if ((chflags & CH_HELPFILE) && was_curr_ifile->getHoldCount() <= 1) {
            /*
             * Don't keep the help file in the ifile list.
             */
            ifile::deleteIfile(was_curr_ifile);
            was_curr_ifile = ifile::getOldIfile();
        }
    }

    if (requestedIfile == nullptr) {
        /*
         * No new file to open.
         * (Don't set old_ifile, because if you call edit_ifile(nullptr),
         *  you're supposed to have saved ifile::getCurrentIfile() yourself,
         *  and you'll restore it if necessary.)
         */
        unsave_ifile(was_curr_ifile);
        return (0);
    }

    filename = utils::save(requestedIfile->getFilename());

    /*
     * See if LESSOPEN specifies an "alternate" file to open.
     */
    altpipe = requestedIfile->getAltpipe();
    if (altpipe != nullptr) {
        /*
         * File is already open.
         * chflags and f are not used by init if requestedIfile has
         * filestate which should be the case if we're here.
         * Set them here to avoid uninitialized variable warnings.
         */
        chflags = 0;
        f = -1;
        alt_filename = requestedIfile->getAltfilename();
        open_filename = (alt_filename != nullptr) ? alt_filename : filename;

    } else {

        if (strcmp(filename, FAKE_HELPFILE) == 0 || strcmp(filename, FAKE_EMPTYFILE) == 0)
            alt_filename = nullptr;
        else
            alt_filename = filename::open_altfile(filename, &f, &altpipe);

        open_filename = (alt_filename != nullptr) ? alt_filename : filename;

        chflags = 0;
        if (altpipe != nullptr) {
            /*
             * The alternate "file" is actually a pipe.
             * f has already been set to the file descriptor of the pipe
             * in the call to filename::open_altfile above.
             * Keep the file descriptor open because it was opened
             * via popen(), and pclose() wants to close it.
             */
            chflags |= CH_POPENED;
            if (strcmp(filename, "-") == 0)
                chflags |= CH_KEEPOPEN;

        } else if (strcmp(filename, "-") == 0) {
            /*
             * Use standard input.
             * Keep the file descriptor open because we can't reopen it.
             */
            f = fd0;
            chflags |= CH_KEEPOPEN;
            /*
             * Must switch stdin to BINARY mode.
             */
            // SET_BINARY(f); //No longer needed?
        } else if (strcmp(open_filename, FAKE_EMPTYFILE) == 0) {
            f = -1;
            chflags |= CH_NODATA;

        } else if (strcmp(open_filename, FAKE_HELPFILE) == 0) {
            f = -1;
            chflags |= CH_HELPFILE;

        } else if ((parg.p_string = filename::bad_file(open_filename)) != nullptr) {
            /*
             * It looks like a bad file.  Don't try to open it.
             */
            error((char*)"%s", parg);
            free(parg.p_string);

        err1:
            if (alt_filename != nullptr) {
                close_pipe((FILE*)altpipe);
                filename::close_altfile(alt_filename, filename);
            }
            ifile::deleteIfile(requestedIfile);
            free(filename);
            /*
             * Re-open the current file.
             */
            if (was_curr_ifile == requestedIfile) {
                /*
                 * Whoops.  The "current" ifile is the one we just deleted.
                 * Just give up.
                 */
                utils::quit(QUIT_ERROR);
            }
            reedit_ifile(was_curr_ifile);
            return (1);

        } else if ((f = open(open_filename, OPEN_READ)) < 0) {
            /*
             * Got an error trying to open it.
             */
            parg.p_string = errno_message(filename);
            error((char*)"%s", parg);
            free(parg.p_string);
            goto err1;

        } else {

            chflags |= CH_CANSEEK;
            if (!force_open && !requestedIfile->getOpened() && filename::bin_file(f)) {
                /*
                 * Looks like a binary file.
                 * Ask user if we should proceed.
                 */
                parg.p_string = filename;
                answer = query((char*)"\"%s\" may be a binary file.  See it anyway? ",
                    parg);
                if (answer != 'y' && answer != 'Y') {
                    close(f);
                    goto err1;
                }
            }
        }
    }

    /*
     * Get the new ifile.
     * Get the saved position for the file.
     */
    if (was_curr_ifile != nullptr) {
        ifile::setOldIfile(was_curr_ifile);
        unsave_ifile(was_curr_ifile);
    }
    setCurrentIfile(requestedIfile);
    requestedIfile->setAltfilename(alt_filename);
    requestedIfile->setAltpipe(altpipe);

    ifile::getCurrentIfile()->setOpened(true); /* File has been opened */

    initial_scrpos = ifile::getCurrentIfile()->getPos();

    new_file = true;
    ch::init(f, chflags);

    if (!(chflags & CH_HELPFILE)) {
        if (less::Settings::namelogfile != nullptr && is_tty)
            use_logfile(less::Settings::namelogfile);

#if HAVE_STAT_INO
        /* Remember the i-number and device of the opened file. */
        if (strcmp(open_filename, "-") != 0) {
            struct stat statbuf;
            int r = stat(open_filename, &statbuf);
            if (r == 0) {
                curr_ino = statbuf.st_ino;
                curr_dev = statbuf.st_dev;
            }
        }
#endif
        if (every_first_cmd != nullptr) {
            command::ungetcc(CHAR_END_COMMAND);
            command::ungetsc(every_first_cmd);
        }
    }

    no_display = !any_display;
    flush();
    any_display = true;

    if (is_tty) {
        /*
         * Output is to a real tty.
         */

        /*
         * Indicate there is nothing displayed yet.
         */
        pos_clear();
        clr_linenum();
#if HILITE_SEARCH
        clr_hilite();
#endif
        if (strcmp(filename, FAKE_HELPFILE) && strcmp(filename, FAKE_EMPTYFILE)) {
            char* qfilename = filename::shell_quote(filename);
            cmdbuf::cmd_addhist((struct mlist*)ml_examine, qfilename, 1);
            free(qfilename);
        }

        if (no_display && errmsgs > 0) {
            /*
             * We displayed some messages on error output
             * (file descriptor 2; see error() function).
             * Before erasing the screen contents,
             * display the file name and wait for a keystroke.
             */
            parg.p_string = filename;
            error((char*)"%s", parg);
        }
    }
    free(filename);
    return (0);
}

/*
 * Edit a space-separated list of files.
 * For each filename in the list, enter it into the ifile list.
 * Then edit the first one.
 */
int edit_list(char* filelist)
{
    ifile::Ifile* save_ifile;
    char* good_filename;
    char* filename;
    char* gfilelist;
    char* gfilename;
    char* qfilename;
    struct textlist tl_files;
    struct textlist tl_gfiles;

    save_ifile = save_curr_ifile();
    good_filename = nullptr;

    /*
     * Run thru each filename in the list.
     * Try to glob the filename.
     * If it doesn't expand, just try to open the filename.
     * If it does expand, try to open each name in that list.
     */
    init_textlist(&tl_files, filelist);
    filename = nullptr;

    while ((filename = forw_textlist(&tl_files, filename)) != nullptr) {

        gfilelist = filename::lglob(filename);
        init_textlist(&tl_gfiles, gfilelist);
        gfilename = nullptr;

        while ((gfilename = forw_textlist(&tl_gfiles, gfilename)) != nullptr) {

            qfilename = filename::shell_unquote(gfilename);

            if (edit(qfilename) == 0 && good_filename == nullptr)
                good_filename = ifile::getCurrentIfile()->getFilename();

            free(qfilename);
        }
        free(gfilelist);
    }
    /*
     * Edit the first valid filename in the list.
     */
    if (good_filename == nullptr) {
        unsave_ifile(save_ifile);
        return (1);
    }
    if (ifile::getIfile(good_filename) == ifile::getCurrentIfile()) {
        /*
         * Trying to edit the current file; don't reopen it.
         */
        unsave_ifile(save_ifile);
        return (0);
    }
    reedit_ifile(save_ifile);
    return (edit(good_filename));
}

/*
 * Edit the first file in the command line (ifile) list.
 */
int edit_first(void)
{
    if (ifile::numIfiles() == 0)
        return (edit_stdin());
    ifile::setCurrentIfile(nullptr);
    return (edit_next(1));
}

/*
 * Edit the last file in the command line (ifile) list.
 */
int edit_last(void)
{
    ifile::setCurrentIfile(nullptr);
    return (edit_prev(1));
}

/*
 * Edit the n-th next or previous file in the command line (ifile) list.
 */
static int edit_istep(ifile::Ifile* h, int n, int dir)
{
    ifile::Ifile* next;

    /*
     * Skip n filenames, then try to edit each filename.
     */
    for (;;) {
        next = (dir > 0) ? ifile::nextIfile(h) : ifile::prevIfile(h);
        if (--n < 0) {
            if (edit_ifile(h) == 0)
                break;
        }
        if (next == nullptr) {
            /*
             * Reached end of the ifile list.
             */
            return (1);
        }
        if (is_abort_signal(less::Settings::sigs)) {
            /*
             * Interrupt breaks out, if we're in a long
             * list of files that can't be opened.
             */
            return (1);
        }
        h = next;
    }
    /*
     * Found a file that we can edit.
     */
    return (0);
}

static int edit_inext(ifile::Ifile* h, int n)
{
    return (edit_istep(h, n, +1));
}

int edit_next(int n)
{
    return edit_istep(ifile::getCurrentIfile(), n, +1);
}

static int edit_iprev(ifile::Ifile* h, int n)
{
    return (edit_istep(h, n, -1));
}

int edit_prev(int n)
{
    return edit_istep(ifile::getCurrentIfile(), n, -1);
}

/*
 * Edit a specific file in the command line (ifile) list.
 */
int edit_index(int n)
{
    ifile::Ifile* h = nullptr;

    do {

        if ((h = ifile::nextIfile(h)) == nullptr) {
            /*
             * Reached end of the list without finding it.
             */
            return (1);
        }
    } while (ifile::getIndex(h) != n);

    return (edit_ifile(h));
}

ifile::Ifile* save_curr_ifile(void)
{
    ifile::Ifile* ret = ifile::getCurrentIfile();
    if (ret != nullptr)
        ret->setHold(1);
    return (ret);
}

void unsave_ifile(ifile::Ifile* save_ifile)
{
    if (save_ifile != nullptr)
        save_ifile->setHold(-1);
}

/*
 * Reedit the ifile which was previously open.
 */
void reedit_ifile(ifile::Ifile* save_ifile)
{
    ifile::Ifile* next;
    ifile::Ifile* prev;

    /*
     * Try to reopen the ifile.
     * Note that opening it may fail (maybe the file was removed),
     * in which case the ifile will be deleted from the list.
     * So save the next and prev ifiles first.
     */
    unsave_ifile(save_ifile);
    next = ifile::nextIfile(save_ifile);
    prev = ifile::prevIfile(save_ifile);

    if (edit_ifile(save_ifile) == 0)
        return;
    /*
     * If can't reopen it, open the next input file in the list.
     */
    if (next != nullptr && edit_inext(next, 0) == 0)
        return;
    /*
     * If can't open THAT one, open the previous input file in the list.
     */
    if (prev != nullptr && edit_iprev(prev, 0) == 0)
        return;
    /*
     * If can't even open that, we're stuck.  Just quit.
     */
    utils::quit(QUIT_ERROR);
}

void reopen_curr_ifile(void)
{
    ifile::Ifile* save_ifile = save_curr_ifile();
    close_file();
    reedit_ifile(save_ifile);
}

/*
 * Edit standard input.
 */
int edit_stdin(void)
{
    if (isatty(fd0)) {
        error((char*)"Missing filename (\"less --help\" for help)", NULL_PARG);
        utils::quit(QUIT_OK);
    }
    return (edit((char*)"-"));
}

/*
 * Copy a file directly to standard output.
 * Used if standard output is not a tty.
 */
void cat_file(void)
{
    int c;

    while ((c = ch::forw_get()) != EOI)
        putchr(c);
    flush();
}


/*
 * If the user asked for a log file and our input file
 * is standard input, create the log file.
 * We take care not to blindly overwrite an existing file.
 */
void use_logfile(char* filename)
{
    int exists;
    int answer;
    parg_t parg;

    if (ch::getflags() & CH_CANSEEK)
        /*
         * Can't currently use a log file on a file that can seek.
         */
        return;

    /*
     * {{ We could use access() here. }}
     */
    exists = open(filename, OPEN_READ);
    if (exists >= 0)
        close(exists);
    exists = (exists >= 0);

    /*
     * Decide whether to overwrite the log file or append to it.
     * If it doesn't exist we "overwrite" it.
     */
    if (!exists || less::Settings::force_logfile) {
        /*
         * Overwrite (or create) the log file.
         */
        answer = 'O';
    } else {
        /*
         * Ask user what to do.
         */
        parg.p_string = filename;
        answer = query((char*)"Warning: \"%s\" exists; Overwrite, Append or Don't log? ", parg);
    }

loop:
    switch (answer) {
    case 'O':
    case 'o':
        /*
         * Overwrite: create the file.
         */
        less::Settings::logfile = creat(filename, 0644);
        break;
    case 'A':
    case 'a':
        /*
         * Append: open the file and seek to the end.
         */
        less::Settings::logfile = open(filename, OPEN_APPEND);
        if (lseek(less::Settings::logfile, (off_t)0, SEEK_END) == BAD_LSEEK) {
            close(less::Settings::logfile);
            less::Settings::logfile = -1;
        }
        break;
    case 'D':
    case 'd':
        /*
         * Don't do anything.
         */
        return;
    case 'q':
        utils::quit(QUIT_OK);
        /*NOTREACHED*/
    default:
        /*
         * Eh?
         */
        answer = query((char*)"Overwrite, Append, or Don't log? (Type \"O\", \"A\", \"D\" or \"q\") ", NULL_PARG);
        goto loop;
    }

    if (less::Settings::logfile < 0) {
        /*
         * Error in opening logfile.
         */
        parg.p_string = filename;
        error((char*)"Cannot write to \"%s\"", parg);
        return;
    }
    
}

} // namespace edit