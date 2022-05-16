

#ifndef UTILS_H
#define UTILS_H

/* 
 * Utility Functions
 */

#include "less.hpp"

namespace utils {

 char * save (const char *s);
 void * ecalloc (int count, unsigned int size);
 char * skipsp (char *s);
 int sprefix (char *ps, char *s, int uppercase);
 void quit (int status);

}

#endif