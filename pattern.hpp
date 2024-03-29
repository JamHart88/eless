#ifndef PATTERN_H
#define PATTERN_H
/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

#include "less.hpp"

#if HAVE_GNU_REGEX
#define __USE_GNU 1
#include <regex.h>
#define PATTERN_TYPE struct re_pattern_buffer*
#define CLEAR_PATTERN(name) name = NULL
#endif

#if HAVE_POSIX_REGCOMP
#include <regex.h>
#ifdef REG_EXTENDED
#define REGCOMP_FLAG REG_EXTENDED
#else
#define REGCOMP_FLAG 0
#endif
#define PATTERN_TYPE regex_t*
#define CLEAR_PATTERN(name) name = NULL
#endif

#if HAVE_PCRE
#include <pcre.h>
#define PATTERN_TYPE pcre*
#define CLEAR_PATTERN(name) name = NULL
#endif

#if HAVE_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#define PATTERN_TYPE pcre2_code*
#define CLEAR_PATTERN(name) name = NULL
#endif

#if HAVE_RE_COMP
char* re_comp(char*);
int   re_exec(char*);
#define PATTERN_TYPE int
#define CLEAR_PATTERN(name) name = 0
#endif

#if HAVE_REGCMP
char*        regcmp(char*);
char*        regex(char**, char*);
extern char* __loc1;
#define PATTERN_TYPE char**
#define CLEAR_PATTERN(name) name = NULL
#endif

#if HAVE_V8_REGCOMP
#include "regexp.hpp"
extern int reg_show_error;
#define PATTERN_TYPE struct regexp*
#define CLEAR_PATTERN(name) name = NULL
#endif

#if NO_REGEX
#define PATTERN_TYPE void*
#define CLEAR_PATTERN(name)
#endif

namespace pattern {
int   compile_pattern(char* pattern, int search_type, PATTERN_TYPE* comp_pattern); // not used
void  uncompile_pattern(PATTERN_TYPE* pattern);                                    // not used
int   valid_pattern(char* pattern);                                                // not used
int   is_null_pattern(PATTERN_TYPE pattern);                                       // not used
int   match_pattern(PATTERN_TYPE pattern, char* tpattern, char* line, int line_len, char** sp, char** ep, int notbol,
      int search_type);
char* pattern_lib_name(void);

} // namespace pattern

#endif