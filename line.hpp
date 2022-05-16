#ifndef LINE_H
#define LINE_H

/*
 * Routines to manipulate the "line buffer".
 * The line buffer holds a line of output as it is being built
 * in preparation for output to the screen.
 */

#include "less.hpp"

public
void init_line(void);
public
int is_ascii_char(LWCHAR ch);
public
void prewind(void);
public
void plinenum(POSITION pos);
public
void pshift_all(void);
public
int is_ansi_end(LWCHAR ch);
public
int is_ansi_middle(LWCHAR ch);
public
void skip_ansi(char **pp, const char *limit);
public
int pappend(int c, POSITION pos);
public
int pflushmbc(void);
public
void pdone(int endline, int chopped, int forw);
public
void set_status_col(int c);
public
int gline(int i, int *ap);
public
void null_line(void);
public
POSITION forw_raw_line(POSITION curr_pos, char **linep, int *line_lenp);
public
POSITION back_raw_line(POSITION curr_pos, char **linep, int *line_lenp);
public
int rrshift(void);

#endif