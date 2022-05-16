#ifndef MARK_H
#define MARK_H

/*
 * Keeping records or Marks in a file
 */

#include "less.hpp"

public
void init_mark(void);
public
int badmark(int c);
public
void setmark(int c, int where);
public
void clrmark(int c);
public
void lastmark(void);
public
void gomark(int c);
public
POSITION markpos(int c);
public
char posmark(POSITION pos);
public
void unmark(IFILE ifile);
public
void mark_check_ifile(IFILE ifile);
public
void save_marks(FILE *fout, char *hdr);
public
void restore_mark(char *line);

#endif
