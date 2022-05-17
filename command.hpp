#ifndef COMMAND_H
#define COMMAND_H


//
// User-level command processor.
//

#include "less.hpp"

 int in_mca (void);
 void dispversion (void);
 int getcc (void);
 void ungetcc (lwchar_t c);
 void ungetsc (char *s);
 lwchar_t peekcc (void);
 void commands (void);

#endif