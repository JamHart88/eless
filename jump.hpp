#ifndef JUMP_H
#define JUMP_H

#include "less.hpp"


void jump_forw(void);

void jump_forw_buffered(void);

void jump_back(LINENUM linenum);

void repaint(void);

void jump_percent(int percent, long fraction);

void jump_line_loc(POSITION pos, int sline);

void jump_loc(POSITION pos, int sline);

#endif