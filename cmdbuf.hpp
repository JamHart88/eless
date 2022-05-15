#ifndef CMDBUF_H
#define CMDBUF_H

//
// Functions which manipulate the command buffer.
// Used only by command() and related functions.
//

#include "less.h"

public void cmd_reset (void);
public void clear_cmd (void);
public void cmd_putstr (const char *s);
public int len_cmdbuf (void);
public void set_mlist (void *mlist, int cmdflags);
public void cmd_addhist (struct mlist *mlist, const char *cmd, int modified);
public void cmd_accept (void);
public int cmd_char (int c);
public LINENUM cmd_int (long *frac);
public char * get_cmdbuf (void);
public char * cmd_lastpattern (void);
public void init_cmdhist (void);
public void save_cmdhist (void);

#endif