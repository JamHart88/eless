#ifndef EDIT_H
#define EDIT_H

#include "less.hpp"

public void init_textlist (struct textlist *tlist, char *str);
public char * forw_textlist (struct textlist *tlist, char *prev);
public char * back_textlist (struct textlist *tlist, char *prev);
public int edit (char *filename);
public int edit_ifile (IFILE ifile);
public int edit_list (char *filelist);
public int edit_first (void);
public int edit_last (void);
public int edit_next (int n);
public int edit_prev (int n);
public int edit_index (int n);
public IFILE save_curr_ifile (void);
public void unsave_ifile (IFILE save_ifile);
public void reedit_ifile (IFILE save_ifile);
public void reopen_curr_ifile (void);
public int edit_stdin (void);
public void cat_file (void);
public void use_logfile (char *filename);


#endif