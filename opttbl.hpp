#ifndef OPTTBL_H
#define OPTTBL_H

/*
 * The option table.
 */

#include "less.hpp"


void init_option(void);

struct loption *findopt(int c);

struct loption *findopt_name(char **p_optname, char **p_oname, int *p_err);

#endif
