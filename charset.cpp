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

#include "charset.hpp"
#include "less.hpp"
#include "output.hpp"
#include "utils.hpp"

#include <cctype>
#include <clocale>
#include <langinfo.h>

#include "decode.hpp"

extern int bs_mode;

namespace charset {

/*
 * Predefined character sets,
 * selected by the LESSCHARSET environment variable.
 */
// clang-format off
struct charset {
    const char *name;
    int        *p_flag;
    const char *desc;
} charsets[] = {
    { "ascii",        NULL,                      "8bcccbcc18b95.b" },
    { "utf-8",        &less::Globals::utf_mode, "8bcccbcc18b95.b126.bb" },
    { "iso8859",      NULL,                      "8bcccbcc18b95.33b." },
    { "latin3",       NULL,                      "8bcccbcc18b95.33b5.b8.b15.b4.b12.b18.b12.b." },
    { "arabic",       NULL,                      "8bcccbcc18b95.33b.3b.7b2.13b.3b.b26.5b19.b" },
    { "greek",        NULL,                      "8bcccbcc18b95.33b4.2b4.b3.b35.b44.b" },
    { "greek2005",    NULL,                      "8bcccbcc18b95.33b14.b35.b44.b" },
    { "hebrew",       NULL,                      "8bcccbcc18b95.33b.b29.32b28.2b2.b" },
    { "koi8-r",       NULL,                      "8bcccbcc18b95.b." },
    { "KOI8-T",       NULL,                      "8bcccbcc18b95.b8.b6.b8.b.b.5b7.3b4.b4.b3.b.b.3b." },
    { "georgianps",   NULL,                      "8bcccbcc18b95.3b11.4b12.2b." },
    { "tcvn",         NULL,                      "b..b...bcccbccbbb7.8b95.b48.5b." },
    { "TIS-620",      NULL,                      "8bcccbcc18b95.b.4b.11b7.8b." },
    { "next",         NULL,                      "8bcccbcc18b95.bb125.bb" },
    { "dos",          NULL,                      "8bcccbcc12bc5b95.b." },
    { "windows-1251", NULL,                      "8bcccbcc12bc5b95.b24.b." },
    { "windows-1252", NULL,                      "8bcccbcc12bc5b95.b.b11.b.2b12.b." },
    { "windows-1255", NULL,                      "8bcccbcc12bc5b95.b.b8.b.5b9.b.4b." },
    { "ebcdic",       NULL,                      "5bc6bcc7bcc41b.9b7.9b5.b..8b6.10b6.b9.7b9.8b8.17b3.3b9.7b9.8b8.6b10.b.b.b." },
    { "IBM-1047",     NULL,                      "4cbcbc3b9cbccbccbb4c6bcc5b3cbbc4bc4bccbc191.b" },
    { NULL,           NULL,                      NULL }
};
// clang-format on

/*
 * Support "locale charmap"/nl_langinfo(CODESET) values, as well as others.
 */
// clang-format off
struct cs_alias {
    const char *name;
    const char *oname;
} cs_aliases[] = {
    { "UTF-8",           "utf-8" },
    { "utf8",            "utf-8" },
    { "UTF8",            "utf-8" },
    { "ANSI_X3.4-1968",  "ascii" },
    { "US-ASCII",        "ascii" },
    { "latin1",          "iso8859" },
    { "ISO-8859-1",      "iso8859" },
    { "latin9",          "iso8859" },
    { "ISO-8859-15",     "iso8859" },
    { "latin2",          "iso8859" },
    { "ISO-8859-2",      "iso8859" },
    { "ISO-8859-3",      "latin3" },
    { "latin4",          "iso8859" },
    { "ISO-8859-4",      "iso8859" },
    { "cyrillic",        "iso8859" },
    { "ISO-8859-5",      "iso8859" },
    { "ISO-8859-6",      "arabic" },
    { "ISO-8859-7",      "greek" },
    { "IBM9005",         "greek2005" },
    { "ISO-8859-8",      "hebrew" },
    { "latin5",          "iso8859" },
    { "ISO-8859-9",        "iso8859" },
    { "latin6",          "iso8859" },
    { "ISO-8859-10",     "iso8859" },
    { "latin7",          "iso8859" },
    { "ISO-8859-13",     "iso8859" },
    { "latin8",          "iso8859" },
    { "ISO-8859-14",     "iso8859" },
    { "latin10",         "iso8859" },
    { "ISO-8859-16",     "iso8859" },
    { "IBM437",          "dos" },
    { "EBCDIC-US",       "ebcdic" },
    { "IBM1047",         "IBM-1047" },
    { "KOI8-R",          "koi8-r" },
    { "KOI8-U",          "koi8-r" },
    { "GEORGIAN-PS",     "georgianps" },
    { "TCVN5712-1",      "tcvn" },
    { "NEXTSTEP",        "next" },
    { "windows",         "windows-1252" }, /* backward compatibility */
    { "CP1251",          "windows-1251" },
    { "CP1252",          "windows-1252" },
    { "CP1255",          "windows-1255" },
    { NULL,              NULL }
};
// clang-format on

#define IS_BINARY_CHAR 01
#define IS_CONTROL_CHAR 02

static char  chardef[256];
static char* binfmt    = NULL;
static char* utfbinfmt = NULL;

/*
 * Define a charset, given a description string.
 * The string consists of 256 letters,
 * one for each character in the charset.
 * If the string is shorter than 256 letters, missing letters
 * are taken to be identical to the last one.
 * A decimal number followed by a letter is taken to be a
 * repetition of the letter.
 *
 * Each letter is one of:
 *    . normal character
 *    b binary character
 *    c control character
 */
static void ichardef(char* s)
{
  char* cp;
  int   n;
  char  v;

  n  = 0;
  v  = 0;
  cp = chardef;
  while (*s != '\0') {
    switch (*s++) {
    case '.':
      v = 0;
      break;
    case 'c':
      v = IS_CONTROL_CHAR;
      break;
    case 'b':
      v = IS_BINARY_CHAR | IS_CONTROL_CHAR;
      break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      n = (10 * n) + (s[-1] - '0');
      continue;

    default:
      output::error((char*)"invalid chardef", NULL_PARG);
      utils::quit(QUIT_ERROR);
      /*NOTREACHED*/
    }

    do {
      if (cp >= chardef + sizeof(chardef)) {
        output::error((char*)"chardef longer than 256", NULL_PARG);
        utils::quit(QUIT_ERROR);
        /*NOTREACHED*/
      }
      *cp++ = v;
    } while (--n > 0);
    n = 0;
  }

  while (cp < chardef + sizeof(chardef))
    *cp++ = v;
}

/*
 * Define a charset, given a charset name.
 * The valid charset names are listed in the "charsets" array.
 */
static int icharset(char* name, int no_error)
{
  struct charset*  p;
  struct cs_alias* a;

  if (name == NULL || *name == '\0')
    return (0);

  /* First see if the name is an alias. */
  for (a = cs_aliases; a->name != NULL; a++) {
    if (strcmp(name, a->name) == 0) {
      name = (char*)a->oname;
      break;
    }
  }

  for (p = charsets; p->name != NULL; p++) {
    if (strcmp(name, p->name) == 0) {
      ichardef((char*)p->desc);
      if (p->p_flag != NULL) {
        *(p->p_flag) = 1;
      }
      return (1);
    }
  }

  if (!no_error) {
    output::error((char*)"invalid charset name", NULL_PARG);
    utils::quit(QUIT_ERROR);
  }
  return (0);
}

#if HAVE_LOCALE
/*
 * Define a charset, given a locale name.
 */
static void ilocale(void)
{
  int c;

  for (c = 0; c < (int)sizeof(chardef); c++) {
    if (isprint(c))
      chardef[c] = 0;
    else if (iscntrl(c))
      chardef[c] = IS_CONTROL_CHAR;
    else
      chardef[c] = IS_BINARY_CHAR | IS_CONTROL_CHAR;
  }
}
#endif

/*
 * Define the printing format for control (or binary utf) chars.
 */

void setfmt(char* s, char** fmtvarptr, int* attrptr, char* default_fmt)
{
  if (s && less::Globals::utf_mode) {
    /* It would be too hard to account for width otherwise.  */
    char const* t = s;
    while (*t) {
      if (*t < ' ' || *t > '~') {
        s = default_fmt;
        goto attr;
      }
      t++;
    }
  }

  /* %n is evil */
  if (s == NULL || *s == '\0' || (*s == '*' && (s[1] == '\0' || s[2] == '\0' || strchr(s + 2, 'n'))) || (*s != '*' && strchr(s, 'n')))
    s = default_fmt;

  /*
   * Select the attributes if it starts with "*".
   */
attr:
  if (*s == '*' && s[1] != '\0') {
    switch (s[1]) {
    case 'd':
      *attrptr = AT_BOLD;
      break;
    case 'k':
      *attrptr = AT_BLINK;
      break;
    case 's':
      *attrptr = AT_STANDOUT;
      break;
    case 'u':
      *attrptr = AT_UNDERLINE;
      break;
    default:
      *attrptr = AT_NORMAL;
      break;
    }
    s += 2;
  }
  *fmtvarptr = s;
}

/*
 *
 */
static void set_charset(void)
{
  char* s;

  /*
   * See if environment variable LESSCHARSET is defined.
   */
  s = decode::lgetenv((char*)"LESSCHARSET");
  if (icharset(s, 0))
    return;

  /*
   * LESSCHARSET is not defined: try LESSCHARDEF.
   */
  s = decode::lgetenv((char*)"LESSCHARDEF");
  if (!decode::isnullenv(s)) {
    ichardef(s);
    return;
  }

#if HAVE_LOCALE
#ifdef CODESET
  /*
   * Try using the codeset name as the charset name.
   */
  s = nl_langinfo(CODESET);
  if (icharset(s, 1))
    return;
#endif
#endif

  /*
   * Check whether LC_ALL, LC_CTYPE or LANG look like UTF-8 is used.
   */
  if ((s = decode::lgetenv((char*)"LC_ALL")) != NULL || (s = decode::lgetenv((char*)"LC_CTYPE")) != NULL || (s = decode::lgetenv((char*)"LANG")) != NULL) {
    if (strstr(s, "UTF-8") != NULL || strstr(s, "utf-8") != NULL
        || strstr(s, "UTF8") != NULL || strstr(s, "utf8") != NULL)
      if (icharset((char*)"utf-8", 1))
        return;
  }

  /*
   * Get character definitions from locale functions,
   * rather than from predefined charset entry.
   */
  ilocale();
  /*
   * Default to "latin1".
   */
  (void)icharset((char*)"latin1", 1);
}

/*
 * Initialize charset data structures.
 */

void init_charset(void)
{
  char* s;

#if HAVE_LOCALE
  ignore_result(setlocale(LC_ALL, ""));
#endif

  set_charset();

  s = decode::lgetenv((char*)"LESSBINFMT");
  setfmt(s, &binfmt, &less::Globals::binattr, (char*)"*s<%02X>");

  s = decode::lgetenv((char*)"LESSUTFBINFMT");
  setfmt(s, &utfbinfmt, &less::Globals::binattr, (char*)"<U+%04lX>");
}

/*
 * Is a given character a "binary" character?
 */

int binary_char(lwchar_t c)
{
  if (less::Globals::utf_mode)
    return (is_ubin_char(c));
  c &= 0377;
  return (chardef[c] & IS_BINARY_CHAR);
}

/*
 * Is a given character a "control" character?
 */

int control_char(lwchar_t c)
{
  c &= 0377;
  return (chardef[c] & IS_CONTROL_CHAR);
}

/*
 * Return the printable form of a character.
 * For example, in the "ascii" charset '\3' is printed as "^C".
 */

char* prchar(lwchar_t c)
{
  /* {{ This buffer can be overrun if LESSBINFMT is a long string. }} */
  static char buf[32];

  c &= 0377;
  if ((c < 128 || !less::Globals::utf_mode) && !control_char(c))
    ignore_result(snprintf(buf, sizeof(buf), "%c", (int)c));
  else if (c == esc)
    strcpy(buf, "ESC");

  else if (c < 128 && !control_char(c ^ 0100))
    ignore_result(snprintf(buf, sizeof(buf), "^%c", (int)(c ^ 0100)));

  else
    ignore_result(snprintf(buf, sizeof(buf), binfmt, c));
  return (buf);
}

/*
 * Return the printable form of a UTF-8 character.
 */

char* prutfchar(lwchar_t ch)
{
  static char buf[32];

  if (ch == esc)
    strcpy(buf, "ESC");
  else if (ch < 128 && control_char(ch)) {
    if (!control_char(ch ^ 0100))
      ignore_result(snprintf(buf, sizeof(buf), "^%c", ((char)ch) ^ 0100));
    else
      ignore_result(snprintf(buf, sizeof(buf), binfmt, (char)ch));
  } else if (is_ubin_char(ch)) {
    ignore_result(snprintf(buf, sizeof(buf), utfbinfmt, ch));
  } else {
    char* p = buf;
    if (ch >= 0x80000000)
      ch = 0xFFFD; /* REPLACEMENT CHARACTER */
    put_wchar(&p, ch);
    *p = '\0';
  }
  return (buf);
}

/*
 * Get the length of a UTF-8 character in bytes.
 */

int utf_len(int ch)
{
  if ((ch & 0x80) == 0)
    return 1;
  if ((ch & 0xE0) == 0xC0)
    return 2;
  if ((ch & 0xF0) == 0xE0)
    return 3;
  if ((ch & 0xF8) == 0xF0)
    return 4;
  if ((ch & 0xFC) == 0xF8)
    return 5;
  if ((ch & 0xFE) == 0xFC)
    return 6;
  /* Invalid UTF-8 encoding. */
  return 1;
}

/*
 * Does the parameter point to the lead byte of a well-formed UTF-8 character?
 */

int is_utf8_well_formed(char* ss, int slen)
{
  int            i;
  int            len;
  unsigned char* s = (unsigned char*)ss;

  if (IS_UTF8_INVALID(s[0]))
    return (0);

  len = utf_len(s[0]);
  if (len > slen)
    return (0);
  if (len == 1)
    return (1);
  if (len == 2) {
    if (s[0] < 0xC2)
      return (0);
  } else {
    unsigned char mask;
    mask = (~((1 << (8 - len)) - 1)) & 0xFF;
    if (s[0] == mask && (s[1] & mask) == 0x80)
      return (0);
  }

  for (i = 1; i < len; i++)
    if (!IS_UTF8_TRAIL(s[i]))
      return (0);
  return (1);
}

/*
 * Skip bytes until a UTF-8 lead byte (11xxxxxx) or ASCII byte (0xxxxxxx) is found.
 */

void utf_skip_to_lead(char** pp, char* limit)
{
  do {
    ++(*pp);
  } while (*pp < limit && !IS_UTF8_LEAD((*pp)[0] & 0377) && !IS_ASCII_OCTET((*pp)[0]));
}

/*
 * Get the value of a UTF-8 character.
 */

lwchar_t get_wchar(const char* p)
{
  switch (utf_len(p[0])) {
  case 1:
  default:
    /* 0xxxxxxx */
    return (lwchar_t)(p[0] & 0xFF);
  case 2:
    /* 110xxxxx 10xxxxxx */
    return (lwchar_t)(((p[0] & 0x1F) << 6) | (p[1] & 0x3F));
  case 3:
    /* 1110xxxx 10xxxxxx 10xxxxxx */
    return (lwchar_t)(((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F));
  case 4:
    /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    return (lwchar_t)(((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F));
  case 5:
    /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
    return (lwchar_t)(((p[0] & 0x03) << 24) | ((p[1] & 0x3F) << 18) | ((p[2] & 0x3F) << 12) | ((p[3] & 0x3F) << 6) | (p[4] & 0x3F));
  case 6:
    /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
    return (lwchar_t)(((p[0] & 0x01) << 30) | ((p[1] & 0x3F) << 24) | ((p[2] & 0x3F) << 18) | ((p[3] & 0x3F) << 12) | ((p[4] & 0x3F) << 6) | (p[5] & 0x3F));
  }
}

/*
 * Store a character into a UTF-8 string.
 */

void put_wchar(char** pp, lwchar_t ch)
{
  if (!less::Globals::utf_mode || ch < 0x80) {
    /* 0xxxxxxx */
    *(*pp)++ = (char)ch;
  } else if (ch < 0x800) {
    /* 110xxxxx 10xxxxxx */
    *(*pp)++ = (char)(0xC0 | ((ch >> 6) & 0x1F));
    *(*pp)++ = (char)(0x80 | (ch & 0x3F));
  } else if (ch < 0x10000) {
    /* 1110xxxx 10xxxxxx 10xxxxxx */
    *(*pp)++ = (char)(0xE0 | ((ch >> 12) & 0x0F));
    *(*pp)++ = (char)(0x80 | ((ch >> 6) & 0x3F));
    *(*pp)++ = (char)(0x80 | (ch & 0x3F));
  } else if (ch < 0x200000) {
    /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    *(*pp)++ = (char)(0xF0 | ((ch >> 18) & 0x07));
    *(*pp)++ = (char)(0x80 | ((ch >> 12) & 0x3F));
    *(*pp)++ = (char)(0x80 | ((ch >> 6) & 0x3F));
    *(*pp)++ = (char)(0x80 | (ch & 0x3F));
  } else if (ch < 0x4000000) {
    /* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
    *(*pp)++ = (char)(0xF0 | ((ch >> 24) & 0x03));
    *(*pp)++ = (char)(0x80 | ((ch >> 18) & 0x3F));
    *(*pp)++ = (char)(0x80 | ((ch >> 12) & 0x3F));
    *(*pp)++ = (char)(0x80 | ((ch >> 6) & 0x3F));
    *(*pp)++ = (char)(0x80 | (ch & 0x3F));
  } else {
    /* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
    *(*pp)++ = (char)(0xF0 | ((ch >> 30) & 0x01));
    *(*pp)++ = (char)(0x80 | ((ch >> 24) & 0x3F));
    *(*pp)++ = (char)(0x80 | ((ch >> 18) & 0x3F));
    *(*pp)++ = (char)(0x80 | ((ch >> 12) & 0x3F));
    *(*pp)++ = (char)(0x80 | ((ch >> 6) & 0x3F));
    *(*pp)++ = (char)(0x80 | (ch & 0x3F));
  }
}

/*
 * Step forward or backward one character in a string.
 */

lwchar_t step_char(
    char**      pp,
    signed int  dir,
    const char* limit)
{
  lwchar_t ch;
  int      len;
  char*    p = *pp;

  if (!less::Globals::utf_mode) {
    /* It's easy if chars are one byte. */
    if (dir > 0)
      ch = (lwchar_t)(unsigned char)((p < limit) ? *p++ : 0);
    else
      ch = (lwchar_t)(unsigned char)((p > limit) ? *--p : 0);
  } else if (dir > 0) {
    len = utf_len(*p);
    if (p + len > limit) {
      ch = 0;
      p  = (char*)limit;
    } else {
      ch = get_wchar(p);
      p += len;
    }
  } else {
    while (p > limit && IS_UTF8_TRAIL(p[-1]))
      p--;
    if (p > limit)
      ch = get_wchar(--p);
    else
      ch = 0;
  }
  *pp = p;
  return ch;
}

/*
 * Unicode characters data
 * Actual data is in the generated *.uni files.
 */

#define DECLARE_RANGE_TABLE_START(name) \
  static struct wchar_range name##_array[] = {
#define DECLARE_RANGE_TABLE_END(name) \
  }                                   \
  ;                                   \
  struct wchar_range_table name##_table = { name##_array, sizeof(name##_array) / sizeof(*name##_array) };

DECLARE_RANGE_TABLE_START(compose)
#include "compose.uni"
DECLARE_RANGE_TABLE_END(compose)

DECLARE_RANGE_TABLE_START(ubin)
#include "ubin.uni"
DECLARE_RANGE_TABLE_END(ubin)

DECLARE_RANGE_TABLE_START(wide)
#include "wide.uni"
DECLARE_RANGE_TABLE_END(wide)

DECLARE_RANGE_TABLE_START(fmt)
#include "fmt.uni"
DECLARE_RANGE_TABLE_END(fmt)

/* comb_table is special pairs, not ranges. */
static struct wchar_range comb_table[] = {
  { 0x0644, 0x0622 },
  { 0x0644, 0x0623 },
  { 0x0644, 0x0625 },
  { 0x0644, 0x0627 },
};

static int is_in_table(lwchar_t ch,
    struct wchar_range_table*   table)
{
  int hi;
  int lo;

  /* Binary search in the table. */
  if (ch < table->table[0].first)
    return 0;
  lo = 0;
  hi = table->count - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    if (ch > table->table[mid].last)
      lo = mid + 1;
    else if (ch < table->table[mid].first)
      hi = mid - 1;
    else
      return 1;
  }
  return 0;
}

/*
 * Is a character a UTF-8 composing character?
 * If a composing character follows any char, the two combine into one glyph.
 */

int is_composing_char(lwchar_t ch)
{
  return is_in_table(ch, &compose_table) || (bs_mode != BS_CONTROL && is_in_table(ch, &fmt_table));
}

/*
 * Should this UTF-8 character be treated as binary?
 */

int is_ubin_char(lwchar_t ch)
{
  int ubin = is_in_table(ch, &ubin_table) || (bs_mode == BS_CONTROL && is_in_table(ch, &fmt_table));
  return ubin;
}

/*
 * Is this a double width UTF-8 character?
 */

int is_wide_char(lwchar_t ch)
{
  return is_in_table(ch, &wide_table);
}

/*
 * Is a character a UTF-8 combining character?
 * A combining char acts like an ordinary char, but if it follows
 * a specific char (not any char), the two combine into one glyph.
 */

int is_combining_char(lwchar_t ch1, lwchar_t ch2)
{
  /* The table is small; use linear search. */

  for (long unsigned int i = 0; i < sizeof(comb_table) / sizeof(*comb_table); i++) {
    if (ch1 == comb_table[i].first && ch2 == comb_table[i].last)
      return 1;
  }
  return 0;
}

}; // namespace charset