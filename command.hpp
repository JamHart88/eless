#ifndef COMMAND_H
#define COMMAND_H


//
// User-level command processor.
//

#include "less.hpp"

public int in_mca (void);
public void dispversion (void);
public int getcc (void);
public void ungetcc (LWCHAR c);
public void ungetsc (char *s);
public LWCHAR peekcc (void);
public void commands (void);

#endif