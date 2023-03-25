#ifndef LSYSTEM_H
#define LSYSTEM_H

/*
 * Routines to execute other programs.
 * Necessarily very OS dependent.
 */

#include "less.hpp"

namespace lsystem {

void lsystem(char* cmd, char* donemsg);
int  pipe_mark(int c, char* cmd);
int  pipe_data(char* cmd, position_t spos, position_t epos); // not used

} // namespace lsystem
#endif