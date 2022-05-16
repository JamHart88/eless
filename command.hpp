#ifndef COMMAND_H
#define COMMAND_H


//
// User-level command processor.
//

#include "less.hpp"

 int in_mca (void);
 void dispversion (void);
 int getcc (void);
 void ungetcc (LWCHAR c);
 void ungetsc (char *s);
 LWCHAR peekcc (void);
 void commands (void);

#endif