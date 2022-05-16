#ifndef LSYSTEM_H
#define LSYSTEM_H

/*
 * Routines to execute other programs.
 * Necessarily very OS dependent.
 */

#include "less.hpp"

 void lsystem (char *cmd, char *donemsg);
 int pipe_mark (int c, char *cmd);
 int pipe_data (char *cmd, POSITION spos, POSITION epos);

#endif 