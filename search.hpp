#ifndef SEARCH_H
#define SEARCH_H

/*
 * Routines to search a file for a pattern.
 */

#include "less.hpp"


public void init_search (void);
public void repaint_hilite (int on);
public void clear_attn (void);
public void undo_search (void);
public void clr_hlist (struct hilite_tree *anchor);
public void clr_hilite (void);
public void clr_filter (void);
public int is_filtered (POSITION pos);
public POSITION next_unfiltered (POSITION pos);
public POSITION prev_unfiltered (POSITION pos);
public int is_hilited (POSITION pos, POSITION epos, int nohide, int *p_matches);
public void chg_hilite (void);
public void chg_caseless (void);
public int search (int search_type, char *pattern, int n);
public void prep_hilite (POSITION spos, POSITION epos, int maxlines);
public void set_filter_pattern (char *pattern, int search_type);
public int is_filtering (void);


#endif