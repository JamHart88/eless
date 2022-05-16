#ifndef EDIT_H
#define EDIT_H

#include "less.hpp"

 void init_textlist (struct textlist *tlist, char *str);
 char * forw_textlist (struct textlist *tlist, char *prev);
 char * back_textlist (struct textlist *tlist, char *prev);
 int edit (char *filename);
 int edit_ifile (IFILE ifile);
 int edit_list (char *filelist);
 int edit_first (void);
 int edit_last (void);
 int edit_next (int n);
 int edit_prev (int n);
 int edit_index (int n);
 IFILE save_curr_ifile (void);
 void unsave_ifile (IFILE save_ifile);
 void reedit_ifile (IFILE save_ifile);
 void reopen_curr_ifile (void);
 int edit_stdin (void);
 void cat_file (void);
 void use_logfile (char *filename);


#endif