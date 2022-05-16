#ifndef DECODE_H
#define DECODE_H


// 
// Routines to decode user commands.
//
// This is all table driven.
// A command table is a sequence of command descriptors.
// Each command descriptor is a sequence of bytes with the following format:
//    <c1><c2>...<cN><0><action>
// The characters c1,c2,...,cN are the command string; that is,
// the characters which the user must type.
// It is terminated by a null <0> byte.
// The byte after the null byte is the action code associated
// with the command string.
// If an action byte is OR-ed with A_EXTRA, this indicates
// that the option byte is followed by an extra string.
//
// There may be many command tables.
// The first (default) table is built-in.
// Other tables are read in from "lesskey" files.
// All the tables are linked together and are searched in order.
//

#include "less.hpp"


 void expand_cmd_tables (void);
 void init_cmds (void);
 void add_fcmd_table (char *buf, int len);
 void add_ecmd_table (char *buf, int len);
 int fcmd_decode (char *cmd, char **sp);
 int ecmd_decode (char *cmd, char **sp);
 char * lgetenv (char *var);
 int isnullenv (char* s);
 int lesskey (char *filename, int sysvar);
 void add_hometable (char *envname, char *def_filename, int sysvar);
 int editchar (int c, int flags); 

#endif