/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Prompting and other messages.
 * There are three flavors of prompts, SHORT, MEDIUM and LONG,
 * selected by the -m/-M options.
 * There is also the "equals message", printed by the = command.
 * A prompt is a message composed of various pieces, such as the
 * name of the file being viewed, the os::percentage into the file, etc.
 */

#include "prompt.hpp"
#include "ch.hpp"
#include "filename.hpp"
#include "forwback.hpp"
#include "ifile.hpp"
#include "less.hpp"
#include "linenum.hpp"
#include "option.hpp"
#include "os.hpp"
#include "position.hpp"
#include "tags.hpp"
#include "utils.hpp"

extern int  pr_type;
extern bool new_file;
extern int  sc_width;
extern int  so_s_width, so_e_width;
extern int  linenums;
extern int  hshift;
extern int  sc_height;
extern int  jump_sline;
#if EDITOR
extern char* editor;
extern char* editproto;
#endif

// TODO: Move to namespaces
/*
 * Prototypes for the three flavors of prompts.
 * These strings are expanded by pr_expand().
 */
static const char s_proto[]    = "?n?f%f .?m(%T %i of %m) ..?e(END) ?x- Next\\: %x..%t";
static const char m_proto[]    = "?n?f%f .?m(%T %i of %m) ..?e(END) ?x- Next\\: %x.:?pB%pB\\%:byte %bB?s/%s...%t";
static const char M_proto[]    = "?f%f .?n?m(%T %i of %m) ..?ltlines %lt-%lb?L/%L. :byte %bB?s/%s. .?e(END) ?x- Next\\: %x.:?pB%pB\\%..%t";
static const char e_proto[]    = "?f%f .?m(%T %i of %m) .?ltlines %lt-%lb?L/%L. .byte %bB?s/%s. ?e(END) :?pB%pB\\%..%t";
static const char h_proto[]    = "HELP -- ?eEND -- Press g to see it again:Press RETURN for more., or q when done";
static const char w_proto[]    = "Waiting for data";
static const char more_proto[] = "--More--(?eEND ?x- Next\\: %x.:?pB%pB\\%:byte %bB?s/%s...%t)";

char*       prproto[3];
char const* eqproto = e_proto;
char const* hproto  = h_proto;
char const* wproto  = w_proto;

static char  message[PROMPT_SIZE];
static char* mp;

namespace prompt {

/*
 * Initialize the prompt prototype strings.
 */
void init_prompt(void)
{
  prproto[0] = utils::save(s_proto);
  prproto[1] = utils::save(option::Option::less_is_more ? more_proto : m_proto);
  prproto[2] = utils::save(M_proto);
  eqproto    = utils::save(e_proto);
  hproto     = utils::save(h_proto);
  wproto     = utils::save(w_proto);
}

/*
 * Append a string to the end of the message.
 */
static void ap_str(char* s)
{
  int len;

  len = (int)strlen(s);
  if (mp + len >= message + PROMPT_SIZE)
    len = (int)(message + PROMPT_SIZE - mp - 1);
  strcpy(mp, s);
  mp += len;
  *mp = '\0';
}

/*
 * Append a character to the end of the message.
 */
static void ap_char(char c)
{
  char buf[2];

  buf[0] = c;
  buf[1] = '\0';
  ap_str(buf);
}

/*
 * Append a position_t (as a decimal integer) to the end of the message.
 */
static void ap_pos(position_t pos)
{
  const int bufLength = utils::strlen_bound<position_t>();
  char*     bufPtr    = new char[bufLength];

  utils::typeToStr<position_t>(pos, bufPtr, bufLength);
  ap_str(bufPtr);

  delete[] bufPtr;
}

/*
 * Append a line number to the end of the message.
 */
static void ap_linenum(linenum_t linenum)
{
  const int bufLength = utils::strlen_bound<linenum_t>();
  char*     bufPtr    = new char[bufLength];

  utils::typeToStr<linenum_t>(linenum, bufPtr, bufLength);
  ap_str(bufPtr);
  delete[] bufPtr;
}

/*
 * Append an integer to the end of the message.
 */
static void ap_int(int num)
{
  const int bufLength = utils::strlen_bound<int>();

  char* bufPtr = new char[bufLength];

  utils::typeToStr<int>(num, bufPtr, bufLength);
  ap_str(bufPtr);

  delete[] bufPtr;
}

/*
 * Append a question mark to the end of the message.
 */
static void ap_quest(void)
{
  ap_str((char*)"?");
}

/*
 * Return the "current" byte offset in the file.
 */
static position_t curr_byte(int where)
{
  position_t pos;

  pos = position::position(where);
  while (pos == NULL_POSITION && where >= 0 && where < sc_height - 1)
    pos = position::position(++where);
  if (pos == NULL_POSITION)
    pos = ch::length();
  return (pos);
}

/*
 * Return the value of a prototype conditional.
 * A prototype string may include conditionals which consist of a
 * question mark followed by a single letter.
 * Here we decode that letter and return the appropriate boolean value.
 */
static int cond(char c, int where)
{
  position_t len;

  switch (c) {
  case 'a': /* Anything in the message yet? */
    return (mp > message);
  case 'b': /* Current byte offset known? */
    return (curr_byte(where) != NULL_POSITION);
  case 'c':
    return (hshift != 0);
  case 'e': /* At end of file? */
    return (forwback::eof_displayed());
  case 'f': /* Filename known? */
  case 'g':
    return (strcmp(ifile::getCurrentIfile()->getFilename(), "-") != 0);
  case 'l': /* Line number known? */
  case 'd': /* Same as l */
    if (!linenums)
      return 0;
    return (linenum::currline(where) != 0);
  case 'L': /* Final line number known? */
  case 'D': /* Final page number known? */
    return (linenums && ch::length() != NULL_POSITION);
  case 'm': /* More than one file? */
#if TAGS
    return (tags::ntags() ? (tags::ntags() > 1) : (ifile::numIfiles() > 1));
#else
    return (nifile() > 1);
#endif
  case 'n': /* First prompt in a new file? */
#if TAGS
    return (tags::ntags() ? 1 : static_cast<int>(new_file));
#else
    return (new_file);
#endif
  case 'p': /* Percent into file (bytes) known? */
    return (curr_byte(where) != NULL_POSITION && ch::length() > 0);
  case 'P': /* Percent into file (lines) known? */
    return (linenum::currline(where) != 0 && (len = ch::length()) > 0 && linenum::find_linenum(len) != 0);
  case 's': /* Size of file known? */
  case 'B':
    return (ch::length() != NULL_POSITION);
  case 'x': /* Is there a "next" file? */
#if TAGS
    if (tags::ntags())
      return (0);
#endif
    return (ifile::nextIfile(ifile::getCurrentIfile()) != nullptr);
  }
  return (0);
}

/*
 * Decode a "percent" prototype character.
 * A prototype string may include various "percent" escapes;
 * that is, a percent sign followed by a single letter.
 * Here we decode that letter and take the appropriate action,
 * usually by appending something to the message being built.
 */
static void protochar(int c, int where, int iseditproto)
{
  position_t    pos;
  position_t    len;
  int           n;
  linenum_t     linenum;
  linenum_t     last_linenum;
  ifile::Ifile* h;
  char*         s;

#undef PAGE_NUM
#define PAGE_NUM(linenum) ((((linenum)-1) / (sc_height - 1)) + 1)

  switch (c) {
  case 'b': /* Current byte offset */
    pos = curr_byte(where);
    if (pos != NULL_POSITION)
      ap_pos(pos);
    else
      ap_quest();
    break;
  case 'c':
    ap_int(hshift);
    break;
  case 'd': /* Current page number */
    linenum = linenum::currline(where);
    if (linenum > 0 && sc_height > 1)
      ap_linenum(PAGE_NUM(linenum));
    else
      ap_quest();
    break;
  case 'D': /* Final page number */
    /* Find the page number of the last byte in the file (len-1). */
    len = ch::length();
    if (len == NULL_POSITION)
      ap_quest();
    else if (len == 0)
      /* An empty file has no pages. */
      ap_linenum(0);
    else {
      linenum = linenum::find_linenum(len - 1);
      if (linenum <= 0)
        ap_quest();
      else
        ap_linenum(PAGE_NUM(linenum));
    }
    break;
#if EDITOR
  case 'E': /* Editor name */
    ap_str(editor);
    break;
#endif
  case 'f': /* File name */
    ap_str(ifile::getCurrentIfile()->getFilename());
    break;
  case 'F': /* Last component of file name */
    ap_str(filename::last_component(ifile::getCurrentIfile()->getFilename()));
    break;
  case 'g': /* Shell-escaped file name */
    s = filename::shell_quote(ifile::getCurrentIfile()->getFilename());
    ap_str(s);
    free(s);
    break;
  case 'i': /* Index into list of files */
#if TAGS
    if (tags::ntags())
      ap_int(tags::curr_tag());
    else
#endif
      ap_int(ifile::getIndex(ifile::getCurrentIfile()));
    break;
  case 'l': /* Current line number */
    linenum = linenum::currline(where);
    if (linenum != 0)
      ap_linenum(linenum);
    else
      ap_quest();
    break;
  case 'L': /* Final line number */
    len = ch::length();
    if (len == NULL_POSITION || len == ch_zero || (linenum = linenum::find_linenum(len)) <= 0)
      ap_quest();
    else
      ap_linenum(linenum - 1);
    break;
  case 'm': /* Number of files */
#if TAGS
    n = tags::ntags();
    if (n)
      ap_int(n);
    else
#endif
      ap_int(ifile::numIfiles());
    break;
  case 'p': /* Percent into file (bytes) */
    pos = curr_byte(where);
    len = ch::length();
    if (pos != NULL_POSITION && len > 0)
      ap_int(os::percentage(pos, len));
    else
      ap_quest();
    break;
  case 'P': /* Percent into file (lines) */
    linenum = linenum::currline(where);
    if (linenum == 0 || (len = ch::length()) == NULL_POSITION || len == ch_zero || (last_linenum = linenum::find_linenum(len)) <= 0)
      ap_quest();
    else
      ap_int(os::percentage(linenum, last_linenum));
    break;
  case 's': /* Size of file */
  case 'B':
    len = ch::length();
    if (len != NULL_POSITION)
      ap_pos(len);
    else
      ap_quest();
    break;
  case 't': /* Truncate trailing spaces in the message */
    while (mp > message && mp[-1] == ' ')
      mp--;
    *mp = '\0';
    break;
  case 'T': /* Type of list */
#if TAGS
    if (tags::ntags())
      ap_str((char*)"tag");
    else
#endif
      ap_str((char*)"file");
    break;
  case 'x': /* Name of next file */
    h = ifile::nextIfile(ifile::getCurrentIfile());
    if (h != nullptr)
      ap_str(h->getFilename());
    else
      ap_quest();
    break;
  }
}

/*
 * Skip a false conditional.
 * When a false condition is found (either a false IF or the ELSE part
 * of a true IF), this routine scans the prototype string to decide
 * where to resume parsing the string.
 * We must keep track of nested IFs and skip them properly.
 */
static const char* skipcond(const char* p)
{
  int iflevel;

  /*
   * We came in here after processing a ? or :,
   * so we start nested one level deep.
   */
  iflevel = 1;

  for (;;)
    switch (*++p) {
    case '?':
      /*
       * Start of a nested IF.
       */
      iflevel++;
      break;
    case ':':
      /*
       * Else.
       * If this matches the IF we came in here with,
       * then we're done.
       */
      if (iflevel == 1)
        return (p);
      break;
    case '.':
      /*
       * Endif.
       * If this matches the IF we came in here with,
       * then we're done.
       */
      if (--iflevel == 0)
        return (p);
      break;
    case '\\':
      /*
       * Backslash escapes the next character.
       */
      ++p;
      break;
    case '\0':
      /*
       * Whoops.  Hit end of string.
       * This is a malformed conditional, but just treat it
       * as if all active conditionals ends here.
       */
      return (p - 1);
    }
  /*NOTREACHED*/
}

/*
 * Decode a char that represents a position on the screen.
 */
static const char* wherechar(char const* p, int* wp)
{
  switch (*p) {
  case 'b':
  case 'd':
  case 'l':
  case 'p':
  case 'P':
    switch (*++p) {
    case 't':
      *wp = TOP;
      break;
    case 'm':
      *wp = MIDDLE;
      break;
    case 'b':
      *wp = BOTTOM;
      break;
    case 'B':
      *wp = BOTTOM_PLUS_ONE;
      break;
    case 'j':
      *wp = position::sindex_from_sline(jump_sline);
      break;
    default:
      *wp = TOP;
      p--;
      break;
    }
  }
  return (p);
}

/*
 * Construct a message based on a prototype string.
 */
char* pr_expand(const char* proto, int maxwidth)
{
  const char* p;
  int         c;
  int         where;

  mp = message;

  if (*proto == '\0')
    return ((char*)"");

  for (p = proto; *p != '\0'; p++) {
    switch (*p) {
    default: /* Just put the character in the message */
      ap_char(*p);
      break;
    case '\\': /* Backslash escapes the next character */
      p++;
      ap_char(*p);
      break;
    case '?': /* Conditional (IF) */
      if ((c = *++p) == '\0')
        --p;
      else {
        where = 0;
        p     = wherechar(p, &where);
        if (!cond(c, where))
          p = skipcond(p);
      }
      break;
    case ':': /* ELSE */
      p = skipcond(p);
      break;
    case '.': /* ENDIF */
      break;
    case '%': /* Percent escape */
      if ((c = *++p) == '\0')
        --p;
      else {
        where = 0;
        p     = wherechar(p, &where);
        protochar(c, where,
#if EDITOR
            (proto == editproto));
#else
            0);
#endif
      }
      break;
    }
  }

  if (mp == message)
    return ((char*)"");
  if (maxwidth > 0 && mp >= message + maxwidth) {
    /*
     * Message is too long.
     * Return just the final portion of it.
     */
    return (mp - maxwidth);
  }
  return (message);
}

/*
 * Return a message suitable for printing by the "=" command.
 */
char* eq_message(void)
{
  return (pr_expand(eqproto, 0));
}

/*
 * Return a prompt.
 * This depends on the prompt type (SHORT, MEDIUM, LONG), etc.
 * If we can't come up with an appropriate prompt, return NULL
 * and the caller will prompt with a colon.
 */
char* pr_string(void)
{
  char* prompt;
  int   type;

  type   = (!option::Option::less_is_more) ? pr_type : pr_type ? 0
                                                               : 1;
  prompt = pr_expand((ch::getflags() & CH_HELPFILE) ? hproto : prproto[type],
      sc_width - so_s_width - so_e_width - 2);
  new_file = false;
  return (prompt);
}

/*
 * Return a message suitable for printing while waiting in the F command.
 */
char* wait_message(void)
{
  return (pr_expand(wproto, sc_width - so_s_width - so_e_width - 2));
}

} // namespace prompt