#ifndef COMMAND_H
#define COMMAND_H


//
// User-level command processor.
//

#include "less.hpp"

namespace command {

int in_mca (void);
void dispversion (void);
int getcc (void);
void ungetcc (lwchar_t c);
void ungetsc (char *s);
lwchar_t peekcc (void); // not used in other modules
void commands (void);

}; // namespace command

#endif