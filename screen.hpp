#ifndef SCREEN_H
#define SCREEN_H

/*
 * Routines which deal with the characteristics of the terminal.
 * Uses termcap to be as terminal-independent as possible.
 */

#include "less.hpp"

namespace screen {

void  raw_mode(int on);
void  scrsize(void);
char* special_key_str(int key);
void  get_term(void);
void  init_mouse(void);
void  deinit_mouse(void);
void  init(void);
void  deinit(void);
void  home(void);
void  add_line(void);
void  remove_top(int n); // not used
void  win32_scroll_up(int n); // not used
void  lower_left(void);
void  line_left(void); // not used
void  check_winch(void);
void  goto_line(int sindex);
void  vbell(void);
void  bell(void);
void  clear(void);
void  clear_eol(void);
void  clear_bot(void);
void  at_enter(int attr);
void  at_exit(void);
void  at_switch(int attr);
int   is_at_equiv(int attr1, int attr2);
int   apply_at_specials(int attr);
void  backspace(void); // not used
void  putbs(void);

} // namespace screen

#endif