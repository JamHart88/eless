#ifndef MARK_H
#define MARK_H

/*
 * Keeping records or Marks in a file
 */

#include "less.hpp"


void init_mark(void);

int badmark(int c);

void setmark(int c, int where);

void clrmark(int c);

void lastmark(void);

void gomark(int c);

POSITION markpos(int c);

char posmark(POSITION pos);

void unmark(IFILE ifile);

void mark_check_ifile(IFILE ifile);

void save_marks(FILE *fout, char *hdr);

void restore_mark(char *line);

#endif
