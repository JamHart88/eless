/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */


/*
 * Routines to mess around with filenames (and files).
 * Much of this is very OS dependent.
 */

#include "less.hpp"
#include "charset.hpp"
#include "filename.hpp"
#include "lglob.hpp"
#include "os.hpp"
#include "ch.hpp"
#include "decode.hpp"
#include "ifile.hpp"
#include "line.hpp"
#include "output.hpp"
#include "utils.hpp"

#if HAVE_STAT
#include <sys/stat.h>
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#endif

extern int force_open;
extern int use_lessopen;
extern int ctldisp;
extern int utf_mode;
extern IFILE curr_ifile;
extern IFILE old_ifile;
#if SPACES_IN_FILENAMES
extern char openquote;
extern char closequote;
#endif

/*
 * Remove quotes around a filename.
 */
 char * shell_unquote(char *str)
{
    char *name;
    char *p;

    name = p = (char *) utils::ecalloc(strlen(str)+1, sizeof(char));
    if (*str == openquote)
    {
        str++;
        while (*str != '\0')
        {
            if (*str == closequote)
            {
                if (str[1] != closequote)
                    break;
                str++;
            }
            *p++ = *str++;
        }
    } else
    {
        char *esc = get_meta_escape();
        int esclen = (int) strlen(esc);
        while (*str != '\0')
        {
            if (esclen > 0 && strncmp(str, esc, esclen) == 0)
                str += esclen;
            *p++ = *str++;
        }
    }
    *p = '\0';
    return (name);
}

/*
 * Get the shell's escape character.
 */
 char * get_meta_escape(void)
{
    char *s;

    s = lgetenv((char *)"LESSMETAESCAPE");
    if (s == NULL)
        s = (char *)DEF_METAESCAPE;
    return (s);
}

/*
 * Get the characters which the shell considers to be "metacharacters".
 */
static char * metachars(void)
{
    static char *mchars = NULL;

    if (mchars == NULL)
    {
        mchars = lgetenv((char *)"LESSMETACHARS");
        if (mchars == NULL)
            mchars =(char *) DEF_METACHARS;
    }
    return (mchars);
}

/*
 * Is this a shell metacharacter?
 */
static int metachar(char c)
{
    return (strchr(metachars(), c) != NULL);
}

/*
 * Insert a backslash before each metacharacter in a string.
 */
 char * shell_quote(char *s)
{
    char *p;
    char *newstr;
    int len;
    char *esc = get_meta_escape();
    int esclen = (int) strlen(esc);
    int use_quotes = 0;
    int have_quotes = 0;

    /*
     * Determine how big a string we need to allocate.
     */
    len = 1; /* Trailing null byte */
    for (p = s;  *p != '\0';  p++)
    {
        len++;
        if (*p == openquote || *p == closequote)
            have_quotes = 1;
        if (metachar(*p))
        {
            if (esclen == 0)
            {
                /*
                 * We've got a metachar, but this shell 
                 * doesn't support escape chars.  Use quotes.
                 */
                use_quotes = 1;
            } else
            {
                /*
                 * Allow space for the escape char.
                 */
                len += esclen;
            }
        }
    }
    if (use_quotes)
    {
        if (have_quotes)
            /*
             * We can't quote a string that contains quotes.
             */
            return (NULL);
        len = (int) strlen(s) + 3;
    }
    /*
     * Allocate and construct the new string.
     */
    newstr = p = (char *) utils::ecalloc(len, sizeof(char));
    if (use_quotes)
    {
        SNPRINTF3(newstr, len, "%c%s%c", openquote, s, closequote);
    } else
    {
        while (*s != '\0')
        {
            if (metachar(*s))
            {
                /*
                 * Add the escape char.
                 */
                strcpy(p, esc);
                p += esclen;
            }
            *p++ = *s++;
        }
        *p = '\0';
    }
    return (newstr);
}

/*
 * Return a pathname that points to a specified file in a specified directory.
 * Return NULL if the file does not exist in the directory.
 */
static char * dirfile(char *dirname, char *filename)
{
    char *pathname;
    int len;
    int f;

    if (dirname == NULL || *dirname == '\0')
        return (NULL);
    /*
     * Construct the full pathname.
     */
    len = (int) (strlen(dirname) + strlen(filename) + 2);
    pathname = (char *) calloc(len, sizeof(char));
    if (pathname == NULL)
        return (NULL);
    SNPRINTF3(pathname, len, "%s%s%s", dirname, PATHNAME_SEP, filename);
    /*
     * Make sure the file exists.
     */
    f = open(pathname, OPEN_READ);
    if (f < 0)
    {
        free(pathname);
        pathname = NULL;
    } else
    {
        close(f);
    }
    return (pathname);
}

/*
 * Return the full pathname of the given file in the "home directory".
 */
 char * homefile(char *filename)
{
    char *pathname;

    /*
     * Try $HOME/filename.
     */
    pathname = dirfile(lgetenv((char *)"HOME"), filename);
    if (pathname != NULL)
        return (pathname);
    return (NULL);
}

/*
 * Expand a string, substituting any "%" with the current filename,
 * and any "#" with the previous filename.
 * But a string of N "%"s is just replaced with N-1 "%"s.
 * Likewise for a string of N "#"s.
 * {{ This is a lot of work just to support % and #. }}
 */
 char * fexpand(char *s)
{
    char *fr, *to;
    int n;
    char *e;
    IFILE ifile;

#define fchar_ifile(c)         \
    ((c) == '%' ? curr_ifile : \
     (c) == '#' ? old_ifile : NULL_IFILE)

    /*
     * Make one pass to see how big a buffer we 
     * need to allocate for the expanded string.
     */
    n = 0;
    for (fr = s;  *fr != '\0';  fr++)
    {
        switch (*fr)
        {
        case '%':
        case '#':
            if (fr > s && fr[-1] == *fr)
            {
                /*
                 * Second (or later) char in a string
                 * of identical chars.  Treat as normal.
                 */
                n++;
            } else if (fr[1] != *fr)
            {
                /*
                 * Single char (not repeated).  Treat specially.
                 */
                ifile = fchar_ifile(*fr);
                if (ifile == NULL_IFILE)
                    n++;
                else
                    n += (int) strlen(get_filename(ifile));
            }
            /*
             * Else it is the first char in a string of
             * identical chars.  Just discard it.
             */
            break;
        default:
            n++;
            break;
        }
    }

    e = (char *) utils::ecalloc(n+1, sizeof(char));

    /*
     * Now copy the string, expanding any "%" or "#".
     */
    to = e;
    for (fr = s;  *fr != '\0';  fr++)
    {
        switch (*fr)
        {
        case '%':
        case '#':
            if (fr > s && fr[-1] == *fr)
            {
                *to++ = *fr;
            } else if (fr[1] != *fr)
            {
                ifile = fchar_ifile(*fr);
                if (ifile == NULL_IFILE)
                    *to++ = *fr;
                else
                {
                    strcpy(to, get_filename(ifile));
                    to += strlen(to);
                }
            }
            break;
        default:
            *to++ = *fr;
            break;
        }
    }
    *to = '\0';
    return (e);
}


#if TAB_COMPLETE_FILENAME

/*
 * Return a blank-separated list of filenames which "complete"
 * the given string.
 */
 char * fcomplete(char *s)
{
    char *fpat;
    char *qs;

    /*
     * Complete the filename "s" by globbing "s*".
     */
    {
    int len = (int) strlen(s) + 2;
    fpat = (char *) utils::ecalloc(len, sizeof(char));
    SNPRINTF1(fpat, len, "%s*", s);
    }
    qs = lglob(fpat);
    s = shell_unquote(qs);
    if (strcmp(s,fpat) == 0)
    {
        /*
         * The filename didn't expand.
         */
        free(qs);
        qs = NULL;
    }
    free(s);
    free(fpat);
    return (qs);
}
#endif

/*
 * Try to determine if a file is "binary".
 * This is just a guess, and we need not try too hard to make it accurate.
 */
 int bin_file(int f)
{
    int n;
    int bin_count = 0;
    char data[256];
    char* p;
    char* edata;

    if (!seekable(f))
        return (0);
    if (lseek(f, (off_t)0, SEEK_SET) == BAD_LSEEK)
        return (0);
    n = read(f, data, sizeof(data));
    if (n <= 0)
        return (0);
    edata = &data[n];
    for (p = data;  p < edata;  )
    {
        if (utf_mode && !is_utf8_well_formed(p, edata-data))
        {
            bin_count++;
            utf_skip_to_lead(&p, edata);
        } else 
        {
            lwchar_t c = step_char(&p, +1, edata);
            if (ctldisp == OPT_ONPLUS && IS_CSI_START(c))
                skip_ansi(&p, edata);
            else if (binary_char(c))
                bin_count++;
        }
    }
    /*
     * Call it a binary file if there are more than 5 binary characters
     * in the first 256 bytes of the file.
     */
    return (bin_count > 5);
}

/*
 * Try to determine the size of a file by seeking to the end.
 */
static position_t seek_filesize(int f)
{
    off_t spos;

    spos = lseek(f, (off_t)0, SEEK_END);
    if (spos == BAD_LSEEK)
        return (NULL_POSITION);
    return ((position_t) spos);
}

/*
 * Read a string from a file.
 * Return a pointer to the string in memory.
 */
static char * readfd(FILE *fd)
{
    D("readfd");
    int len;
    int ch;
    char *buf;
    char *p;
    
    /* 
     * Make a guess about how many chars in the string
     * and allocate a buffer to hold it.
     */
    len = 100;
    buf = (char *) utils::ecalloc(len, sizeof(char));
    for (p = buf;  ;  p++)
    {
        if ((ch = getc(fd)) == '\n' || ch == EOF)
            break;
        if (p - buf >= len-1)
        {
            debug("TOOBIG");
            /*
             * The string is too big to fit in the buffer we have.
             * Allocate a new buffer, twice as big.
             */
            len *= 2;
            *p = '\0';
            p = (char *) utils::ecalloc(len, sizeof(char));
            strcpy(p, buf);
            free(buf);
            buf = p;
            p = buf + strlen(buf);
        }
        *p = ch;
    }
    *p = '\0';
    return (buf);
}



#if HAVE_POPEN

/*
 * Execute a shell command.
 * Return a pointer to a pipe connected to the shell command's standard output.
 */
static FILE * shellcmd(char *cmd)
{
    FILE *fd;

    char *shell;

    shell = lgetenv((char *)"SHELL");
    if (!isnullenv(shell))
    {
        char *scmd;
        char *esccmd;

        /*
         * Read the output of <$SHELL -c cmd>.  
         * Escape any metacharacters in the command.
         */
        esccmd = shell_quote(cmd);
        if (esccmd == NULL)
        {
            fd = popen(cmd, "r");
        } else
        {
            int len = (int) (strlen(shell) + strlen(esccmd) + 5);
            scmd = (char *) utils::ecalloc(len, sizeof(char));
            SNPRINTF3(scmd, len, "%s %s %s", shell, shell_coption(), esccmd);
            free(esccmd);
            fd = popen(scmd, "r");
            free(scmd);
        }
    } else
    {
        fd = popen(cmd, "r");
    }
    /*
     * Redirection in `popen' might have messed with the
     * standard devices.  Restore binary input mode.
     */
    SET_BINARY(0);
    return (fd);
}

#endif /* HAVE_POPEN */


/*
 * Expand a filename, doing any system-specific metacharacter substitutions.
 */
 char * lglob(char *filename)
{
    char *gfilename;

    filename = fexpand(filename);
    
#ifdef DECL_GLOB_LIST
{
    /*
     * The globbing function returns a list of names.
     */
    int length;
    char *p;
    char *qfilename;
    DECL_GLOB_LIST(list)

    GLOB_LIST(filename, list);
    if (GLOB_LIST_FAILED(list))
    {
        return (filename);
    }
    length = 1; /* Room for trailing null byte */
    for (SCAN_GLOB_LIST(list, p))
    {
        INIT_GLOB_LIST(list, p);
        qfilename = shell_quote(p);
        if (qfilename != NULL)
        {
              length += strlen(qfilename) + 1;
            free(qfilename);
        }
    }
    gfilename = (char *) ecalloc(length, sizeof(char));
    for (SCAN_GLOB_LIST(list, p))
    {
        INIT_GLOB_LIST(list, p);
        qfilename = shell_quote(p);
        if (qfilename != NULL)
        {
            sprintf(gfilename + strlen(gfilename), "%s ", qfilename);
            free(qfilename);
        }
    }
    /*
     * Overwrite the final trailing space with a null terminator.
     */
    *--p = '\0';
    GLOB_LIST_DONE(list);
}
#else
#ifdef DECL_GLOB_NAME
{
    /*
     * The globbing function returns a single name, and
     * is called multiple times to walk thru all names.
     */
    char *p;
    int len;
    int n;
    char *pfilename;
    char *qfilename;
    DECL_GLOB_NAME(fnd,drive,dir,fname,ext,handle)
    
    GLOB_FIRST_NAME(filename, &fnd, handle);
    if (GLOB_FIRST_FAILED(handle))
    {
        return (filename);
    }

    _splitpath(filename, drive, dir, fname, ext);
    len = 100;
    gfilename = (char *) ecalloc(len, sizeof(char));
    p = gfilename;
    do {
        n = (int) (strlen(drive) + strlen(dir) + strlen(fnd.GLOB_NAME) + 1);
        pfilename = (char *) ecalloc(n, sizeof(char));
        SNPRINTF3(pfilename, n, "%s%s%s", drive, dir, fnd.GLOB_NAME);
        qfilename = shell_quote(pfilename);
        free(pfilename);
        if (qfilename != NULL)
        {
            n = (int) strlen(qfilename);
            while (p - gfilename + n + 2 >= len)
            {
                /*
                 * No room in current buffer.
                 * Allocate a bigger one.
                 */
                len *= 2;
                *p = '\0';
                p = (char *) ecalloc(len, sizeof(char));
                strcpy(p, gfilename);
                free(gfilename);
                gfilename = p;
                p = gfilename + strlen(gfilename);
            }
            strcpy(p, qfilename);
            free(qfilename);
            p += n;
            *p++ = ' ';
        }
    } while (GLOB_NEXT_NAME(handle, &fnd) == 0);

    /*
     * Overwrite the final trailing space with a null terminator.
     */
    *--p = '\0';
    GLOB_NAME_DONE(handle);
}
#else
#if HAVE_POPEN
{
    /*
     * We get the shell to glob the filename for us by passing
     * an "echo" command to the shell and reading its output.
     */
    FILE *fd;
    char *s;
    char *lessecho;
    char *cmd;
    char *esc;
    int len;

    esc = get_meta_escape();
    if (strlen(esc) == 0)
        esc = (char *)"-";
    esc = shell_quote(esc);
    if (esc == NULL)
    {
        return (filename);
    }
    lessecho = lgetenv((char *)"LESSECHO");
    if (isnullenv(lessecho))
        lessecho =(char *) "lessecho";
    /*
     * Invoke lessecho, and read its output (a globbed list of filenames).
     */
    len = (int) (strlen(lessecho) + strlen(filename) + (7*strlen(metachars())) + 24);
    cmd = (char *) utils::ecalloc(len, sizeof(char));
    SNPRINTF4(cmd, len, "%s -p0x%x -d0x%x -e%s ", lessecho, openquote, closequote, esc);
    free(esc);
    for (s = metachars();  *s != '\0';  s++)
        sprintf(cmd + strlen(cmd), "-n0x%x ", *s);
    sprintf(cmd + strlen(cmd), "-- %s", filename);
    fd = shellcmd(cmd);
    free(cmd);
    if (fd == NULL)
    {
        /*
         * Cannot create the pipe.
         * Just return the original (fexpanded) filename.
         */
        return (filename);
    }
    gfilename = readfd(fd);
    pclose(fd);
    if (*gfilename == '\0')
    {
        free(gfilename);
        return (filename);
    }
}
#else
    /*
     * No globbing functions at all.  Just use the fexpanded filename.
     */
    gfilename = save(filename);
#endif
#endif
#endif
    free(filename);
    return (gfilename);
}

/*
 * @@@
 */
 char * lrealpath(char *path)
{
#if HAVE_REALPATH
    char *rpath = realpath(path, NULL);
    if (rpath != NULL)
        return (rpath);
#endif
    return (utils::save(path));
}

/*
 * Return number of %s escapes in a string.
 * Return a large number if there are any other % escapes besides %s.
 */
static int num_pct_s(char *lessopen)
{
    int num = 0;

    while (*lessopen != '\0')
    {
        if (*lessopen == '%')
        {
            if (lessopen[1] == '%')
                ++lessopen;
            else if (lessopen[1] == 's')
                ++num;
            else
                return (999);
        }
        ++lessopen;
    }
    return (num);
}

/*
 * See if we should open a "replacement file" 
 * instead of the file we're about to open.
 */
 char * open_altfile(char *filename, int *pf, void **pfd)
{
#if !HAVE_POPEN
    return (NULL);
#else
    char *lessopen;
    char *qfilename;
    char *cmd;
    int len;
    FILE *fd;
#if HAVE_FILENO
    int returnfd = 0;
#endif
    
    if (!use_lessopen)
        return (NULL);
    ch_ungetchar(-1);
    if ((lessopen = lgetenv((char *)"LESSOPEN")) == NULL)
        return (NULL);
    while (*lessopen == '|')
    {
        /*
         * If LESSOPEN starts with a |, it indicates 
         * a "pipe preprocessor".
         */
#if !HAVE_FILENO
        error((char *)"LESSOPEN pipe is not supported", NULL_PARG);
        return (NULL);
#else
        lessopen++;
        returnfd++;
#endif
    }
    if (*lessopen == '-')
    {
        /*
         * Lessopen preprocessor will accept "-" as a filename.
         */
        lessopen++;
    } else
    {
        if (strcmp(filename, "-") == 0)
            return (NULL);
    }
    if (num_pct_s(lessopen) != 1)
    {
        error((char *)"LESSOPEN ignored: must contain exactly one %%s", NULL_PARG);
        return (NULL);
    }

    qfilename = shell_quote(filename);
    len = (int) (strlen(lessopen) + strlen(qfilename) + 2);
    cmd = (char *) utils::ecalloc(len, sizeof(char));
    SNPRINTF1(cmd, len, lessopen, qfilename);
    free(qfilename);
    fd = shellcmd(cmd);
    free(cmd);
    if (fd == NULL)
    {
        /*
         * Cannot create the pipe.
         */
        return (NULL);
    }
#if HAVE_FILENO
    if (returnfd)
    {
        char c;
        int f;

        /*
         * The first time we open the file, read one char 
         * to see if the pipe will produce any data.
         * If it does, push the char back on the pipe.
         */
        f = fileno(fd);
        SET_BINARY(f);
        if (read(f, &c, 1) != 1)
        {
            /*
             * Pipe is empty.
             * If more than 1 pipe char was specified,
             * the exit status tells whether the file itself 
             * is empty, or if there is no alt file.
             * If only one pipe char, just assume no alt file.
             */
            int status = pclose(fd);
            if (returnfd > 1 && status == 0) {
                *pfd = NULL;
                *pf = -1;
                return (utils::save(FAKE_EMPTYFILE));
            }
            return (NULL);
        }
        ch_ungetchar(c);
        *pfd = (void *) fd;
        *pf = f;
        return (utils::save("-"));
    }
#endif
    cmd = readfd(fd);
    pclose(fd);
    if (*cmd == '\0')
        /*
         * Pipe is empty.  This means there is no alt file.
         */
        return (NULL);
    return (cmd);
#endif /* HAVE_POPEN */
}

/*
 * Close a replacement file.
 */
 void close_altfile(char *altfilename, char *filename)
{
#if HAVE_POPEN
    char *lessclose;
    FILE *fd;
    char *cmd;
    int len;
    
    ch_ungetchar(-1);
    if ((lessclose = lgetenv((char *)"LESSCLOSE")) == NULL)
             return;
    if (num_pct_s(lessclose) > 2) 
    {
        error((char *)"LESSCLOSE ignored; must contain no more than 2 %%s", NULL_PARG);
        return;
    }
    len = (int) (strlen(lessclose) + strlen(filename) + strlen(altfilename) + 2);
    cmd = (char *) utils::ecalloc(len, sizeof(char));
    SNPRINTF2(cmd, len, lessclose, filename, altfilename);
    fd = shellcmd(cmd);
    free(cmd);
    if (fd != NULL)
        pclose(fd);
#endif
}
        
/*
 * Is the specified file a directory?
 */
 int is_dir(char *filename)
{
    int isdir = 0;

#if HAVE_STAT
{
    int r;
    struct stat statbuf;

    r = stat(filename, &statbuf);
    isdir = (r >= 0 && S_ISDIR(statbuf.st_mode));
}
#endif
    return (isdir);
}

/*
 * Returns NULL if the file can be opened and
 * is an ordinary file, otherwise an error message
 * (if it cannot be opened or is a directory, etc.)
 */
 char * bad_file(char *filename)
{
    char *m = NULL;

    if (!force_open && is_dir(filename))
    {
        static char is_a_dir[] = " is a directory";

        m = (char *) utils::ecalloc(strlen(filename) + sizeof(is_a_dir), 
            sizeof(char));
        strcpy(m, filename);
        strcat(m, is_a_dir);
    } else
    {
#if HAVE_STAT
        int r;
        struct stat statbuf;

        r = stat(filename, &statbuf);
        if (r < 0)
        {
            m = errno_message(filename);
        } else if (force_open)
        {
            m = NULL;
        } else if (!S_ISREG(statbuf.st_mode))
        {
            static char not_reg[] = " is not a regular file (use -f to see it)";
            m = (char *) utils::ecalloc(strlen(filename) + sizeof(not_reg),
                sizeof(char));
            strcpy(m, filename);
            strcat(m, not_reg);
        }
#endif
    }
    return (m);
}

/*
 * Return the size of a file, as cheaply as possible.
 * In Unix, we can stat the file.
 */
 position_t filesize(int f)
{
#if HAVE_STAT
    struct stat statbuf;

    if (fstat(f, &statbuf) >= 0)
        return ((position_t) statbuf.st_size);
#endif
    return (seek_filesize(f));
}

/*
 * 
 */
 char * shell_coption(void)
{
    return ((char *)"-c");
}

/*
 * Return last component of a pathname.
 */
 char * last_component(char *name)
{
    char *slash;

    for (slash = name + strlen(name);  slash > name; )
    {
        --slash;
        if (*slash == *PATHNAME_SEP || *slash == '/')
            return (slash + 1);
    }
    return (name);
}

