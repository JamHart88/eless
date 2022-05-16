#ifndef LSYSTEM_H
#define LSYSTEM_H

/*
 * Routines to execute other programs.
 * Necessarily very OS dependent.
 */

#include "less.hpp"

public void lsystem (char *cmd, char *donemsg);
public int pipe_mark (int c, char *cmd);
public int pipe_data (char *cmd, POSITION spos, POSITION epos);

#endif 