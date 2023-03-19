#ifndef OS_H
#define OS_H

#include "less.hpp"

namespace os {
    
int        iread(int fd, unsigned char* buf, unsigned int len);
void       intread(void);
time_t     get_time(void);
char*      errno_message(char* filename);
int        percentage(position_t num, position_t den);
position_t percent_pos(position_t pos, int percent, long fraction);

} // namespace os

#endif