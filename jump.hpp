#ifndef JUMP_H
#define JUMP_H

#include "less.hpp"

public
void jump_forw(void);
public
void jump_forw_buffered(void);
public
void jump_back(LINENUM linenum);
public
void repaint(void);
public
void jump_percent(int percent, long fraction);
public
void jump_line_loc(POSITION pos, int sline);
public
void jump_loc(POSITION pos, int sline);

#endif