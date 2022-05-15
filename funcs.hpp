#ifndef FUNCS_H
#define FUNCH_H

#include "less.h"

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

public void lsystem (char *cmd, char *donemsg);
public int pipe_mark (int c, char *cmd);
public int pipe_data (char *cmd, POSITION spos, POSITION epos);
public void init_mark (void);
public int badmark (int c);
public void setmark (int c, int where);
public void clrmark (int c);
public void lastmark (void);
public void gomark (int c);
public POSITION markpos (int c);
public char posmark (POSITION pos);
public void unmark (IFILE ifile);
public void mark_check_ifile (IFILE ifile);
public void save_marks (FILE *fout, char *hdr);
public void restore_mark (char *line);
public void opt_o (int type, char *s);
public void opt__O (int type, char *s);
public void opt_j (int type, char *s);
public void calc_jump_sline (void);
public void opt_shift (int type, char *s);
public void calc_shift_count (void);
public void opt_k (int type, char *s);
public void opt_t (int type, char *s);
public void opt__T (int type, char *s);
public void opt_p (int type, char *s);
public void opt__P (int type, char *s);
public void opt_b (int type, char *s);
public void opt_i (int type, char *s);
public void opt__V (int type, char *s);
public void opt_D (int type, char *s);
public void opt_x (int type, char *s);
public void opt_quote (int type, char *s);
public void opt_rscroll (int type, char *s);
public void opt_query (int type, char *s);
public void opt_mousecap (int type, char *s);
public void opt_wheel_lines (int type, char *s);
public int get_swindow (void);
public char * propt (int c);
public void scan_option (char *s);
public void toggle_option (struct loption *o, int lower, char *s, int how_toggle);
public int opt_has_param (struct loption *o);
public char * opt_prompt (struct loption *o);
public char * opt_toggle_disallowed (int c);
public int isoptpending (void);
public void nopendopt (void);
public int getnum (char **sp, char *printopt, int *errp);
public long getfraction (char **sp, char *printopt, int *errp);
public int get_quit_at_eof (void);
public void init_option (void);
public struct loption * findopt (int c);
public struct loption * findopt_name (char **p_optname, char **p_oname, int *p_err);

public int  os9_signal (int type, RETSIGTYPE (*handler)());
public void put_line (void);
public void flush (void);
public int putchr (int c);
public void putstr (const char *s);
public void get_return (void);
public void error (char *fmt, PARG *parg);
public void ierror (char *fmt, PARG *parg);
public int query (char *fmt, PARG *parg);
public int compile_pattern (char *pattern, int search_type, PATTERN_TYPE *comp_pattern);
public void uncompile_pattern (PATTERN_TYPE *pattern);
public int valid_pattern (char *pattern);
public int is_null_pattern (PATTERN_TYPE pattern);
public int match_pattern (PATTERN_TYPE pattern, char *tpattern, char *line, int line_len, char **sp, char **ep, int notbol, int search_type);
public char * pattern_lib_name (void);
public POSITION position (int sindex);
public void add_forw_pos (POSITION pos);
public void add_back_pos (POSITION pos);
public void pos_clear (void);
public void pos_init (void);
public int onscreen (POSITION pos);
public int empty_screen (void);
public int empty_lines (int s, int e);
public void get_scrpos (struct scrpos *scrpos, int where);
public int sindex_from_sline (int sline);
public void init_prompt (void);
public char * pr_expand (const char *proto, int maxwidth);
public char * eq_message (void);
public char * pr_string (void);
public char * wait_message (void);
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