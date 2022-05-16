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


public void del_ifile (IFILE h);
public IFILE next_ifile (IFILE h);
public IFILE prev_ifile (IFILE h);
public IFILE getoff_ifile (IFILE ifile);
public int nifile (void);
public IFILE get_ifile (char *filename, IFILE prev);
public char * get_filename (IFILE ifile);
public int get_index (IFILE ifile);
public void store_pos (IFILE ifile, struct scrpos *scrpos);
public void get_pos (IFILE ifile, struct scrpos *scrpos);
public void set_open (IFILE ifile);
public int opened (IFILE ifile);
public void hold_ifile (IFILE ifile, int incr);
public int held_ifile (IFILE ifile);
public void * get_filestate (IFILE ifile);
public void set_filestate (IFILE ifile, void *filestate);
public void set_altpipe (IFILE ifile, void *p);
public void * get_altpipe (IFILE ifile);
public void set_altfilename (IFILE ifile, char *altfilename);
public char * get_altfilename (IFILE ifile);


#endif