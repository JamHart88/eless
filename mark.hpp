#ifndef MARK_H
#define MARK_H

/*
 * Keeping records or Marks in a file
 */

#include "ifile.hpp"
#include "less.hpp"

void init_mark(void);
int badmark(int c);
void setmark(int c, int where);
void clrmark(int c);
void lastmark(void);
void gomark(int c);
position_t markpos(int c);
char posmark(position_t pos);
void unmark(ifile::Ifile * ifilePtr);
void mark_check_ifile(ifile::Ifile * ifilePtr);
void save_marks(FILE* fout, char* hdr);
void restore_mark(char* line);

#endif
