/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Routines dealing with signals.
 *
 * A signal usually merely causes a bit to be set in the "signals" word.
 * At some convenient time, the mainline code checks to see if any
 * signals need processing by calling psignal().
 * If we happen to be reading from a file [in os::iread()] at the time
 * the signal is received, we call os::intread to interrupt the os::iread.
 */

#include "signal.hpp"
#include "forwback.hpp"
#include "less.hpp"
#include "optfunc.hpp"
#include "os.hpp"
#include "output.hpp"
#include "screen.hpp"
#include "utils.hpp"

#include <csignal>

// TODO: Move to namespace

extern int  sc_width, sc_height;
extern int  lnloop;
extern int  linenums;
extern int  wscroll;
extern int  reading;
extern int  quit_on_intr;
extern long jump_sline_fraction;

namespace sig {


static inline void ignore_result(sighandler_t unused_result)
{
  (void)unused_result;
}

/*
 * Interrupt signal handler.
 */
/* ARGSUSED*/
// -------------------------------------------
// Converted from C to C++ - C below
// static RETSIGTYPE
// u_interrupt(type)
//     int type;
static RETSIGTYPE u_interrupt(int type)
{
  screen::bell();
  ignore_result(std::signal(SIGINT, u_interrupt));
  less::Globals::sigs |= S_INTERRUPT;
  if (reading)
    os::intread(); /* May longjmp */
}

#ifdef SIGTSTP
/*
 * "Stop" (^Z) signal handler.
 */
/* ARGSUSED*/
// -------------------------------------------
// Converted from C to C++ - C below
// static RETSIGTYPE
// stop(type)
//     int type;
static RETSIGTYPE stop(int type)
{
  ignore_result(std::signal(SIGTSTP, stop));
  less::Globals::sigs |= S_STOP;
  if (reading)
    os::intread();
}
#endif

#undef SIG_LESSWINDOW
#ifdef SIGWINCH
#define SIG_LESSWINDOW SIGWINCH
#else
#ifdef SIGWIND
#define SIG_LESSWINDOW SIGWIND
#endif
#endif

#ifdef SIG_LESSWINDOW
/*
 * "Window" change handler
 */
/* ARGSUSED*/
// -------------------------------------------
// Converted from C to C++ - C below
// public RETSIGTYPE
// winch(type)
//     int type;

RETSIGTYPE winch(int type)
{
  ignore_result(std::signal(SIG_LESSWINDOW, winch));
  less::Globals::sigs |= S_WINCH;
  if (reading)
    os::intread();
}
#endif

// -------------------------------------------
// Converted from C to C++ - C below
// static RETSIGTYPE
// terminate(type)
//     int type;
static RETSIGTYPE terminate(int type)
{
  utils::quit(15);
}

/*
 * Set up the signal handlers.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// init_signals(on)
//     int on;

void init_signals(int on)
{
  if (on) {
    /*
     * Set signal handlers.
     */
    (void)std::signal(SIGINT, u_interrupt);
#ifdef SIGTSTP
    (void)std::signal(SIGTSTP, stop);
#endif
#ifdef SIGWINCH
    (void)std::signal(SIGWINCH, winch);
#endif
#ifdef SIGWIND
    (void)std::signal(SIGWIND, winch);
#endif
#ifdef SIGQUIT
    (void)std::signal(SIGQUIT, SIG_IGN);
#endif
#ifdef SIGTERM
    (void)std::signal(SIGTERM, terminate);
#endif
  } else {
    /*
     * Restore signals to defaults.
     */
    (void)std::signal(SIGINT, SIG_DFL);
#ifdef SIGTSTP
    (void)std::signal(SIGTSTP, SIG_DFL);
#endif
#ifdef SIGWINCH
    (void)std::signal(SIGWINCH, SIG_IGN);
#endif
#ifdef SIGWIND
    (void)std::signal(SIGWIND, SIG_IGN);
#endif
#ifdef SIGQUIT
    (void)std::signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
    (void)std::signal(SIGTERM, SIG_DFL);
#endif
  }
}

/*
 * Process any signals we have received.
 * A received signal cause a bit to be set in "less::Globals::sigs".
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// psignals(void)

void psignals(void)
{
  int tsignals;

  if ((tsignals = less::Globals::sigs) == 0)
    return;
  less::Globals::sigs = 0;

#ifdef SIGTSTP
  if (tsignals & S_STOP) {
    /*
     * Clean up the terminal.
     */
#ifdef SIGTTOU
    ignore_result(std::signal(SIGTTOU, SIG_IGN));
#endif
    screen::clear_bot();
    screen::deinit();
    output::flush();
    screen::raw_mode(0);
#ifdef SIGTTOU
    ignore_result(std::signal(SIGTTOU, SIG_DFL));
#endif
    ignore_result(std::signal(SIGTSTP, SIG_DFL));
    kill(getpid(), SIGTSTP);
    /*
     * ... Bye bye. ...
     * Hopefully we'll be back later and resume here...
     * Reset the terminal and arrange to jump::repaint the
     * screen when we get back to the main command loop.
     */
    ignore_result(std::signal(SIGTSTP, stop));
    screen::raw_mode(1);
    screen::init();
    screen_trashed = TRASHED;
    tsignals |= S_WINCH;
  }
#endif
#ifdef S_WINCH
  if (tsignals & S_WINCH) {
    int old_width, old_height;
    /*
     * Re-execute screen::scrsize() to read the new window size.
     */
    old_width  = sc_width;
    old_height = sc_height;
    screen::get_term();
    if (sc_width != old_width || sc_height != old_height) {
      wscroll = (sc_height + 1) / 2;
      optfunc::calc_jump_sline();
      optfunc::calc_shift_count();
    }
    screen_trashed = TRASHED;
  }
#endif
  if (tsignals & S_INTERRUPT) {
    if (quit_on_intr)
      utils::quit(QUIT_INTERRUPT);
  }
}

} // namespace sig