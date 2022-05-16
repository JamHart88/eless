#ifndef TTYIN_H
#define TTYIN_H

/*
 * Routines dealing with getting input from the keyboard (i.e. from the user).
 */

#include "less.hpp"

 void open_getchr (void);
 void close_getchr (void);
 int default_wheel_lines (void);
 int getchr (void);

#endif