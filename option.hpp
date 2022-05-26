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

char* propt(int c);
void scan_option(char* s);
void toggle_option(struct loption* o, int lower, char* s, int how_toggle);
int opt_has_param(struct loption* o);
char* opt_prompt(struct loption* o);
char* opt_toggle_disallowed(int c);
int isoptpending(void);
void nopendopt(void);
int getnum(char** sp, char* printopt, int* errp);
long getfraction(char** sp, char* printopt, int* errp);
int get_quit_at_eof(void);

#define END_OPTION_STRING ('$')

/*
 * Types of options.
 */
// clang-format off
#define BOOL 01            /* Boolean option: 0 or 1 */
#define TRIPLE 02          /* Triple-valued option: 0, 1 or 2 */
#define NUMBER 04          /* Numeric option */
#define STRING 010         /* String-valued option */
#define NOVAR 020          /* No associated variable */
#define REPAINT 040        /* Repaint screen after toggling option */
#define NO_TOGGLE 0100     /* Option cannot be toggled with "-" cmd */
#define HL_REPAINT 0200    /* Repaint hilites after toggling option */
#define NO_QUERY 0400      /* Option cannot be queried with "_" cmd */
#define INIT_HANDLER 01000 /* Call option handler function at startup */
// clang-format on

#define OTYPE (BOOL | TRIPLE | NUMBER | STRING | NOVAR)

#define OLETTER_NONE '\1' /* Invalid option letter */

/*
 * Argument to a handling function tells what type of activity:
 */
// clang-format off
#define INIT 0   /* Initialization (from command line) */
#define QUERY 1  /* Query (from _ or - command) */
#define TOGGLE 2 /* Change value (from - command) */
// clang-format on

/* Flag to toggle_option to specify how to "toggle" */
// clang-format off
#define OPT_NO_TOGGLE 0
#define OPT_TOGGLE    1
#define OPT_UNSET     2
#define OPT_SET       3
#define OPT_NO_PROMPT 0100
// clang-format on

/* Error code from findopt_name */
#define OPT_AMBIG 1

// clang-format off
struct optname
{
    char *oname;           /* Long (GNU-style) option name */
    struct optname *onext; /* List of synonymous option names */
};
// clang-format on

#define OPTNAME_MAX 32 /* Max length of long option name */

// clang-format off
struct loption
{
    char oletter;                           /* The controlling letter (a-z) */
    struct optname *onames;                 /* Long (GNU-style) option name */
    int otype;                              /* Type of the option */
    int odefault;                           /* Default value */
    int *ovar;                              /* Pointer to the associated variable */
    void(*ofunc) (int, char *);             /* Pointer to special handling function */
    char *odesc[3];                         /* Description of each value */
};
// clang-format on
#endif