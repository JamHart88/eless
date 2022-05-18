#ifndef IFILE_H
#define IFILE_H

//
// An IFILE represents an input file.
//
// It is actually a pointer to an ifile structure,
// but is opaque outside this module.
// Ifile structures are kept in a linked list in the order they
// appear on the command line.
// Any new file which does not already appear in the list is
// inserted after the current file.
//

#include "less.hpp"


void del_ifile(IFILE h);

IFILE next_ifile(IFILE h);

IFILE prev_ifile(IFILE h);

IFILE getoff_ifile(IFILE ifile);

int nifile(void);

IFILE get_ifile(const char *filename, IFILE prev);

char *get_filename(IFILE ifile);

int get_index(IFILE ifile);

void store_pos(IFILE ifile, struct scrpos *scrpos);

void get_pos(IFILE ifile, struct scrpos *scrpos);

void set_open(IFILE ifile);

int opened(IFILE ifile);

void hold_ifile(IFILE ifile, int incr);

int held_ifile(IFILE ifile);

void *get_filestate(IFILE ifile);

void set_filestate(IFILE ifile, void *filestate);

void set_altpipe(IFILE ifile, void *p);

void *get_altpipe(IFILE ifile);

void set_altfilename(IFILE ifile, char *altfilename);

char *get_altfilename(IFILE ifile);

#endif