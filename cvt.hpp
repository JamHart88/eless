#ifndef CVT_H
#define CVT_H

#include "less.hpp"

int cvt_length(int len, int ops);
int* cvt_alloc_chpos(int len);
void cvt_text(char* odst, char* osrc, int* chpos, int* lenp, int ops);

#endif