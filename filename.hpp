#ifndef FILENAME_H
#define FILENAME_H

//
// Routines to mess around with filenames (and files).
// Much of this is very OS dependent.
//

#include "less.h"

public char * shell_unquote (char *str);
public char * get_meta_escape (void);
public char * shell_quote (char *s);
public char * homefile (char *filename);
public char * fexpand (char *s);
public char * fcomplete (char *s);
public int bin_file (int f);
public char * lglob (char *filename);
public char * lrealpath (char *path);
public char * open_altfile (char *filename, int *pf, void **pfd);
public void close_altfile (char *altfilename, char *filename);
public int is_dir (char *filename);
public char * bad_file (char *filename);

#endif