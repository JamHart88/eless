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


#define IS_ASCII_OCTET(c)   (((c) & 0x80) == 0)
#define IS_UTF8_TRAIL(c)    (((c) & 0xC0) == 0x80)
#define IS_UTF8_LEAD2(c)    (((c) & 0xE0) == 0xC0)
#define IS_UTF8_LEAD3(c)    (((c) & 0xF0) == 0xE0)
#define IS_UTF8_LEAD4(c)    (((c) & 0xF8) == 0xF0)
#define IS_UTF8_LEAD5(c)    (((c) & 0xFC) == 0xF8)
#define IS_UTF8_LEAD6(c)    (((c) & 0xFE) == 0xFC)
#define IS_UTF8_INVALID(c)  (((c) & 0xFE) == 0xFE)
#define IS_UTF8_LEAD(c)     (((c) & 0xC0) == 0xC0 && !IS_UTF8_INVALID(c))


public void setfmt (char *s, char **fmtvarptr, int *attrptr, char *default_fmt);
public void init_charset (void);
public int binary_char (LWCHAR c);
public int control_char (LWCHAR c);
public char * prchar (LWCHAR c);
public char * prutfchar (LWCHAR ch);
public int utf_len (int ch);
public int is_utf8_well_formed (char *ss, int slen);
public void utf_skip_to_lead (char **pp, char *limit);
public LWCHAR get_wchar (const char *p);
public void put_wchar (char **pp, LWCHAR ch);
public LWCHAR step_char (char **pp, signed int dir, const char *limit);
public int is_composing_char (LWCHAR ch);
public int is_ubin_char (LWCHAR ch);
public int is_wide_char (LWCHAR ch);
public int is_combining_char (LWCHAR ch1, LWCHAR ch2);

#endif