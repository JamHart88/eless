#ifndef OPTION_H
#define OPTION_H
/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

#include "less.hpp"

namespace option {

enum option_t {
  OPT_OFF    = 0,
  OPT_ON     = 1,
  OPT_ONPLUS = 2
};

/* How quiet should we be?
   NOT_QUIET     : Ring bell at eof and for errors
   LITTLE_QUIET  : Ring bell only for errors
   VERY_QUIET    : Never ring bell
*/
enum quiet_t {
  NOT_QUIET    = 0,
  LITTLE_QUIET = 1,
  VERY_QUIET   = 2
};

struct Option {
  // Logile related parameters
  static quiet_t  quiet;
  static bool     plusoption;
  static int      less_is_more;
  static option_t quit_at_eof;       // Quit after hitting end of file twice
  static bool     opt_use_backslash; // Use backslash escaping in option parsing
};

inline quiet_t  Option::quiet             = NOT_QUIET;
inline bool     Option::plusoption        = false;
inline int      Option::less_is_more      = 0; // Make compatible with POSIX more
inline option_t Option::quit_at_eof       = OPT_ON;
inline bool     Option::opt_use_backslash = false;

constexpr const char END_OPTION_STRING = '$';

/*
 * Types of options.
 */

constexpr const int BOOL         = 01;    // Boolean option: 0 or 1
constexpr const int TRIPLE       = 02;    // Triple-valued option: 0, 1 or 2
constexpr const int NUMBER       = 04;    // Numeric option
constexpr const int STRING       = 010;   // String-valued option
constexpr const int NOVAR        = 020;   // No associated variable
constexpr const int REPAINT      = 040;   // Repaint screen after toggling option
constexpr const int NO_TOGGLE    = 0100;  // Option cannot be toggled with "-" cmd
constexpr const int HL_REPAINT   = 0200;  // Repaint hilites after toggling option
constexpr const int NO_QUERY     = 0400;  // Option cannot be queried with "_" cmd
constexpr const int INIT_HANDLER = 01000; // Call option handler function at startup

constexpr const int OTYPE = (BOOL | TRIPLE | NUMBER | STRING | NOVAR);

constexpr const char OLETTER_NONE = '\1'; // Invalid option letter

/*
 * Argument to a handling function tells what type of activity:
 */

constexpr const int INIT   = 0; // Initialization (from command line)
constexpr const int QUERY  = 1; // Query (from _ or - command)
constexpr const int TOGGLE = 2; // Change value (from - command)

// Flag to toggle_option to specify how to "toggle"

constexpr const int OPT_NO_TOGGLE = 0;
constexpr const int OPT_TOGGLE    = 1;
constexpr const int OPT_UNSET     = 2;
constexpr const int OPT_SET       = 3;
constexpr const int OPT_NO_PROMPT = 0100;

/* Error code from opttbl::findopt_name */
constexpr const int OPT_AMBIG = 1;

struct optname {
  char*           oname; // Long (GNU-style) option name
  struct optname* onext; // List of synonymous option names
};

constexpr const int OPTNAME_MAX = 32; // Max length of long option name

struct loption {
  char            oletter;   // The controlling letter (a-z)
  struct optname* onames;    // Long (GNU-style) option name
  int             otype;     // Type of the option
  int             odefault;  // Default value
  int*            ovar;      // Pointer to the associated variable
  void (*ofunc)(int, char*); // Pointer to special handling function
  char* odesc[3];            // Description of each value
};

char*    propt(int c);
void     scan_option(char* s);
void     toggle_option(struct loption* o, int lower, char* s, int how_toggle);
int      opt_has_param(struct loption* o);
char*    opt_prompt(struct loption* o);
char*    opt_toggle_disallowed(int c);
int      isoptpending(void);
void     nopendopt(void);
int      getnum(char** sp, char* printopt, int* errp);
long     getfraction(char** sp, char* printopt, int* errp);
option_t get_quit_at_eof(void);

} // namespace option

#endif