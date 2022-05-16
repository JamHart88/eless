#ifndef JUMP_H
#define JUMP_H

#include "less.hpp"


void jump_forw(void);

void jump_forw_buffered(void);

void jump_back(linenum_t linenum);

void repaint(void);

void jump_percent(int percent, long fraction);

void jump_line_loc(position_t pos, int sline);

void jump_loc(position_t pos, int sline);

#endif