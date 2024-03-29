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

#include "os.hpp"
#include "less.hpp"
#include "output.hpp"
#include "utils.hpp"

#include <csetjmp>
#include <csignal>
#include <ctime>
#if HAVE_ERRNO_H
#include <cerrno>
#endif
#if HAVE_VALUES_H
#include <values.h>
#endif

/*
 * BSD setjmp() saves (and longjmp() restores) the signal mask.
 * This costs a system call or two per setjmp(), so if possible we screen::clear the
 * signal mask with sigsetmask(), and use _setjmp()/_longjmp() instead.
 * On other systems, setjmp() doesn't affect the signal mask and so
 * _setjmp() does not exist; we just use setjmp().
 */
#define SET_JUMP _setjmp
#define LONG_JUMP _longjmp

int reading;

static jmp_buf read_label;

namespace os {

/*
 * Like read() system call, but is deliberately interruptible.
 * A call to intread() from a signal handler will interrupt
 * any pending iread().
 */
int iread(int fd, unsigned char* buf, unsigned int len)
{
  int n;

start:
  if (SET_JUMP(read_label)) {
    /*
     * We jumped here from intread.
     */
    reading = 0;
    // #if HAVE_SIGPROCMASK
    {
      sigset_t mask;
      sigemptyset(&mask);
      sigprocmask(SIG_SETMASK, &mask, NULL);
    }
    // #else
    // #if HAVE_SIGSETMASK
    //         sigsetmask(0);
    // #endif
    // #endif
    return (READ_INTR);
  }

  output::flush();
  reading = 1;
  n       = read(fd, buf, len);
#if 1
  /*
   * This is a kludge to workaround a problem on some systems
   * where terminating a remote tty connection causes read() to
   * start returning 0 forever, instead of -1.
   */
  {
    if (!less::Globals::ignore_eoi) {
      static int consecutive_nulls = 0;
      if (n == 0)
        consecutive_nulls++;
      else
        consecutive_nulls = 0;
      if (consecutive_nulls > 20)
        utils::quit(QUIT_ERROR);
    }
  }
#endif
  reading = 0;
  if (n < 0) {
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
void intread(void)
{
  LONG_JUMP(read_label, 1);
}

/*
 * Return the current time.
 */

time_t get_time(void)
{
  return time(NULL);
}

/*
 * errno_message: Return an output::error message based on the value of "errno".
 */
char* errno_message(char* filename)
{
  char* p;
  char* m;
  int   len;
#if HAVE_ERRNO
#if MUST_DEFINE_ERRNO
  extern int errno;
#endif
  p = strerror(errno);
#else
  p            = "cannot open";
#endif
  len = (int)(strlen(filename) + strlen(p) + 3);
  m   = (char*)utils::ecalloc(len, sizeof(char));
  ignore_result(snprintf(m, len, "%s: %s", filename, p));
  return (m);
}

/* #define HAVE_FLOAT 0 */

static position_t muldiv(position_t val, position_t num, position_t den)
{
#if HAVE_FLOAT
  double v = (((double)val) * num) / den;
  return ((position_t)(v + 0.5));
#else
  position_t v = ((position_t)val) * num;

  if (v / num == val)
    /* No overflow */
    return (position_t)(v / den);
  else
    /* Above calculation overflows;
     * use a method that is less precise but won't overflow. */
    return (position_t)(val / (den / num));
#endif
}

/*
 * Return the ratio of two POSITIONS, as a percentage.
 * {{ Assumes a position_t is a long int. }}
 */
int percentage(position_t num, position_t den)
{
  return (int)muldiv(num, (position_t)100, den);
}

/*
 * Return the specified percentage of a position_t.
 */
position_t percent_pos(position_t pos, int percent, long fraction)
{
  /* Change percent (parts per 100) to perden (parts per NUM_FRAC_DENOM). */
  position_t perden = (percent * (NUM_FRAC_DENOM / 100)) + (fraction / 100);

  if (perden == 0)
    return (0);
  return muldiv(pos, perden, (position_t)NUM_FRAC_DENOM);
}

} // namespace os