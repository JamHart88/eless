#ifndef FORWBACK_H
#define FORWBACK_H

//
// Primitives for displaying the file on the screen,
// scrolling either forward or backward.
//

#include "less.hpp"


int eof_displayed(void);

int entire_file_displayed(void);

void squish_check(void);

void forw(int n, position_t pos, int force, int only_last, int nblank);

void back(int n, position_t pos, int force, int only_last);

void forward(int n, int force, int only_last);

void backward(int n, int force, int only_last);

int get_back_scroll(void);

int get_one_screen(void);

#endif
