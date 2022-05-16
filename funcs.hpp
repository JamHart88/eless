#ifndef FUNCS_H
#define FUNCS_H

#include "less.hpp"

// TODO: create a utils file that holds these routines
// These are all from main.cpp
public char * save (const char *s);
public VOID_POINTER ecalloc (int count, unsigned int size);
public char * skipsp (char *s);
public int sprefix (char *ps, char *s, int uppercase);
public void quit (int status);


//cmd
//command
//cvt
//decode.cpp
// edit
//filename
//forback
//ifile
//jump
//line
//linenum
//lsystem
//mark
//optfunc
//opttbl
//output
//pattern
//position
//prompt

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
public RETSIGTYPE winch (int type);
public void init_signals (int on);
public void psignals (void);
public void cleantags (void);
public int gettagtype (void);
public void findtag (char *tag);
public POSITION tagsearch (void);
public char * nexttag (int n);
public char * prevtag (int n);
public int ntags (void);
public int curr_tag (void);
public int edit_tagfile (void); // tag.cpp
public void open_getchr (void);
public void close_getchr (void);
public int default_wheel_lines (void);
public int getchr (void);

#endif