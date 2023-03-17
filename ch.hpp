#ifndef CH_H
#define CH_H

//
// Low level character input from the input file.
// We use these special purpose routines which optimize moving
// both forward and backward from the current read pointer.
//
#include "less.hpp"

namespace ch {

void       ungetchar(int c);
void       end_logfile(void);
void       sync_logfile(void);
int        seek(position_t pos);
int        end_seek(void);
int        end_buffer_seek(void);
int        beg_seek(void);
position_t length(void);
position_t tell(void);
int        forw_get(void);
int        back_get(void);
void       setbufspace(int bufspace);
void       flush(void);
int        seekable(int f);
void       set_eof(void);
void       init(int f, int flags);
void       close(void);
int        getflags(void);

} // namespace ch

#endif
