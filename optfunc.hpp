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
 *    INIT      The option is being initialized from the command line.
 *    TOGGLE    The option is being changed from within the program.
 *    QUERY     The setting of the option is merely being queried.
 */

#include "less.hpp"

public void opt_o (int type, char *s);
public void opt__O (int type, char *s);
public void opt_j (int type, char *s);
public void calc_jump_sline (void);
public void opt_shift (int type, char *s);
public void calc_shift_count (void);
public void opt_k (int type, char *s);
public void opt_t (int type, char *s);
public void opt__T (int type, char *s);
public void opt_p (int type, char *s);
public void opt__P (int type, char *s);
public void opt_b (int type, char *s);
public void opt_i (int type, char *s);
public void opt__V (int type, char *s);
public void opt_D (int type, char *s);
public void opt_x (int type, char *s);
public void opt_quote (int type, char *s);
public void opt_rscroll (int type, char *s);
public void opt_query (int type, char *s);
public void opt_mousecap (int type, char *s);
public void opt_wheel_lines (int type, char *s);
public int get_swindow (void);

#endif