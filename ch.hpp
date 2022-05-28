#ifndef CH_H
#define CH_H

//
// Low level character input from the input file.
// We use these special purpose routines which optimize moving
// both forward and backward from the current read pointer.
//
#include "less.hpp"

void ch_ungetchar(int c);
void end_logfile(void);
void sync_logfile(void);
int ch_seek(position_t pos);
int ch_end_seek(void);
int ch_end_buffer_seek(void);
int ch_beg_seek(void);
position_t ch_length(void);
position_t ch_tell(void);
int ch_forw_get(void);
int ch_back_get(void);
void ch_setbufspace(int bufspace);
void ch_flush(void);
int seekable(int f);
void ch_set_eof(void);
void ch_init(int f, int flags);
void ch_close(void);
int ch_getflags(void);

#endif

