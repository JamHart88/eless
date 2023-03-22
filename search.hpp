#ifndef SEARCH_H
#define SEARCH_H

/*
 * Routines to search a file for a pattern.
 */

#include "less.hpp"

namespace search {

void       init_search(void);
void       repaint_hilite(int on);
void       clear_attn(void);
void       undo_search(void);
void       clr_hlist(struct hilite_tree* anchor); // not used
void       clr_hilite(void);
void       clr_filter(void); // not used
int        is_filtered(position_t pos);
position_t next_unfiltered(position_t pos);
position_t prev_unfiltered(position_t pos);
int        is_hilited(position_t pos, position_t epos, int nohide, int* p_matches);
void       chg_hilite(void);
void       chg_caseless(void);
int        search(int search_type, char* pattern, int n);
void       prep_hilite(position_t spos, position_t epos, int maxlines);
void       set_filter_pattern(char* pattern, int search_type);
int        is_filtering(void);

} // namespace search

#endif