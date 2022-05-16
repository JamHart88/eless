#ifndef FUNCS_H
#define FUNCH_H

#include "less.hpp"

public char * save (const char *s);
public VOID_POINTER ecalloc (int count, unsigned int size);
public char * skipsp (char *s);
public int sprefix (char *ps, char *s, int uppercase);
public void quit (int status);
public void raw_mode (int on);
public void scrsize (void);
public char * special_key_str (int key);
public void get_term (void);
public void init_mouse (void);
public void deinit_mouse (void);
public void init (void);
public void deinit (void);
public void home (void);
public void add_line (void);
public void remove_top (int n);
public void win32_scroll_up (int n);
public void lower_left (void);
public void line_left (void);
public void check_winch (void);
public void goto_line (int sindex);
public void vbell (void);
public void bell (void);
public void clear (void);
public void clear_eol (void);
public void clear_bot (void);
public void at_enter (int attr);
public void at_exit (void);
public void at_switch (int attr);
public int is_at_equiv (int attr1, int attr2);
public int apply_at_specials (int attr);
public void backspace (void);
public void putbs (void);
public int win32_kbhit (void);
public char WIN32getch (void);
public void WIN32setcolors (int fg, int bg);
public void WIN32textout (char *text, int len);

public void setfmt (char *s, char **fmtvarptr, int *attrptr, char *default_fmt);
public void init_charset (void);
public int binary_char (LWCHAR c);
public int control_char (LWCHAR c);
public char * prchar (LWCHAR c);
public char * prutfchar (LWCHAR ch);
public int utf_len (int ch);
public int is_utf8_well_formed (char *ss, int slen);
public void utf_skip_to_lead (char **pp, char *limit);
public LWCHAR get_wchar (const char *p);
public void put_wchar (char **pp, LWCHAR ch);
public LWCHAR step_char (char **pp, signed int dir, const char *limit);
public int is_composing_char (LWCHAR ch);
public int is_ubin_char (LWCHAR ch);
public int is_wide_char (LWCHAR ch);
public int is_combining_char (LWCHAR ch1, LWCHAR ch2);

//cmd
//command
//cvt
//decode.cpp
// edit
//filename

public POSITION filesize (int f);
public char * shell_coption (void);
public char * last_component (char *name);

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