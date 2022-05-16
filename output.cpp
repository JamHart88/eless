/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * High level routines dealing with the output to the screen.
 */

#include "output.hpp"
#include "command.hpp"
#include "forwback.hpp"
#include "less.hpp"
#include "line.hpp"
#include "screen.hpp"
#include "ttyin.hpp"

public
int errmsgs; /* Count of messages displayed by error() */
public
int need_clr;
public
int final_attr;
public
int at_prompt;

extern int sigs;
extern int sc_width;
extern int so_s_width, so_e_width;
extern int screen_trashed;
extern int any_display;
extern int is_tty;
extern int oldbot;

/*
 * Display the line which is in the line buffer.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// put_line(void)
public
void put_line(void)
{
    int c;
    int i;
    int a;

    if (ABORT_SIGS()) {
        /*
         * Don't output if a signal is pending.
         */
        screen_trashed = 1;
        return;
    }

    final_attr = AT_NORMAL;

    for (i = 0; (c = gline(i, &a)) != '\0'; i++) {
        at_switch(a);
        final_attr = a;
        if (c == '\b')
            putbs();
        else
            putchr(c);
    }

    at_exit();
}

static char obuf[OUTBUF_SIZE];
static char* ob = obuf;

/*
 * Flush buffered output.
 *
 * If we haven't displayed any file data yet,
 * output messages on error output (file descriptor 2),
 * otherwise output on standard output (file descriptor 1).
 *
 * This has the desirable effect of producing all
 * error messages on error output if standard output
 * is directed to a file.  It also does the same if
 * we never produce any real output; for example, if
 * the input file(s) cannot be opened.  If we do
 * eventually produce output, code in edit() makes
 * sure these messages can be seen before they are
 * overwritten or scrolled away.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// flush(void)
public
void flush(void)
{
    int n;
    int fd;

    n = (int)(ob - obuf);
    if (n == 0)
        return;

    fd = (any_display) ? 1 : 2;
    if (write(fd, obuf, n) != n)
        screen_trashed = 1;
    ob = obuf;
}

/*
 * Output a character.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public int
// putchr(c)
//     int c;
public
int putchr(int c)
{
    if (need_clr) {
        need_clr = 0;
        clear_bot();
    }
    /*
     * Some versions of flush() write to *ob, so we must flush
     * when we are still one char from the end of obuf.
     */
    if (ob >= &obuf[sizeof(obuf) - 1])
        flush();
    *ob++ = c;
    at_prompt = 0;
    return (c);
}

/*
 * Output a string.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// putstr(s)
//     const char *s;
public
void putstr(const char* s)
{
    while (*s != '\0')
        putchr(*s++);
}

/*
 * Convert an string to an integral type.
 */
#define STR_TO_TYPE_FUNC(funcname, type)  \
    type funcname(char* buf, char** ebuf) \
    {                                     \
        type val = 0;                     \
        for (;;) {                        \
            char c = *buf++;              \
            if (c < '0' || c > '9')       \
                break;                    \
            val = 10 * val + c - '0';     \
        }                                 \
        if (ebuf != NULL)                 \
            *ebuf = buf;                  \
        return val;                       \
    }

STR_TO_TYPE_FUNC(lstrtopos, POSITION)
STR_TO_TYPE_FUNC(lstrtoi, int)

/*
 * Output an integer in a given radix.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// static int
// iprint_int(num)
//     int num;
static int iprint_int(int num)
{
    char buf[strlen_bound<int>() + 2];

    typeToStr<int>(num, buf);
    putstr(buf);
    return ((int)strlen(buf));
}

/*
 * Output a line number in a given radix.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// static int
// iprint_linenum(num)
//     LINENUM num;
static int iprint_linenum(LINENUM num)
{
    char buf[strlen_bound<LINENUM>() + 2];

    typeToStr<LINENUM>(num, buf);
    putstr(buf);
    return ((int)strlen(buf));
}

/*
 * This function implements printf-like functionality
 * using a more portable argument list mechanism than printf's.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// static int
// less_printf(fmt, parg)
//     char *fmt;
//     PARG *parg;
static int less_printf(char* fmt, PARG* parg)
{
    char* s;
    int col;

    col = 0;
    while (*fmt != '\0') {
        if (*fmt != '%') {
            putchr(*fmt++);
            col++;
        } else {
            ++fmt;
            switch (*fmt++) {
            case 's':
                s = parg->p_string;
                parg++;
                while (*s != '\0') {
                    putchr(*s++);
                    col++;
                }
                break;
            case 'd':
                col += iprint_int(parg->p_int);
                parg++;
                break;
            case 'n':
                col += iprint_linenum(parg->p_linenum);
                parg++;
                break;
            case '%':
                putchr('%');
                break;
            }
        }
    }
    return (col);
}

/*
 * Get a RETURN.
 * If some other non-trivial char is pressed, unget it, so it will
 * become the next command.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// get_return(void)
public
void get_return(void)
{
    int c;

#if ONLY_RETURN
    while ((c = getchr()) != '\n' && c != '\r')
        bell();
#else
    c = getchr();
    if (c != '\n' && c != '\r' && c != ' ' && c != READ_INTR)
        ungetcc(c);
#endif
}

/*
 * Output a message in the lower left corner of the screen
 * and wait for carriage return.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// error(fmt, parg)
//     char *fmt;
//     PARG *parg;
public
void error(char* fmt, PARG* parg)
{
    int col = 0;
    static char return_to_continue[] = "  (press RETURN)";

    errmsgs++;

    if (any_display && is_tty) {
        if (!oldbot)
            squish_check();
        at_exit();
        clear_bot();
        at_enter(AT_STANDOUT);
        col += so_s_width;
    }

    col += less_printf(fmt, parg);

    if (!(any_display && is_tty)) {
        putchr('\n');
        return;
    }

    putstr(return_to_continue);
    at_exit();
    col += sizeof(return_to_continue) + so_e_width;

    get_return();
    lower_left();
    clear_eol();

    if (col >= sc_width)
        /*
         * Printing the message has probably scrolled the screen.
         * {{ Unless the terminal doesn't have auto margins,
         *    in which case we just hammered on the right margin. }}
         */
        screen_trashed = 1;

    flush();
}

static char intr_to_abort[] = "... (interrupt to abort)";

/*
 * Output a message in the lower left corner of the screen
 * and don't wait for carriage return.
 * Usually used to warn that we are beginning a potentially
 * time-consuming operation.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// ierror(fmt, parg)
//     char *fmt;
//     PARG *parg;
public
void ierror(char* fmt, PARG* parg)
{
    at_exit();
    clear_bot();
    at_enter(AT_STANDOUT);
    (void)less_printf(fmt, parg);
    putstr(intr_to_abort);
    at_exit();
    flush();
    need_clr = 1;
}

/*
 * Output a message in the lower left corner of the screen
 * and return a single-character response.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public int
// query(fmt, parg)
//     char *fmt;
//     PARG *parg;
public
int query(char* fmt, PARG* parg)
{
    int c;
    int col = 0;

    if (any_display && is_tty)
        clear_bot();

    (void)less_printf(fmt, parg);
    c = getchr();

    if (!(any_display && is_tty)) {
        putchr('\n');
        return (c);
    }

    lower_left();
    if (col >= sc_width)
        screen_trashed = 1;
    flush();

    return (c);
}
