#ifndef OUTPUT_H
#define OUTPUT_H

/*
 * High level routines dealing with the output to the screen.
 */

#include "less.hpp"

public
void put_line(void);
public
void flush(void);
public
int putchr(int c);
public
void putstr(const char *s);
public
void get_return(void);
public
void error(char *fmt, PARG *parg);
public
void ierror(char *fmt, PARG *parg);
public
int query(char *fmt, PARG *parg);

#endif