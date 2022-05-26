#ifndef LINE_H
#define LINE_H

/*
 * Routines to manipulate the "line buffer".
 * The line buffer holds a line of output as it is being built
 * in preparation for output to the screen.
 */

#include "less.hpp"

void init_line(void);
int is_ascii_char(lwchar_t ch);
void prewind(void);
void plinenum(position_t pos);
void pshift_all(void);
int is_ansi_end(lwchar_t ch);
int is_ansi_middle(lwchar_t ch);
void skip_ansi(char** pp, const char* limit);
int pappend(int c, position_t pos);
int pflushmbc(void);
void pdone(bool endline, bool chopped, int forw);
void set_status_col(int c);
int gline(int i, int* ap);
void null_line(void);
position_t forw_raw_line(position_t curr_pos, char** linep, int* line_lenp);
position_t back_raw_line(position_t curr_pos, char** linep, int* line_lenp);
int rrshift(void);

#endif