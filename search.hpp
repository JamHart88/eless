#ifndef SEARCH_H
#define SEARCH_H

/*
 * Routines to search a file for a pattern.
 */

#include "less.hpp"


 void init_search (void);
 void repaint_hilite (int on);
 void clear_attn (void);
 void undo_search (void);
 void clr_hlist (struct hilite_tree *anchor);
 void clr_hilite (void);
 void clr_filter (void);
 int is_filtered (POSITION pos);
 POSITION next_unfiltered (POSITION pos);
 POSITION prev_unfiltered (POSITION pos);
 int is_hilited (POSITION pos, POSITION epos, int nohide, int *p_matches);
 void chg_hilite (void);
 void chg_caseless (void);
 int search (int search_type, char *pattern, int n);
 void prep_hilite (POSITION spos, POSITION epos, int maxlines);
 void set_filter_pattern (char *pattern, int search_type);
 int is_filtering (void);


#endif