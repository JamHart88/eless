#ifndef OS_H
#define OS_H

#include "less.hpp"

public
int iread(int fd, unsigned char *buf, unsigned int len);
public
void intread(VOID_PARAM);
public
time_type get_time(VOID_PARAM);
public
char *errno_message(char *filename);
public
int percentage(POSITION num, POSITION den);
public
POSITION percent_pos(POSITION pos, int percent, long fraction);

#endif