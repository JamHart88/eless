#ifndef INPUT_H
#define INPUT_H

/*
 * High level routines dealing with getting lines of input
 * from the file being viewed.
 *
 * When we speak of "lines" here, we mean PRINTABLE lines;
 * lines processed with respect to the screen width.
 * We use the term "raw line" to refer to lines simply
 * delimited by newlines; not processed with respect to screen width.
 */

#include "less.hpp"

namespace input {

position_t forw_line(position_t curr_pos);
position_t back_line(position_t curr_pos);
void set_attnpos(position_t pos);

} // namespace input
#endif