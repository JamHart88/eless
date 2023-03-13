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
 * If we happen to be reading from a file [in iread()] at the time
 * the signal is received, we call intread to interrupt the iread.
 */

#include "signal.hpp"
#include "forwback.hpp"
#include "less.hpp"
#include "optfunc.hpp"
#include "os.hpp"
#include "output.hpp"
#include "screen.hpp"
#include "utils.hpp"

#include <signal.h>

// TODO: Move to namespace

extern int sc_width, sc_height;
extern int lnloop;
extern int linenums;
extern int wscroll;
extern int reading;
extern int quit_on_intr;
extern long jump_sline_fraction;

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
    bell();
    signal(SIGINT, u_interrupt);
    less::Settings::sigs |= S_INTERRUPT;
    if (reading)
        intread(); /* May longjmp */
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
    signal(SIGTSTP, stop);
    less::Settings::sigs |= S_STOP;
    if (reading)
        intread();
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
    signal(SIG_LESSWINDOW, winch);
    less::Settings::sigs |= S_WINCH;
    if (reading)
        intread();
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
        (void)signal(SIGINT, u_interrupt);
#ifdef SIGTSTP
        (void)signal(SIGTSTP, stop);
#endif
#ifdef SIGWINCH
        (void)signal(SIGWINCH, winch);
#endif
#ifdef SIGWIND
        (void)signal(SIGWIND, winch);
#endif
#ifdef SIGQUIT
        (void)signal(SIGQUIT, SIG_IGN);
#endif
#ifdef SIGTERM
        (void)signal(SIGTERM, terminate);
#endif
    } else {
        /*
         * Restore signals to defaults.
         */
        (void)signal(SIGINT, SIG_DFL);
#ifdef SIGTSTP
        (void)signal(SIGTSTP, SIG_DFL);
#endif
#ifdef SIGWINCH
        (void)signal(SIGWINCH, SIG_IGN);
#endif
#ifdef SIGWIND
        (void)signal(SIGWIND, SIG_IGN);
#endif
#ifdef SIGQUIT
        (void)signal(SIGQUIT, SIG_DFL);
#endif
#ifdef SIGTERM
        (void)signal(SIGTERM, SIG_DFL);
#endif
    }
}

/*
 * Process any signals we have received.
 * A received signal cause a bit to be set in "less::Settings::sigs".
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// psignals(void)

void psignals(void)
{
    int tsignals;

    if ((tsignals = less::Settings::sigs) == 0)
        return;
    less::Settings::sigs = 0;

#ifdef SIGTSTP
    if (tsignals & S_STOP) {
        /*
         * Clean up the terminal.
         */
#ifdef SIGTTOU
        signal(SIGTTOU, SIG_IGN);
#endif
        clear_bot();
        deinit();
        flush();
        raw_mode(0);
#ifdef SIGTTOU
        signal(SIGTTOU, SIG_DFL);
#endif
        signal(SIGTSTP, SIG_DFL);
        kill(getpid(), SIGTSTP);
        /*
         * ... Bye bye. ...
         * Hopefully we'll be back later and resume here...
         * Reset the terminal and arrange to repaint the
         * screen when we get back to the main command loop.
         */
        signal(SIGTSTP, stop);
        raw_mode(1);
        init();
        screen_trashed = TRASHED;
        tsignals |= S_WINCH;
    }
#endif
#ifdef S_WINCH
    if (tsignals & S_WINCH) {
        int old_width, old_height;
        /*
         * Re-execute scrsize() to read the new window size.
         */
        old_width = sc_width;
        old_height = sc_height;
        get_term();
        if (sc_width != old_width || sc_height != old_height) {
            wscroll = (sc_height + 1) / 2;
            calc_jump_sline();
            calc_shift_count();
        }
        screen_trashed = TRASHED;
    }
#endif
    if (tsignals & S_INTERRUPT) {
        if (quit_on_intr)
            utils::quit(QUIT_INTERRUPT);
    }
}
