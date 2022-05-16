

#ifndef UTILS_H
#define UTILS_H

/* 
 * Utility Functions
 */

#include "less.hpp"

namespace utils {

public char * save (const char *s);
public void * ecalloc (int count, unsigned int size);
public char * skipsp (char *s);
public int sprefix (char *ps, char *s, int uppercase);
public void quit (int status);

}

#endif