#ifndef OPTTBL_H
#define OPTTBL_H

/*
 * The option table.
 */

#include "less.hpp"
#include "option.hpp"

namespace opttbl {

void                    init_option(void);
struct option::loption* findopt(int c);
struct option::loption* findopt_name(char** p_optname, char** p_oname, int* p_err);

} // namespace opttbl

#endif
