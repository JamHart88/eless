#ifndef LINE_H
#define LINE_H

/*
 * Routines to manipulate the "line buffer".
 * The line buffer holds a line of output as it is being built
 * in preparation for output to the screen.
 */

#include "less.hpp"


void init_line(void);

int is_ascii_char(LWCHAR ch);

void prewind(void);

void plinenum(POSITION pos);

void pshift_all(void);

int is_ansi_end(LWCHAR ch);

int is_ansi_middle(LWCHAR ch);

void skip_ansi(char **pp, const char *limit);

int pappend(int c, POSITION pos);

int pflushmbc(void);

void pdone(int endline, int chopped, int forw);

void set_status_col(int c);

int gline(int i, int *ap);

void null_line(void);

POSITION forw_raw_line(POSITION curr_pos, char **linep, int *line_lenp);

POSITION back_raw_line(POSITION curr_pos, char **linep, int *line_lenp);

int rrshift(void);

#endif