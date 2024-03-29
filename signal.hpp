#ifndef SIGNAL_H
#define SIGNAL_H

/*
 * Routines dealing with signals.
 *
 * A signal usually merely causes a bit to be set in the "signals" word.
 * At some convenient time, the mainline code checks to see if any
 * signals need processing by calling psignal().
 * If we happen to be reading from a file [in os::iread()] at the time
 * the signal is received, we call os::intread to interrupt the os::iread.
 */

#include "less.hpp"

namespace sig {

RETSIGTYPE winch(int type);
void       init_signals(int on);
void       psignals(void);

} // namespace sig
#endif