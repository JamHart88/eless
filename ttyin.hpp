#ifndef TTYIN_H
#define TTYIN_H

/*
 * Routines dealing with getting input from the keyboard (i.e. from the user).
 */

#include "less.hpp"

public void open_getchr (void);
public void close_getchr (void);
public int default_wheel_lines (void);
public int getchr (void);

#endif