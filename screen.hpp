#ifndef SCREEN_H
#define SCREEN_H

/*
 * Routines which deal with the characteristics of the terminal.
 * Uses termcap to be as terminal-independent as possible.
 */

#include "less.hpp"

public
void raw_mode(int on);
public
void scrsize(void);
public
char* special_key_str(int key);
public
void get_term(void);
public
void init_mouse(void);
public
void deinit_mouse(void);
public
void init(void);
public
void deinit(void);
public
void home(void);
public
void add_line(void);
public
void remove_top(int n);
public
void win32_scroll_up(int n);
public
void lower_left(void);
public
void line_left(void);
public
void check_winch(void);
public
void goto_line(int sindex);
public
void vbell(void);
public
void bell(void);
public
void clear(void);
public
void clear_eol(void);
public
void clear_bot(void);
public
void at_enter(int attr);
public
void at_exit(void);
public
void at_switch(int attr);
public
int is_at_equiv(int attr1, int attr2);
public
int apply_at_specials(int attr);
public
void backspace(void);
public
void putbs(void);

#endif