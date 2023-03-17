#ifndef CHARSET_H
#define CHARSET_H
/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Functions to define the character set
 * and do things specific to the character set.
 */

#include "less.hpp"

namespace charset {

// TODO: Move to const/functions
#define IS_ASCII_OCTET(c) (((c)&0x80) == 0)
#define IS_UTF8_TRAIL(c) (((c)&0xC0) == 0x80)
#define IS_UTF8_LEAD2(c) (((c)&0xE0) == 0xC0)
#define IS_UTF8_LEAD3(c) (((c)&0xF0) == 0xE0)
#define IS_UTF8_LEAD4(c) (((c)&0xF8) == 0xF0)
#define IS_UTF8_LEAD5(c) (((c)&0xFC) == 0xF8)
#define IS_UTF8_LEAD6(c) (((c)&0xFE) == 0xFC)
#define IS_UTF8_INVALID(c) (((c)&0xFE) == 0xFE)
#define IS_UTF8_LEAD(c) (((c)&0xC0) == 0xC0 && !IS_UTF8_INVALID(c))

void     setfmt(char* s, char** fmtvarptr, int* attrptr, char* default_fmt);
void     init_charset(void);
int      binary_char(lwchar_t c);
int      control_char(lwchar_t c);
char*    prchar(lwchar_t c);
char*    prutfchar(lwchar_t ch);
int      utf_len(int ch);
int      is_utf8_well_formed(char* ss, int slen);
void     utf_skip_to_lead(char** pp, char* limit);
lwchar_t get_wchar(const char* p);
void     put_wchar(char** pp, lwchar_t ch);
lwchar_t step_char(char** pp, signed int dir, const char* limit);
int      is_composing_char(lwchar_t ch);
int      is_ubin_char(lwchar_t ch);
int      is_wide_char(lwchar_t ch);
int      is_combining_char(lwchar_t ch1, lwchar_t ch2);

} // namespace charset
#endif