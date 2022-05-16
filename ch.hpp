#ifndef CH_H
#define CH_H

//
// Low level character input from the input file.
// We use these special purpose routines which optimize moving
// both forward and backward from the current read pointer.
//

#include "less.hpp"

public
void ch_ungetchar(int c);
public
void end_logfile(void);
public
void sync_logfile(void);
public
int ch_seek(POSITION pos);
public
int ch_end_seek(void);
public
int ch_end_buffer_seek(void);
public
int ch_beg_seek(void);
public
POSITION ch_length(void);
public
POSITION ch_tell(void);
public
int ch_forw_get(void);
public
int ch_back_get(void);
public
void ch_setbufspace(int bufspace);
public
void ch_flush(void);
public
int seekable(int f);
public
void ch_set_eof(void);
public
void ch_init(int f, int flags);
public
void ch_close(void);
public
int ch_getflags(void);


#endif
