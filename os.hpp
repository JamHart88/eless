#ifndef OS_H
#define OS_H

#include "less.hpp"


int iread(int fd, unsigned char *buf, unsigned int len);

void intread(void);

time_type get_time(void);

char *errno_message(char *filename);

int percentage(POSITION num, POSITION den);

POSITION percent_pos(POSITION pos, int percent, long fraction);

#endif