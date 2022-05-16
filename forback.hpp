#ifndef FORBACK_H
#define FORBACK_H


//
// Primitives for displaying the file on the screen,
// scrolling either forward or backward.
//

#include "less.hpp"

public int eof_displayed (void);
public int entire_file_displayed (void);
public void squish_check (void);
public void forw (int n, POSITION pos, int force, int only_last, int nblank);
public void back (int n, POSITION pos, int force, int only_last);
public void forward (int n, int force, int only_last);
public void backward (int n, int force, int only_last);
public int get_back_scroll (void);
public int get_one_screen (void);

#endif
