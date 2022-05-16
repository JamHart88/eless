#ifndef CVT_H
#define CVT_H

#include "less.hpp"

public int cvt_length (int len, int ops);
public int * cvt_alloc_chpos (int len);
public void cvt_text (char *odst, char *osrc, int *chpos, int *lenp, int ops);


#endif