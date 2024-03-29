/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Routines to manipulate the "line buffer".
 * The line buffer holds a line of output as it is being built
 * in preparation for output to the screen.
 */

#include "line.hpp"
#include "ch.hpp"
#include "charset.hpp"
#include "decode.hpp"
#include "input.hpp"
#include "less.hpp"
#include "linenum.hpp"
#include "mark.hpp"
#include "option.hpp"
#include "position.hpp"
#include "screen.hpp"
#include "search.hpp"
#include "utils.hpp"

// TODO: move to namespaces
static char*      linebuf      = NULL;           /* Buffer which holds the current output line */
static char*      attr         = NULL;           /* Extension of linebuf to hold attributes */
int               size_linebuf = 0;              /* Size of line buffer (and attr buffer) */
static int        cshift;                        /* Current left-shift of output line buffer */
int               hshift;                        /* Desired left-shift of output line buffer */
int               tabstops[TABSTOP_MAX] = { 0 }; /* Custom tabstops */
int               ntabstops             = 1;     /* Number of tabstops */
int               tabdefault            = 8;     /* Default repeated tabstops */
position_t        highest_hilite;                /* Pos of last hilite in file found so far */
static int        curr;                          /* Index into linebuf */
static int        column;                        /* Printable length, accounting for backspaces, etc. */
static int        right_curr;
static int        right_column;
static int        is_null_line; /* There is no current line */
static int        lmargin;      /* Left margin */
static int        overstrike;   /* Next char should overstrike previous char */
static int        last_overstrike = AT_NORMAL;
static lwchar_t   pendc;
static position_t pendpos;
static char*      end_ansi_chars;
static char*      mid_ansi_chars;

extern int bs_mode;
extern int linenums;
extern int ctldisp;
extern int twiddle;
extern int status_col;
extern int auto_wrap, ignaw;
extern int bo_s_width, bo_e_width;
extern int ul_s_width, ul_e_width;
extern int bl_s_width, bl_e_width;
extern int so_s_width, so_e_width;
extern int sc_width, sc_height;

extern position_t start_attnpos;
extern position_t end_attnpos;
extern char       rscroll_char;
extern int        rscroll_attr;

namespace line {

static int attr_swidth(int a);
static int attr_ewidth(int a);
static int do_append(lwchar_t ch, char* rep, position_t pos);

static char       mbc_buf[MAX_UTF_CHAR_LEN];
static int        mbc_buf_len   = 0;
static int        mbc_buf_index = 0;
static position_t mbc_pos;

/*
 * Initialize from environment variables.
 */

void init_line(void)
{
  end_ansi_chars = decode::lgetenv((char*)"LESSANSIENDCHARS");
  if (decode::isnullenv(end_ansi_chars))
    end_ansi_chars = (char*)"m";

  mid_ansi_chars = decode::lgetenv((char*)"LESSANSIMIDCHARS");
  if (decode::isnullenv(mid_ansi_chars))
    mid_ansi_chars = (char*)"0123456789:;[?!\"'#%()*+ ";

  linebuf      = (char*)utils::ecalloc(LINEBUF_SIZE, sizeof(char));
  attr         = (char*)utils::ecalloc(LINEBUF_SIZE, sizeof(char));
  size_linebuf = LINEBUF_SIZE;
}

/*
 * Expand the line buffer.
 */
static int expand_linebuf(void)
{
  /* Double the size of the line buffer. */
  int new_size = size_linebuf * 2;

  /* Just realloc to expand the buffer, if we can. */
#if HAVE_REALLOC
  char* new_buf  = (char*)realloc(linebuf, new_size);
  char* new_attr = (char*)realloc(attr, new_size);
#else
  char* new_buf  = (char*)calloc(new_size, sizeof(char));
  char* new_attr = (char*)calloc(new_size, sizeof(char));
#endif
  if (new_buf == NULL || new_attr == NULL) {
    if (new_attr != NULL)
      free(new_attr);
    if (new_buf != NULL)
      free(new_buf);
    return 1;
  }
#if !HAVE_REALLOC
  /*
   * We just calloc'd the buffers; copy the old contents.
   */
  memcpy(new_buf, linebuf, size_linebuf * sizeof(char));
  memcpy(new_attr, attr, size_linebuf * sizeof(char));
  free(attr);
  free(linebuf);
#endif
  linebuf      = new_buf;
  attr         = new_attr;
  size_linebuf = new_size;
  return 0;
}

/*
 * Is a character ASCII?
 */

int is_ascii_char(lwchar_t ch)
{
  return (ch <= 0x7F);
}

/*
 * Rewind the line buffer.
 */

void prewind(void)
{
  curr            = 0;
  column          = 0;
  right_curr      = 0;
  right_column    = 0;
  cshift          = 0;
  overstrike      = 0;
  last_overstrike = AT_NORMAL;
  mbc_buf_len     = 0;
  is_null_line    = 0;
  pendc           = '\0';
  lmargin         = 0;
  if (status_col)
    lmargin += 2;
}

/*
 * Set a character in the line buffer.
 */
static void set_linebuf(int n, char ch, char a)
{
  linebuf[n] = ch;
  attr[n]    = a;
}

/*
 * Append a character to the line buffer.
 */
static void add_linebuf(char ch, char a, int w)
{
  set_linebuf(curr++, ch, a);
  column += w;
}

/*
 * Insert the line number (of the given position) into the line buffer.
 */

void plinenum(position_t pos)
{
  linenum_t linenum = 0;
  int       i;

  if (linenums == option::OPT_ONPLUS) {
    /*
     * Get the line number and put it in the current line.
     * {{ Note: since linenum::find_linenum calls forw_raw_line,
     *    it may seek in the input file, requiring the caller
     *    of plinenum to re-seek if necessary. }}
     * {{ Since forw_raw_line modifies linebuf, we must
     *    do this first, before storing anything in linebuf. }}
     */
    linenum = linenum::find_linenum(pos);
  }

  /*
   * Display a status column if the -J option is set.
   */
  if (status_col) {
    int  a = AT_NORMAL;
    char c = mark::posmark(pos);
    if (c != 0)
      a |= AT_HILITE;
    else {
      c = ' ';
      if (start_attnpos != NULL_POSITION && pos >= start_attnpos && pos <= end_attnpos)
        a |= AT_HILITE;
    }
    add_linebuf(c, a, 1);           /* column 0: status */
    add_linebuf(' ', AT_NORMAL, 1); /* column 1: empty */
  }

  /*
   * Display the line number at the start of each line
   * if the -N option is set.
   */
  if (linenums == option::OPT_ONPLUS) {

    int   bufLength = utils::strlen_bound<linenum_t>();
    char* bufPtr    = new char[bufLength];

    utils::typeToStr<linenum_t>(linenum, bufPtr, bufLength);

    int numDigits = (int)strlen(bufPtr);
    int pad       = 0;

    if (numDigits < MIN_LINENUM_WIDTH)
      pad = MIN_LINENUM_WIDTH - numDigits;

    for (i = 0; i < pad; i++)
      add_linebuf(utils::CH_SPACE, AT_NORMAL, 1);

    for (i = 0; i < numDigits; i++)
      add_linebuf(bufPtr[i], AT_BOLD, 1);

    add_linebuf(utils::CH_SPACE, AT_NORMAL, 1);
    lmargin += numDigits + pad + 1;

    delete[] bufPtr;
  }
  /*
   * Append enough spaces to bring us to the lmargin.
   */
  while (column < lmargin) {
    add_linebuf(' ', AT_NORMAL, 1);
  }
}

/*
 * Shift the input line left.
 * This means discarding N printable chars at the start of the buffer.
 */
static void pshift(int shift)
{
  lwchar_t      prev_ch = 0;
  unsigned char c;
  int           shifted = 0;
  int           to;
  int           from;
  int           len;
  int           width;
  int           prev_attr;
  int           next_attr;

  if (shift > column - lmargin)
    shift = column - lmargin;
  if (shift > curr - lmargin)
    shift = curr - lmargin;

  to = from = lmargin;
  /*
   * We keep on going when shifted == shift
   * to get all combining chars.
   */
  while (shifted <= shift && from < curr) {
    c = linebuf[from];
    if (ctldisp == option::OPT_ONPLUS && is_csi_start(c)) {
      /* Keep cumulative effect.  */
      linebuf[to] = c;
      attr[to++]  = attr[from++];
      while (from < curr && linebuf[from]) {
        linebuf[to] = linebuf[from];
        attr[to++]  = attr[from];
        if (!is_ansi_middle(linebuf[from++]))
          break;
      }
      continue;
    }

    width = 0;

    if (!IS_ASCII_OCTET(c) && less::Globals::utf_mode) {
      /* Assumes well-formedness validation already done.  */
      lwchar_t ch;

      len = charset::utf_len(c);
      if (from + len > curr)
        break;
      ch = charset::get_wchar(linebuf + from);
      if (!charset::is_composing_char(ch) && !charset::is_combining_char(prev_ch, ch))
        width = charset::is_wide_char(ch) ? 2 : 1;
      prev_ch = ch;
    } else {
      len = 1;
      if (c == '\b')
        /* XXX - Incorrect if several '\b' in a row.  */
        width = (less::Globals::utf_mode && charset::is_wide_char(prev_ch)) ? -2 : -1;
      else if (!charset::control_char(c))
        width = 1;
      prev_ch = 0;
    }

    if (width == 2 && shift - shifted == 1) {
      /* Should never happen when called by pshift_all().  */
      attr[to] = attr[from];
      /*
       * Assume a wide_char will never be the first half of a
       * combining_char pair, so reset prev_ch in case we're
       * followed by a '\b'.
       */
      prev_ch = static_cast<unsigned char>(linebuf[to++] = ' ');
      from += len;
      shifted++;
      continue;
    }

    /* Adjust width for magic cookies. */
    prev_attr = (to > 0) ? attr[to - 1] : AT_NORMAL;
    next_attr = (from + len < curr) ? attr[from + len] : prev_attr;
    if (!screen::is_at_equiv(attr[from], prev_attr) && !screen::is_at_equiv(attr[from], next_attr)) {
      width += attr_swidth(attr[from]);
      if (from + len < curr)
        width += attr_ewidth(attr[from]);
      if (screen::is_at_equiv(prev_attr, next_attr)) {
        width += attr_ewidth(prev_attr);
        if (from + len < curr)
          width += attr_swidth(next_attr);
      }
    }

    if (shift - shifted < width)
      break;
    from += len;
    shifted += width;
    if (shifted < 0)
      shifted = 0;
  }
  while (from < curr) {
    linebuf[to] = linebuf[from];
    attr[to++]  = attr[from++];
  }
  curr = to;
  column -= shifted;
  cshift += shifted;
}

/*
 *
 */

void pshift_all(void)
{
  pshift(column);
}

/*
 * Return the printing width of the start (enter) sequence
 * for a given character attribute.
 */
static int attr_swidth(int a)
{
  int w = 0;

  a = screen::apply_at_specials(a);

  if (a & AT_UNDERLINE)
    w += ul_s_width;
  if (a & AT_BOLD)
    w += bo_s_width;
  if (a & AT_BLINK)
    w += bl_s_width;
  if (a & AT_STANDOUT)
    w += so_s_width;

  return w;
}

/*
 * Return the printing width of the end (exit) sequence
 * for a given character attribute.
 */
static int attr_ewidth(int a)
{
  int w = 0;

  a = screen::apply_at_specials(a);

  if (a & AT_UNDERLINE)
    w += ul_e_width;
  if (a & AT_BOLD)
    w += bo_e_width;
  if (a & AT_BLINK)
    w += bl_e_width;
  if (a & AT_STANDOUT)
    w += so_e_width;

  return w;
}

/*
 * Return the printing width of a given character and attribute,
 * if the character were added to the current position in the line buffer.
 * Adding a character with a given attribute may cause an enter or exit
 * attribute sequence to be inserted, so this must be taken into account.
 */
static int pwidth(lwchar_t ch, int a, lwchar_t prev_ch)
{
  int w;

  if (ch == '\b')
    /*
     * Backspace moves backwards one or two positions.
     * XXX - Incorrect if several '\b' in a row.
     */
    return (less::Globals::utf_mode && charset::is_wide_char(prev_ch)) ? -2 : -1;

  if (!less::Globals::utf_mode || is_ascii_char(ch)) {
    if (charset::control_char((char)ch)) {
      /*
       * Control characters do unpredictable things,
       * so we don't even try to guess; say it doesn't move.
       * This can only happen if the -r flag is in effect.
       */
      return (0);
    }
  } else {
    if (charset::is_composing_char(ch) || charset::is_combining_char(prev_ch, ch)) {
      /*
       * Composing and combining chars take up no space.
       *
       * Some terminals, upon failure to compose a
       * composing character with the character(s) that
       * precede(s) it will actually take up one column
       * for the composing character; there isn't much
       * we could do short of testing the (complex)
       * composition process ourselves and printing
       * a binary representation when it fails.
       */
      return (0);
    }
  }

  /*
   * Other characters take one or two columns,
   * plus the width of any attribute enter/exit sequence.
   */
  w = 1;
  if (charset::is_wide_char(ch))
    w++;
  if (curr > 0 && !screen::is_at_equiv(attr[curr - 1], a))
    w += attr_ewidth(attr[curr - 1]);
  if ((screen::apply_at_specials(a) != AT_NORMAL) && (curr == 0 || !screen::is_at_equiv(attr[curr - 1], a)))
    w += attr_swidth(a);
  return (w);
}

/*
 * Delete to the previous base character in the line buffer.
 * Return 1 if one is found.
 */
static int backc(void)
{
  lwchar_t prev_ch;
  char*    p  = linebuf + curr;
  lwchar_t ch = charset::step_char(&p, -1, linebuf + lmargin);
  int      width;

  /* This assumes that there is no '\b' in linebuf.  */
  while (curr > lmargin && column > lmargin && (!(attr[curr - 1] & (AT_ANSI | AT_BINARY)))) {
    curr    = (int)(p - linebuf);
    prev_ch = charset::step_char(&p, -1, linebuf + lmargin);
    width   = pwidth(ch, attr[curr], prev_ch);
    column -= width;
    if (width > 0)
      return 1;
    ch = prev_ch;
  }

  return 0;
}

/*
 * Are we currently within a recognized ANSI escape sequence?
 */
static int in_ansi_esc_seq(void)
{
  char* p;

  /*
   * Search backwards for either an ESC (which means we ARE in a seq);
   * or an end char (which means we're NOT in a seq).
   */
  for (p = &linebuf[curr]; p > linebuf;) {
    lwchar_t ch = charset::step_char(&p, -1, linebuf);
    if (is_csi_start(ch))
      return (1);
    if (!is_ansi_middle(ch))
      return (0);
  }
  return (0);
}

/*
 * Is a character the end of an ANSI escape sequence?
 */

int is_ansi_end(lwchar_t ch)
{
  if (!is_ascii_char(ch))
    return (0);
  return (strchr(end_ansi_chars, (char)ch) != NULL);
}

/*
 * Can a char appear in an ANSI escape sequence, before the end char?
 */

int is_ansi_middle(lwchar_t ch)
{
  if (!is_ascii_char(ch))
    return (0);
  if (is_ansi_end(ch))
    return (0);
  return (strchr(mid_ansi_chars, (char)ch) != NULL);
}

/*
 * Skip past an ANSI escape sequence.
 * pp is initially positioned just after the CSI_START char.
 */

void skip_ansi(char** pp, const char* limit)
{
  lwchar_t c;
  do {
    c = charset::step_char(pp, +1, limit);
  } while (*pp < limit && is_ansi_middle(c));
  /* Note that we discard final char, for which is_ansi_middle is false. */
}

/*
 * Append a character and attribute to the line buffer.
 */
#define STORE_CHAR(ch, a, rep, pos)          \
  do {                                       \
    if (store_char((ch), (a), (rep), (pos))) \
      return (1);                            \
  } while (0)

static int store_char(lwchar_t ch, int a, char* rep, position_t pos)
{
  int  w;
  int  replen;
  char cs;

  w = (a & (AT_UNDERLINE | AT_BOLD)); /* Pre-use w.  */
  if (w != AT_NORMAL)
    last_overstrike = w;

#if HILITE_SEARCH
  {
    int matches;
    if (search::is_hilited(pos, pos + 1, 0, &matches)) {
      /*
       * This character should be highlighted.
       * Override the attribute passed in.
       */
      if (a != AT_ANSI) {
        if (highest_hilite != NULL_POSITION && pos > highest_hilite)
          highest_hilite = pos;
        a |= AT_HILITE;
      }
    }
  }
#endif

  if (ctldisp == option::OPT_ONPLUS && in_ansi_esc_seq()) {
    if (!is_ansi_end(ch) && !is_ansi_middle(ch)) {
      /* Remove whole unrecognized sequence.  */
      char*    p = &linebuf[curr];
      lwchar_t bch;
      do {
        bch = charset::step_char(&p, -1, linebuf);
      } while (p > linebuf && !is_csi_start(bch));
      curr = (int)(p - linebuf);
      return 0;
    }
    a = AT_ANSI; /* Will force re-AT_'ing around it.  */
    w = 0;
  } else if (ctldisp == option::OPT_ONPLUS && is_csi_start(ch)) {
    a = AT_ANSI; /* Will force re-AT_'ing around it.  */
    w = 0;
  } else {
    char*    p       = &linebuf[curr];
    lwchar_t prev_ch = charset::step_char(&p, -1, linebuf);
    w                = pwidth(ch, a, prev_ch);
  }

  if (ctldisp != option::OPT_ON && column + w + attr_ewidth(a) > sc_width)
    /*
     * Won't fit on screen.
     */
    return (1);

  if (rep == NULL) {
    cs     = (char)ch;
    rep    = &cs;
    replen = 1;
  } else {
    replen = charset::utf_len(rep[0]);
  }
  if (curr + replen >= size_linebuf - 6) {
    /*
     * Won't fit in line buffer.
     * Try to expand it.
     */
    if (expand_linebuf())
      return (1);
  }

  if (column > right_column && w > 0) {
    right_column = column;
    right_curr   = curr;
  }

  while (replen-- > 0) {
    add_linebuf(*rep++, a, 0);
  }
  column += w;
  return (0);
}

/*
 * Append a tab to the line buffer.
 * Store spaces to represent the tab.
 */
#define STORE_TAB(a, pos)      \
  do {                         \
    if (store_tab((a), (pos))) \
      return (1);              \
  } while (0)

static int store_tab(int attr, position_t pos)
{
  int to_tab = column + cshift - lmargin;
  int i;

  if (ntabstops < 2 || to_tab >= tabstops[ntabstops - 1])
    to_tab = tabdefault - ((to_tab - tabstops[ntabstops - 1]) % tabdefault);
  else {
    for (i = ntabstops - 2; i >= 0; i--)
      if (to_tab >= tabstops[i])
        break;
    to_tab = tabstops[i + 1] - to_tab;
  }

  if (column + to_tab - 1 + pwidth(' ', attr, 0) + attr_ewidth(attr) > sc_width)
    return 1;

  do {
    STORE_CHAR(' ', attr, (char*)" ", pos);
  } while (--to_tab > 0);
  return 0;
}

#define STORE_PRCHAR(c, pos)      \
  do {                            \
    if (store_prchar((c), (pos))) \
      return 1;                   \
  } while (0)

static int store_prchar(lwchar_t c, position_t pos)
{
  char* s;

  /*
   * Convert to printable representation.
   */
  s = charset::prchar(c);

  /*
   * Make sure we can get the entire representation
   * of the character on this line.
   */
  if (column + (int)strlen(s) - 1 + pwidth(' ', less::Globals::binattr, 0) + attr_ewidth(less::Globals::binattr) > sc_width)
    return 1;

  for (; *s != 0; s++)
    STORE_CHAR(*s, AT_BINARY, NULL, pos);

  return 0;
}

static int flush_mbc_buf(position_t pos)
{
  int i;

  for (i = 0; i < mbc_buf_index; i++)
    if (store_prchar(mbc_buf[i], pos))
      return mbc_buf_index - i;

  return 0;
}

/*
 * Append a character to the line buffer.
 * Expand tabs into spaces, handle underlining, boldfacing, etc.
 * Returns 0 if ok, 1 if couldn't fit in buffer.
 */

int pappend(int c, position_t pos)
{
  int r;

  if (pendc) {
    if (c == '\r' && pendc == '\r')
      return (0);
    if (do_append(pendc, NULL, pendpos))
      /*
       * Oops.  We've probably lost the char which
       * was in pendc, since caller won't back up.
       */
      return (1);
    pendc = '\0';
  }

  if (c == '\r' && bs_mode == BS_SPECIAL) {
    if (mbc_buf_len > 0) /* utf_mode must be on. */
    {
      /* Flush incomplete (truncated) sequence. */
      r             = flush_mbc_buf(mbc_pos);
      mbc_buf_index = r + 1;
      mbc_buf_len   = 0;
      if (r)
        return (mbc_buf_index);
    }

    /*
     * Don't put the CR into the buffer until we see
     * the next char.  If the next char is a newline,
     * discard the CR.
     */
    pendc   = c;
    pendpos = pos;
    return (0);
  }

  if (!less::Globals::utf_mode) {
    r = do_append(c, NULL, pos);
  } else {
    /* Perform strict validation in all possible cases. */
    if (mbc_buf_len == 0) {
    retry:
      mbc_buf_index = 1;
      *mbc_buf      = c;
      if (IS_ASCII_OCTET(c))
        r = do_append(c, NULL, pos);
      else if (IS_UTF8_LEAD(c)) {
        mbc_buf_len = charset::utf_len(c);
        mbc_pos     = pos;
        return (0);
      } else
        /* UTF8_INVALID or stray UTF8_TRAIL */
        r = flush_mbc_buf(pos);
    } else if (IS_UTF8_TRAIL(c)) {
      mbc_buf[mbc_buf_index++] = c;
      if (mbc_buf_index < mbc_buf_len)
        return (0);
      if (charset::is_utf8_well_formed(mbc_buf, mbc_buf_index))
        r = do_append(charset::get_wchar(mbc_buf), mbc_buf, mbc_pos);
      else
        /* Complete, but not shortest form, sequence. */
        mbc_buf_index = r = flush_mbc_buf(mbc_pos);
      mbc_buf_len = 0;
    } else {
      /* Flush incomplete (truncated) sequence.  */
      r             = flush_mbc_buf(mbc_pos);
      mbc_buf_index = r + 1;
      mbc_buf_len   = 0;
      /* Handle new char.  */
      if (!r)
        goto retry;
    }
  }

  /*
   * If we need to shift the line, do it.
   * But wait until we get to at least the middle of the screen,
   * so shifting it doesn't affect the chars we're currently
   * pappending.  (Bold & underline can get messed up otherwise.)
   */
  if (cshift < hshift && column > sc_width / 2) {
    linebuf[curr] = '\0';
    pshift(hshift - cshift);
  }
  if (r) {
    /* How many chars should caller back up? */
    r = (!less::Globals::utf_mode) ? 1 : mbc_buf_index;
  }
  return (r);
}

static int do_append(lwchar_t ch, char* rep, position_t pos)
{
  int      a;
  lwchar_t prev_ch;

  a = AT_NORMAL;

  if (ch == '\b') {
    if (bs_mode == BS_CONTROL)
      goto do_control_char;

    /*
     * A better test is needed here so we don't
     * backspace over part of the printed
     * representation of a binary character.
     */
    if (curr <= lmargin || column <= lmargin || (attr[curr - 1] & (AT_ANSI | AT_BINARY)))
      STORE_PRCHAR('\b', pos);
    else if (bs_mode == BS_NORMAL)
      STORE_CHAR(ch, AT_NORMAL, NULL, pos);
    else if (bs_mode == BS_SPECIAL)
      overstrike = backc();

    return 0;
  }

  if (overstrike > 0) {
    /*
     * Overstrike the character at the current position
     * in the line buffer.  This will cause either
     * underline (if a "_" is overstruck),
     * bold (if an identical character is overstruck),
     * or just deletion of the character in the buffer.
     */
    overstrike = less::Globals::utf_mode ? -1 : 0;
    if (less::Globals::utf_mode) {
      /* To be correct, this must be a base character.  */
      prev_ch = charset::get_wchar(linebuf + curr);
    } else {
      prev_ch = (unsigned char)linebuf[curr];
    }
    a = static_cast<unsigned char>(attr[curr]);
    if (ch == prev_ch) {
      /*
       * Overstriking a char with itself means make it bold.
       * But overstriking an underscore with itself is
       * ambiguous.  It could mean make it bold, or
       * it could mean make it underlined.
       * Use the previous overstrike to resolve it.
       */
      if (ch == '_') {
        if ((a & (AT_BOLD | AT_UNDERLINE)) != AT_NORMAL)
          a |= (AT_BOLD | AT_UNDERLINE);
        else if (last_overstrike != AT_NORMAL)
          a |= last_overstrike;
        else
          a |= AT_BOLD;
      } else
        a |= AT_BOLD;
    } else if (ch == '_') {
      a |= AT_UNDERLINE;
      ch  = prev_ch;
      rep = linebuf + curr;
    } else if (prev_ch == '_') {
      a |= AT_UNDERLINE;
    }
    /* Else we replace prev_ch, but we keep its attributes.  */
  } else if (overstrike < 0) {
    if (charset::is_composing_char(ch) || charset::is_combining_char(charset::get_wchar(linebuf + curr), ch))
      /* Continuation of the same overstrike.  */
      a = last_overstrike;
    else
      overstrike = 0;
  }

  if (ch == '\t') {
    /*
     * Expand a tab into spaces.
     */
    switch (bs_mode) {
    case BS_CONTROL:
      goto do_control_char;
    case BS_NORMAL:
    case BS_SPECIAL:
      STORE_TAB(a, pos);
      break;
    }
  } else if ((!less::Globals::utf_mode || is_ascii_char(ch)) && charset::control_char((char)ch)) {
  do_control_char:
    if (ctldisp == option::OPT_ON || (ctldisp == option::OPT_ONPLUS && is_csi_start(ch))) {
      /*
       * Output as a normal character.
       */
      STORE_CHAR(ch, AT_NORMAL, rep, pos);
    } else {
      STORE_PRCHAR((char)ch, pos);
    }
  } else if (less::Globals::utf_mode && ctldisp != option::OPT_ON && charset::is_ubin_char(ch)) {
    char* s;

    s = charset::prutfchar(ch);

    if (column + (int)strlen(s) - 1 + pwidth(' ', less::Globals::binattr, 0) + attr_ewidth(less::Globals::binattr) > sc_width)
      return (1);

    for (; *s != 0; s++)
      STORE_CHAR(*s, AT_BINARY, NULL, pos);
  } else {
    STORE_CHAR(ch, a, rep, pos);
  }
  return (0);
}

/*
 *
 */

int pflushmbc(void)
{
  int r = 0;

  if (mbc_buf_len > 0) {
    /* Flush incomplete (truncated) sequence.  */
    r           = flush_mbc_buf(mbc_pos);
    mbc_buf_len = 0;
  }
  return r;
}

/*
 * Switch to normal attribute at end of line.
 */
static void add_attr_normal(void)
{
  char* p = (char*)"\033[m";

  if (ctldisp != option::OPT_ONPLUS || !is_ansi_end('m'))
    return;
  for (; *p != '\0'; p++)
    add_linebuf(*p, AT_ANSI, 0);
}

/*
 * Terminate the line in the line buffer.
 */

void pdone(bool endline, bool chopped, int forw)
{
  (void)pflushmbc();

  if (pendc && (pendc != '\r' || !endline))
    /*
     * If we had a pending character, put it in the buffer.
     * But discard a pending CR if we are at end of line
     * (that is, discard the CR in a CR/LF sequence).
     */
    (void)do_append(pendc, NULL, pendpos);

  /*
   * Make sure we've shifted the line, if we need to.
   */
  if (cshift < hshift)
    pshift(hshift - cshift);

  if (chopped && rscroll_char) {
    /*
     * Display the right scrolling char.
     * If we've already filled the rightmost screen char
     * (in the buffer), overwrite it.
     */
    if (column >= sc_width) {
      /* We've already written in the rightmost char. */
      column = right_column;
      curr   = right_curr;
    }
    add_attr_normal();
    while (column < sc_width - 1) {
      /*
       * Space to last (rightmost) char on screen.
       * This may be necessary if the char we overwrote
       * was double-width.
       */
      add_linebuf(' ', AT_NORMAL, 1);
    }
    /* Print rscroll char. It must be single-width. */
    add_linebuf(rscroll_char, rscroll_attr, 1);
  } else {
    add_attr_normal();
  }

  debug::debug("add newline if necessary");
  /*
   * Add a newline if necessary,
   * and append a '\0' to the end of the line.
   * We output a newline if we're not at the right edge of the screen,
   * or if the terminal doesn't auto wrap,
   * or if this is really the end of the line AND the terminal ignores
   * a newline at the right edge.
   * (In the last case we don't want to output a newline if the terminal
   * doesn't ignore it since that would produce an extra blank line.
   * But we do want to output a newline if the terminal ignores it in case
   * the next line is blank.  In that case the single newline output for
   * that blank line would be ignored!)
   */
  if (column < sc_width || !auto_wrap || (endline && ignaw) || ctldisp == option::OPT_ON) {
    add_linebuf('\n', AT_NORMAL, 0);
  } else if (ignaw && column >= sc_width && forw) {
    /*
     * Terminals with "ignaw" don't wrap until they *really* need
     * to, i.e. when the character *after* the last one to fit on a
     * line is output. But they are too hard to deal with when they
     * get in the state where a full screen width of characters
     * have been output but the cursor is sitting on the right edge
     * instead of at the start of the next line.
     * So we nudge them into wrapping by outputting a space
     * character plus a backspace.  But do this only if moving
     * forward; if we're moving backward and drawing this line at
     * the top of the screen, the space would overwrite the first
     * char on the next line.  We don't need to do this "nudge"
     * at the top of the screen anyway.
     */
    add_linebuf(' ', AT_NORMAL, 1);
    add_linebuf('\b', AT_NORMAL, -1);
  }
  set_linebuf(curr, '\0', AT_NORMAL);
}

/*
 *
 */

void set_status_col(int c)
{
  set_linebuf(0, c, AT_NORMAL | AT_HILITE);
}

/*
 * Get a character from the current line.
 * Return the character as the function return value,
 * and the character attribute in *ap.
 */

int gline(int i, int* ap)
{
  if (is_null_line) {
    /*
     * If there is no current line, we pretend the line is
     * either "~" or "", depending on the "twiddle" flag.
     */
    if (twiddle) {
      if (i == 0) {
        *ap = AT_BOLD;
        return '~';
      }
      --i;
    }
    /* Make sure we're back to AT_NORMAL before the '\n'.  */
    *ap = AT_NORMAL;
    return i ? '\0' : '\n';
  }

  *ap = static_cast<unsigned char>(attr[i]);
  return (linebuf[i] & 0xFF);
}

/*
 * Indicate that there is no current line.
 */

void null_line(void)
{
  is_null_line = 1;
  cshift       = 0;
}

/*
 * Analogous to forw_line(), but deals with "raw lines":
 * lines which are not split for screen width.
 * {{ This is supposed to be more efficient than forw_line(). }}
 */

position_t forw_raw_line(position_t curr_pos, char** linep, int* line_lenp)
{
  int        n;
  int        c;
  position_t new_pos;

  if (curr_pos == NULL_POSITION || ch::seek(curr_pos) || (c = ch::forw_get()) == EOI)
    return (NULL_POSITION);

  n = 0;
  for (;;) {
    if (c == '\n' || c == EOI || is_abort_signal(less::Globals::sigs)) {
      new_pos = ch::tell();
      break;
    }
    if (n >= size_linebuf - 1) {
      if (expand_linebuf()) {
        /*
         * Overflowed the input buffer.
         * Pretend the line ended here.
         */
        new_pos = ch::tell() - 1;
        break;
      }
    }
    linebuf[n++] = c;
    c            = ch::forw_get();
  }
  linebuf[n] = '\0';
  if (linep != NULL)
    *linep = linebuf;
  if (line_lenp != NULL)
    *line_lenp = n;
  return (new_pos);
}

/*
 * Analogous to back_line(), but deals with "raw lines".
 * {{ This is supposed to be more efficient than back_line(). }}
 */

position_t back_raw_line(position_t curr_pos, char** linep, int* line_lenp)
{
  int        n;
  int        c;
  position_t new_pos;

  if (curr_pos == NULL_POSITION || curr_pos <= ch_zero || ch::seek(curr_pos - 1))
    return (NULL_POSITION);

  n            = size_linebuf;
  linebuf[--n] = '\0';
  for (;;) {
    c = ch::back_get();
    if (c == '\n' || is_abort_signal(less::Globals::sigs)) {
      /*
       * This is the newline ending the previous line.
       * We have hit the beginning of the line.
       */
      new_pos = ch::tell() + 1;
      break;
    }
    if (c == EOI) {
      /*
       * We have hit the beginning of the file.
       * This must be the first line in the file.
       * This must, of course, be the beginning of the line.
       */
      new_pos = ch_zero;
      break;
    }
    if (n <= 0) {
      int   old_size_linebuf = size_linebuf;
      char* fm;
      char* to;
      if (expand_linebuf()) {
        /*
         * Overflowed the input buffer.
         * Pretend the line ended here.
         */
        new_pos = ch::tell() + 1;
        break;
      }
      /*
       * Shift the data to the end of the new linebuf.
       */
      for (fm = linebuf + old_size_linebuf - 1, to = linebuf + size_linebuf - 1; fm >= linebuf; fm--, to--)
        *to = *fm;
      n = size_linebuf - old_size_linebuf;
    }
    linebuf[--n] = c;
  }
  if (linep != NULL)
    *linep = &linebuf[n];
  if (line_lenp != NULL)
    *line_lenp = size_linebuf - 1 - n;
  return (new_pos);
}

/*
 * Find the shift necessary to show the end of the longest displayed line.
 */

int rrshift(void)
{
  position_t pos;
  int        save_width;
  int        line;
  int        longest = 0;

  save_width = sc_width;
  sc_width   = INT_MAX;
  hshift     = 0;
  pos        = position::position(TOP);
  for (line = 0; line < sc_height && pos != NULL_POSITION; line++) {
    pos = input::forw_line(pos);
    if (column > longest)
      longest = column;
  }
  sc_width = save_width;
  if (longest < sc_width)
    return 0;
  return longest - sc_width;
}

} // namespace line