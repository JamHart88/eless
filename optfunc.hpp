#ifndef OPTFUNC_H
#define OPTFUNC_H

/*
 * Handling functions for command line options.
 *
 * Most options are handled by the generic code in option.c.
 * But all string options, and a few non-string options, require
 * special handling specific to the particular option.
 * This special processing is done by the "handling functions" in this file.
 *
 * Each handling function is passed a "type" and, if it is a string
 * option, the string which should be "assigned" to the option.
 * The type may be one of:
 *    option::INIT      The option is being initialized from the command line.
 *    option::TOGGLE    The option is being changed from within the program.
 *    option::QUERY     The setting of the option is merely being queried.
 */

#include "less.hpp"

void opt_o(int type, char* s);
void opt__O(int type, char* s);
void opt_j(int type, char* s);
void calc_jump_sline(void);
void opt_shift(int type, char* s);
void calc_shift_count(void);
void opt_k(int type, char* s);
void opt_t(int type, char* s);
void opt__T(int type, char* s);
void opt_p(int type, char* s);
void opt__P(int type, char* s);
void opt_b(int type, char* s);
void opt_i(int type, char* s);
void opt__V(int type, char* s);
void opt_D(int type, char* s);
void opt_x(int type, char* s);
void opt_quote(int type, char* s);
void opt_rscroll(int type, char* s);
void opt_query(int type, char* s);
void opt_mousecap(int type, char* s);
void opt_wheel_lines(int type, char* s);
int  get_swindow(void);

#endif