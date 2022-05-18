#ifndef CMDBUF_H
#define CMDBUF_H

//
// Functions which manipulate the command buffer.
// Used only by command() and related functions.
//

#include "less.hpp"

extern char openquote;
extern char closequote;

 void cmd_reset (void);
 void clear_cmd (void);
 void cmd_putstr (const char *s);
 int len_cmdbuf (void);
 void set_mlist (void *mlist, int cmdflags);
 void cmd_addhist (struct mlist *mlist, const char *cmd, int modified);
 void cmd_accept (void);
 int cmd_char (int c);
 linenum_t cmd_int (long *frac);
 char * get_cmdbuf (void);
 char * cmd_lastpattern (void);
 void init_cmdhist (void);
 void save_cmdhist (void);

#endif