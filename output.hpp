#ifndef OUTPUT_H
#define OUTPUT_H

/*
 * High level routines dealing with the output to the screen.
 */

#include "less.hpp"


void put_line(void);

void flush(void);

int putchr(int c);

void putstr(const char *s);

void get_return(void);

void error(char *fmt, parg_t parg);

void ierror(char *fmt, parg_t parg);

int query(char *fmt, parg_t parg);

#endif
