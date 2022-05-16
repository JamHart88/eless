#ifndef OPTTBL_H
#define OPTTBL_H

/*
 * The option table.
 */

#include "less.hpp"

public
void init_option(void);
public
struct loption *findopt(int c);
public
struct loption *findopt_name(char **p_optname, char **p_oname, int *p_err);

#endif
