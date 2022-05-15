/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Operating system dependent routines.
 *
 * Most of the stuff in here is based on Unix, but an attempt
 * has been made to make things work on other operating systems.
 * This will sometimes result in a loss of functionality, unless
 * someone rewrites code specifically for the new operating system.
 *
 * The makefile provides defines to decide whether various
 * Unix features are present.
 */

#include "less.h"
#include "os.h"

#include <setjmp.h>
#include <signal.h>
#include <time.h>
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_VALUES_H
#include <values.h>
#endif

/*
 * BSD setjmp() saves (and longjmp() restores) the signal mask.
 * This costs a system call or two per setjmp(), so if possible we clear the
 * signal mask with sigsetmask(), and use _setjmp()/_longjmp() instead.
 * On other systems, setjmp() doesn't affect the signal mask and so
 * _setjmp() does not exist; we just use setjmp().
 */
#if HAVE__SETJMP && HAVE_SIGSETMASK
#define SET_JUMP _setjmp
#define LONG_JUMP _longjmp
#else
#define SET_JUMP setjmp
#define LONG_JUMP longjmp
#endif

public
int reading;

static jmp_buf read_label;

extern int sigs;

/*
 * Like read() system call, but is deliberately interruptible.
 * A call to intread() from a signal handler will interrupt
 * any pending iread().
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public int
// iread(fd, buf, len)
//     int fd;
//     unsigned char *buf;
//     unsigned int len;
public
int iread(int fd, unsigned char *buf, unsigned int len)
{
    int n;

start:
    if (SET_JUMP(read_label))
    {
        /*
         * We jumped here from intread.
         */
        reading = 0;
#if HAVE_SIGPROCMASK
        {
            sigset_t mask;
            sigemptyset(&mask);
            sigprocmask(SIG_SETMASK, &mask, NULL);
        }
#else
#if HAVE_SIGSETMASK
        sigsetmask(0);
#endif
#endif
        return (READ_INTR);
    }

    flush();
    reading = 1;
    n = read(fd, buf, len);
#if 1
    /*
     * This is a kludge to workaround a problem on some systems
     * where terminating a remote tty connection causes read() to
     * start returning 0 forever, instead of -1.
     */
    {
        extern int ignore_eoi;
        if (!ignore_eoi)
        {
            static int consecutive_nulls = 0;
            if (n == 0)
                consecutive_nulls++;
            else
                consecutive_nulls = 0;
            if (consecutive_nulls > 20)
                quit(QUIT_ERROR);
        }
    }
#endif
    reading = 0;
    if (n < 0)
    {
#if HAVE_ERRNO
        /*
         * Certain values of errno indicate we should just retry the read.
         */
#if MUST_DEFINE_ERRNO
        extern int errno;
#endif
#ifdef EINTR
        if (errno == EINTR)
            goto start;
#endif
#ifdef EAGAIN
        if (errno == EAGAIN)
            goto start;
#endif
#endif
        return (-1);
    }
    return (n);
}

/*
 * Interrupt a pending iread().
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// intread(VOID_PARAM)
public
void intread(VOID_PARAM)
{
    LONG_JUMP(read_label, 1);
}

/*
 * Return the current time.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public time_type
// get_time(VOID_PARAM)
public
time_type get_time(VOID_PARAM)
{
    time_type t;

    time(&t);
    return (t);
}

/*
 * errno_message: Return an error message based on the value of "errno".
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public char *
// errno_message(filename)
//     char *filename;
public
char *errno_message(char *filename)
{
    char *p;
    char *m;
    int len;
#if HAVE_ERRNO
#if MUST_DEFINE_ERRNO
    extern int errno;
#endif
    p = strerror(errno);
#else
    p = "cannot open";
#endif
    len = (int)(strlen(filename) + strlen(p) + 3);
    m = (char *)ecalloc(len, sizeof(char));
    SNPRINTF2(m, len, "%s: %s", filename, p);
    return (m);
}

/* #define HAVE_FLOAT 0 */

// -------------------------------------------
// Converted from C to C++ - C below
// static POSITION
// muldiv(val, num, den)
//     POSITION val, num, den;
static POSITION muldiv(POSITION val, POSITION num, POSITION den)
{
#if HAVE_FLOAT
    double v = (((double)val) * num) / den;
    return ((POSITION)(v + 0.5));
#else
    POSITION v = ((POSITION)val) * num;

    if (v / num == val)
        /* No overflow */
        return (POSITION)(v / den);
    else
        /* Above calculation overflows;
         * use a method that is less precise but won't overflow. */
        return (POSITION)(val / (den / num));
#endif
}

/*
 * Return the ratio of two POSITIONS, as a percentage.
 * {{ Assumes a POSITION is a long int. }}
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public int
// percentage(num, den)
//     POSITION num;
//     POSITION den;
public
int percentage(POSITION num, POSITION den)
{
    return (int)muldiv(num, (POSITION)100, den);
}

/*
 * Return the specified percentage of a POSITION.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public POSITION
// percent_pos(pos, percent, fraction)
//     POSITION pos;
//     int percent;
//     long fraction;
public
POSITION percent_pos(POSITION pos, int percent, long fraction)
{
    /* Change percent (parts per 100) to perden (parts per NUM_FRAC_DENOM). */
    POSITION perden = (percent * (NUM_FRAC_DENOM / 100)) + (fraction / 100);

    if (perden == 0)
        return (0);
    return (POSITION)muldiv(pos, perden, (POSITION)NUM_FRAC_DENOM);
}
