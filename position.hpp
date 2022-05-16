#ifndef POSITION_H
#define POSITION_H
/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Include file for interfacing to position.c modules.
 */

#include "less.hpp"


position_t position(int sindex);

void add_forw_pos(position_t pos);

void add_back_pos(position_t pos);

void pos_clear(void);

void pos_init(void);

int onscreen(position_t pos);

int empty_screen(void);

int empty_lines(int s, int e);

void get_scrpos(struct scrpos *scrpos, int where);

int sindex_from_sline(int sline);

#define TOP (0)
#define TOP_PLUS_ONE (1)
#define BOTTOM (-1)
#define BOTTOM_PLUS_ONE (-2)
#define MIDDLE (-3)

#endif