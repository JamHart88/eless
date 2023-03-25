/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * User-level command processor.
 */

#include "command.hpp"
#include "brac.hpp"
#include "ch.hpp"
#include "cmd.hpp"
#include "cmdbuf.hpp"
#include "decode.hpp"
#include "edit.hpp"
#include "filename.hpp"
#include "forwback.hpp"
#include "ifile.hpp"
#include "input.hpp"
#include "jump.hpp"
#include "less.hpp"
#include "line.hpp"
#include "linenum.hpp"
#include "lsystem.hpp"
#include "mark.hpp"
#include "optfunc.hpp"
#include "option.hpp"
#include "opttbl.hpp"
#include "output.hpp"
#include "position.hpp"
#include "prompt.hpp"
#include "screen.hpp"
#include "search.hpp"
#include "signal.hpp"
#include "tags.hpp"
#include "ttyin.hpp"
#include "utils.hpp"

#include "debug.hpp" // temporary

// TODO: Move these externals to namespaces
extern int           erase_char, erase2_char, kill_char;
extern int           quit_if_one_screen;
extern int           squished;
extern int           sc_width;
extern int           sc_height;
extern char*         kent;
extern int           swindow;
extern int           jump_sline;
extern int           quitting;
extern int           wscroll;
extern int           top_scroll;
extern int           hshift;
extern int           bs_mode;
extern int           show_attn;
extern int           status_col;
extern position_t    highest_hilite;
extern position_t    start_attnpos;
extern position_t    end_attnpos;
extern char*         every_first_cmd;
extern char          version[];
extern struct scrpos initial_scrpos;
extern void*         ml_search;
extern void*         ml_examine;
extern int           wheel_lines;

#if SHELL_ESCAPE || PIPEC
extern void* ml_shell;
#endif

#if EDITOR
extern char* editor;
extern char* editproto;
#endif

extern int shift_count;
extern int oldbot;
extern int forw_prompt;

namespace command {

#if SHELL_ESCAPE
static char* shellcmd = nullptr; // For holding last shell command for "!!"
#endif

static int                     mca;         // The multicharacter command (action)
static int                     search_type; // The previous type of search
static linenum_t               number;      // The number typed by the user
static long                    fraction;    // The fractional part of the number
static struct option::loption* curropt;
static int                     opt_lower;
static int                     optflag;
static bool                    optgetname = false;
static position_t              bottompos;
static int                     save_hshift;
static int                     save_bs_mode;

#if PIPEC
static char pipec;
#endif

/* Stack of ungotten chars (via ungetcc) */
struct ungot {
  struct ungot* ug_next;
  lwchar_t      ug_char;
};
static struct ungot* ungot = nullptr;

static void multi_search(char* pattern, int n, int silent);

/*
 * Move the cursor to start of prompt line before executing a command.
 * This looks nicer if the command takes a long time before
 * updating the screen.
 */
static void cmd_exec(void)
{
  search::clear_attn();
  screen::clear_bot();
  output::flush();
}

/*
 * Indicate we are reading a multi-character command.
 */
static void set_mca(int action)
{
  mca = action;
  screen::deinit_mouse(); /* we don't want mouse events while entering a cmd */
  screen::clear_bot();
  cmdbuf::clear_cmd();
}

/*
 * Indicate we are not reading a multi-character command.
 */
static void clear_mca(void)
{
  if (mca == 0)
    return;
  mca = 0;
  screen::init_mouse();
}

/*
 * Set up the display to start a new multi-character command.
 */
static void start_mca(int action, const char* prompt, void* mlist, int cmdflags)
{
  set_mca(action);
  cmdbuf::cmd_putstr(prompt);
  cmdbuf::set_mlist(mlist, cmdflags);
}

int in_mca(void)
{
  return (mca != 0 && mca != A_PREFIX);
}

/*
 * Set up the display to start a new search command.
 */
static void mca_search(void)
{
#if HILITE_SEARCH
  if (search_type & SRCH_FILTER)
    set_mca(A_FILTER);
  else
#endif
      if (search_type & SRCH_FORW)
    set_mca(A_F_SEARCH);
  else
    set_mca(A_B_SEARCH);

  if (search_type & SRCH_NO_MATCH)
    cmdbuf::cmd_putstr("Non-match ");
  if (search_type & SRCH_FIRST_FILE)
    cmdbuf::cmd_putstr("First-file ");
  if (search_type & SRCH_PAST_EOF)
    cmdbuf::cmd_putstr("EOF-ignore ");
  if (search_type & SRCH_NO_MOVE)
    cmdbuf::cmd_putstr("Keep-pos ");
  if (search_type & SRCH_NO_REGEX)
    cmdbuf::cmd_putstr("Regex-off ");

#if HILITE_SEARCH
  if (search_type & SRCH_FILTER)
    cmdbuf::cmd_putstr("&/");
  else
#endif
      if (search_type & SRCH_FORW)
    cmdbuf::cmd_putstr("/");
  else
    cmdbuf::cmd_putstr("?");
  forw_prompt = 0;
  cmdbuf::set_mlist(ml_search, 0);
}

/*
 * Set up the display to start a new toggle-option command.
 */
static void mca_opt_toggle(void)
{
  int   no_prompt;
  int   flag;
  char* dash;

  no_prompt = (optflag & option::OPT_NO_PROMPT);
  flag      = (optflag & ~option::OPT_NO_PROMPT);
  dash      = (flag == option::OPT_NO_TOGGLE) ? (char*)"_" : (char*)"-";

  set_mca(A_OPT_TOGGLE);
  cmdbuf::cmd_putstr(dash);
  if (optgetname)
    cmdbuf::cmd_putstr(dash);
  if (no_prompt)
    cmdbuf::cmd_putstr("(P)");
  switch (flag) {
  case option::OPT_UNSET:
    cmdbuf::cmd_putstr("+");
    break;
  case option::OPT_SET:
    cmdbuf::cmd_putstr("!");
    break;
  }
  forw_prompt = 0;
  cmdbuf::set_mlist(nullptr, 0);
}

/*
 * Execute a multicharacter command.
 */
static void exec_mca(void)
{
  char* cbuf;

  cmd_exec();
  cbuf = cmdbuf::get_cmdbuf();

  switch (mca) {
  case A_F_SEARCH:
  case A_B_SEARCH:
    multi_search(cbuf, (int)number, 0);
    break;
#if HILITE_SEARCH
  case A_FILTER:
    search_type ^= SRCH_NO_MATCH;
    search::set_filter_pattern(cbuf, search_type);
    break;
#endif
  case A_FIRSTCMD:
    /*
     * Skip leading spaces or + signs in the string.
     */
    while (*cbuf == '+' || *cbuf == ' ')
      cbuf++;
    if (every_first_cmd != nullptr)
      free(every_first_cmd);
    if (*cbuf == '\0')
      every_first_cmd = nullptr;
    else
      every_first_cmd = utils::save(cbuf);
    break;
  case A_OPT_TOGGLE:
    option::toggle_option(curropt, opt_lower, cbuf, optflag);
    curropt = nullptr;
    break;
  case A_F_BRACKET:
    bracket::match_brac(cbuf[0], cbuf[1], 1, (int)number);
    break;
  case A_B_BRACKET:
    bracket::match_brac(cbuf[1], cbuf[0], 0, (int)number);
    break;
#if EXAMINE
  case A_EXAMINE:
    edit::edit_list(cbuf);
#if TAGS
    /* If tag structure is loaded then clean it up. */
    tags::cleantags();
#endif
    break;
#endif
#if SHELL_ESCAPE
  case A_SHELL:
    /*
     * !! just uses whatever is in shellcmd.
     * Otherwise, copy cmdbuf to shellcmd,
     * expanding any special characters ("%" or "#").
     */
    if (*cbuf != '!') {
      if (shellcmd != nullptr)
        free(shellcmd);
      shellcmd = filename::fexpand(cbuf);
    }

    if (shellcmd == nullptr)
      lsystem::lsystem((char*)"", (char*)"!done");
    else
      lsystem::lsystem(shellcmd, (char*)"!done");
    break;
#endif
#if PIPEC
  case A_PIPE:

    (void)lsystem::pipe_mark(pipec, cbuf);
    output::error((char*)"|done", NULL_PARG);
    break;
#endif
  }
}

/*
 * Is a character an erase or kill char?
 */
static int is_erase_char(int c)
{
  return (c == erase_char || c == erase2_char || c == kill_char);
}

/*
 * Is a character a carriage return or newline?
 */
static int is_newline_char(int c)
{
  return (c == '\n' || c == '\r');
}

/*
 * Handle the first char of an option (after the initial dash).
 */
static int mca_opt_first_char(int c)
{
  int flag = (optflag & ~option::OPT_NO_PROMPT);
  if (flag == option::OPT_NO_TOGGLE) {
    switch (c) {
    case '_':
      /* "__" = long option name. */
      optgetname = true;
      mca_opt_toggle();
      return (MCA_MORE);
    }
  } else {
    switch (c) {
    case '+':
      /* "-+" = UNSET. */
      optflag = (flag == option::OPT_UNSET) ? option::OPT_TOGGLE : option::OPT_UNSET;
      mca_opt_toggle();
      return (MCA_MORE);
    case '!':
      /* "-!" = SET */
      optflag = (flag == option::OPT_SET) ? option::OPT_TOGGLE : option::OPT_SET;
      mca_opt_toggle();
      return (MCA_MORE);
    case control<int>('P'):
      optflag ^= option::OPT_NO_PROMPT;
      mca_opt_toggle();
      return (MCA_MORE);
    case '-':
      /* "--" = long option name. */
      optgetname = true;
      mca_opt_toggle();
      return (MCA_MORE);
    }
  }
  /* Char was not handled here. */
  return (NO_MCA);
}

/*
 * Add a char to a long option name.
 * See if we've got a match for an option name yet.
 * If so, display the complete name and stop
 * accepting chars until user hits RETURN.
 */
static int mca_opt_nonfirst_char(int c)
{
  char* p;
  char* oname;
  int   err;

  if (curropt != nullptr) {
    /*
     * Already have a match for the name.
     * Don't accept anything but erase/kill.
     */
    if (is_erase_char(c))
      return (MCA_DONE);
    return (MCA_MORE);
  }
  /*
   * Add char to cmd buffer and try to match
   * the option name.
   */
  if (cmdbuf::cmd_char(c) == CC_QUIT)
    return (MCA_DONE);
  p         = cmdbuf::get_cmdbuf();
  opt_lower = islower(static_cast<int>(p[0]));
  err       = 0;
  curropt   = opttbl::findopt_name(&p, &oname, &err);
  if (curropt != nullptr) {
    /*
     * Got a match.
     * Remember the option and
     * display the full option name.
     */
    cmdbuf::cmd_reset();
    mca_opt_toggle();
    for (p = oname; *p != '\0'; p++) {
      c = static_cast<unsigned char>(*p);
      if (!opt_lower && islower(c))
        c = isupper(c);
      if (cmdbuf::cmd_char(c) != CC_OK)
        return (MCA_DONE);
    }
  } else if (err != option::OPT_AMBIG) {
    screen::bell();
  }
  return (MCA_MORE);
}

/*
 * Handle a char of an option toggle command.
 */
static int mca_opt_char(int c)
{
  parg_t parg;

  /*
   * This may be a short option (single char),
   * or one char of a long option name,
   * or one char of the option parameter.
   */
  if (curropt == nullptr && cmdbuf::len_cmdbuf() == 0) {
    int ret = mca_opt_first_char(c);
    if (ret != NO_MCA)
      return (ret);
  }
  if (optgetname) {
    /* We're getting a long option name.  */
    if (!is_newline_char(c))
      return (mca_opt_nonfirst_char(c));
    if (curropt == nullptr) {
      parg.p_string = cmdbuf::get_cmdbuf();
      output::error((char*)"There is no --%s option", parg);
      return (MCA_DONE);
    }
    optgetname = false;
    cmdbuf::cmd_reset();
  } else {
    if (is_erase_char(c))
      return (NO_MCA);
    if (curropt != nullptr)
      /* We're getting the option parameter. */
      return (NO_MCA);
    curropt = opttbl::findopt(c);
    if (curropt == nullptr) {
      parg.p_string = option::propt(c);
      output::error((char*)"There is no %s option", parg);
      return (MCA_DONE);
    }
    opt_lower = islower(c);
  }
  /*
   * If the option which was entered does not take a
   * parameter, toggle the option immediately,
   * so user doesn't have to hit RETURN.
   */
  if ((optflag & ~option::OPT_NO_PROMPT) != option::OPT_TOGGLE || !option::opt_has_param(curropt)) {
    option::toggle_option(curropt, opt_lower, (char*)"", optflag);
    return (MCA_DONE);
  }
  /*
   * Display a prompt appropriate for the option parameter.
   */
  start_mca(A_OPT_TOGGLE, option::opt_prompt(curropt), (void*)nullptr, 0);
  return (MCA_MORE);
}

/*
 * Handle a char of a search command.
 */
static int mca_search_char(int c)
{
  int flag = 0;

  /*
   * Certain characters as the first char of
   * the pattern have special meaning:
   *    !  Toggle the NO_MATCH flag
   *    *  Toggle the PAST_EOF flag
   *    @  Toggle the FIRST_FILE flag
   */
  if (cmdbuf::len_cmdbuf() > 0)
    return (NO_MCA);

  switch (c) {
  case control<int>('E'): /* ignore END of file */
  case '*':
    if (mca != A_FILTER)
      flag = SRCH_PAST_EOF;
    break;
  case control<int>('F'): /* FIRST file */
  case '@':
    if (mca != A_FILTER)
      flag = SRCH_FIRST_FILE;
    break;
  case control<int>('K'): /* KEEP position */
    if (mca != A_FILTER)
      flag = SRCH_NO_MOVE;
    break;
  case control<int>('R'): /* Don't use REGULAR EXPRESSIONS */
    flag = SRCH_NO_REGEX;
    break;
  case control<int>('N'): /* NOT match */
  case '!':
    flag = SRCH_NO_MATCH;
    break;
  }

  if (flag != 0) {
    search_type ^= flag;
    mca_search();
    return (MCA_MORE);
  }
  return (NO_MCA);
}

/*
 * Handle a character of a multi-character command.
 */
static int mca_char(int c)
{
  int ret;

  switch (mca) {
  case 0:
    /*
     * We're not in a multicharacter command.
     */
    return (NO_MCA);

  case A_PREFIX:
    /*
     * In the prefix of a command.
     * This not considered a multichar command
     * (even tho it uses cmdbuf, etc.).
     * It is handled in the commands() switch.
     */
    return (NO_MCA);

  case A_DIGIT:
    /*
     * Entering digits of a number.
     * Terminated by a non-digit.
     */
    if (!((c >= '0' && c <= '9') || c == '.') && decode::editchar(c, EC_PEEK | EC_NOHISTORY | EC_NOCOMPLETE | EC_NORIGHTLEFT) == A_INVALID) {
      /*
       * Not part of the number.
       * End the number and treat this char
       * as a normal command character.
       */
      number = cmdbuf::cmd_int(&fraction);
      clear_mca();
      cmdbuf::cmd_accept();
      return (NO_MCA);
    }
    break;

  case A_OPT_TOGGLE:
    ret = mca_opt_char(c);
    if (ret != NO_MCA)
      return (ret);
    break;

  case A_F_SEARCH:
  case A_B_SEARCH:
  case A_FILTER:
    ret = mca_search_char(c);
    if (ret != NO_MCA)
      return (ret);
    break;

  default:
    /* Other multicharacter command. */
    break;
  }

  /*
   * The multichar command is terminated by a newline.
   */
  if (is_newline_char(c)) {
    /*
     * Execute the command.
     */
    exec_mca();
    return (MCA_DONE);
  }

  /*
   * Append the char to the command buffer.
   */
  if (cmdbuf::cmd_char(c) == CC_QUIT)
    /*
     * Abort the multi-char command.
     */
    return (MCA_DONE);

  if ((mca == A_F_BRACKET || mca == A_B_BRACKET) && cmdbuf::len_cmdbuf() >= 2) {
    /*
     * Special case for the bracket-matching commands.
     * Execute the command after getting exactly two
     * characters from the user.
     */
    exec_mca();
    return (MCA_DONE);
  }

  /*
   * Need another character.
   */
  return (MCA_MORE);
}

/*
 * Discard any buffered file data.
 */
static void clear_buffers(void)
{
  if (!(ch::getflags() & CH_CANSEEK))
    return;
  ch::flush();
  linenum::clr_linenum();
#if HILITE_SEARCH
  search::clr_hilite();
#endif
}

/*
 * Make sure the screen is displayed.
 */
static void make_display(void)
{
  /*
   * If nothing is displayed yet, display starting from initial_scrpos.
   */
  if (position::empty_screen()) {
    if (initial_scrpos.pos == NULL_POSITION)
      /*
       * {{ Maybe this should be:
       *    jump::jump_loc(ch_zero, jump_sline);
       *    but this behavior seems rather unexpected
       *    on the first screen. }}
       */
      jump::jump_loc(ch_zero, 1);
    else
      jump::jump_loc(initial_scrpos.pos, initial_scrpos.ln);
  } else if (screen_trashed != NOT_TRASHED) {
    int save_top_scroll       = top_scroll;
    int save_eoi              = less::Globals::ignore_eoi;
    top_scroll                = 1;
    less::Globals::ignore_eoi = 0;
    if (screen_trashed == TRASHED_AND_REOPEN_FILE) {

      edit::reopen_curr_ifile();
      jump::jump_forw();
    }
    jump::repaint();
    top_scroll                = save_top_scroll;
    less::Globals::ignore_eoi = save_eoi;
  }
}

/*

 * Display the appropriate prompt.
 */
static void prompt(void)
{
  const char* p;

  if (ungot != nullptr && ungot->ug_char != CHAR_END_COMMAND) {
    /*
     * No prompt necessary if commands are from
     * ungotten chars rather than from the user.
     */
    return;
  }

  /*
   * Make sure the screen is displayed.
   */
  make_display();
  bottompos = position::position(BOTTOM_PLUS_ONE);

  /*
   * If we've hit EOF on the last file and the -E flag is set, quit.
   */
  if (option::get_quit_at_eof() == option::OPT_ONPLUS && forwback::eof_displayed() && !(ch::getflags() & CH_HELPFILE) && ifile::nextIfile(ifile::getCurrentIfile()) == nullptr)
    utils::quit(QUIT_OK);

  /*
   * If the entire file is displayed and the -F flag is set, quit.
   */
  if (quit_if_one_screen && forwback::entire_file_displayed() && !(ch::getflags() & CH_HELPFILE) && ifile::nextIfile(ifile::getCurrentIfile()) == nullptr)
    utils::quit(QUIT_OK);

  /*
   * Select the proper prompt and display it.
   */
  /*
   * If the previous action was a forward movement,
   * don't screen::clear the bottom line of the display;
   * just print the prompt since the forward movement guarantees
   * that we're in the right position to display the prompt.
   * Clearing the line could cause a problem: for example, if the last
   * line displayed ended at the right screen edge without a newline,
   * then clearing would screen::clear the last displayed line rather than
   * the prompt line.
   */
  if (!forw_prompt)
    screen::clear_bot();
  cmdbuf::clear_cmd();
  forw_prompt = 0;
  p           = prompt::pr_string();
  if (search::is_filtering())
    output::putstr("& ");
  if (p == nullptr || *p == '\0')
    output::putchr(':');
  else {
    screen::at_enter(AT_STANDOUT);
    output::putstr(p);
    screen::at_exit();
  }
  screen::clear_eol();
}

/*
 * Display the less version message.
 */

void dispversion(void)
{
  parg_t parg;

  parg.p_string = version;
  output::error((char*)"less %s", parg);
}

/*
 * Return a character to complete a partial command, if possible.
 */
static lwchar_t getcc_end_command(void)
{
  switch (mca) {
  case A_DIGIT:
    /* We have a number but no command.  Treat as #g. */
    return ('g');
  case A_F_SEARCH:
  case A_B_SEARCH:
    /* We have "/string" but no newline.  Add the \n. */
    return ('\n');
  default:
    /* Some other incomplete command.  Let user complete it. */
    return (ttyin::getchr());
  }
}

/*
 * Get command character.
 * The character normally comes from the keyboard,
 * but may come from ungotten characters
 * (characters previously given to ungetcc or ungetsc).
 */
static lwchar_t getccu(void)
{
  lwchar_t c;
  if (ungot == nullptr) {
    /* Normal case: no ungotten chars.
     * Get char from the user. */
    c = ttyin::getchr();
  } else {
    /* Ungotten chars available:
     * Take the top of stack (most recent). */
    struct ungot* ug = ungot;
    c                = ug->ug_char;
    ungot            = ug->ug_next;
    free(ug);

    if (c == CHAR_END_COMMAND)
      c = getcc_end_command();
  }
  return (c);
}

/*
 * Get a command character, but if we receive the orig sequence,
 * convert it to the repl sequence.
 */
static lwchar_t getcc_repl(char const* orig, char const* repl, lwchar_t (*gr_getc)(void), void (*gr_ungetc)(lwchar_t))
{
  lwchar_t c;
  lwchar_t keys[16];
  int      ki = 0;

  c = (*gr_getc)();
  if (orig == nullptr || orig[0] == '\0')
    return c;
  for (;;) {
    keys[ki] = c;
    if (c != (lwchar_t)orig[ki] || ki >= (int)sizeof(keys) - 1) {
      /* This is not orig we have been receiving.
       * If we have stashed chars in keys[],
       * unget them and return the first one. */
      while (ki > 0)
        (*gr_ungetc)(keys[ki--]);
      return keys[0];
    }
    if (orig[++ki] == '\0') {
      /* We've received the full orig sequence.
       * Return the repl sequence. */
      ki = strlen(repl) - 1;
      while (ki > 0)
        (*gr_ungetc)(repl[ki--]);
      return repl[0];
    }
    /* We've received a partial orig sequence (ki chars of it).
     * Get next char and see if it continues to match orig. */
    c = (*gr_getc)();
  }
}

/*
 * Get command character.
 */

int getcc(void)
{
  /* Replace kent (keypad Enter) with a newline. */
  return getcc_repl(kent, "\n", getccu, ungetcc);
}

/*
 * "Unget" a command character.
 * The next getcc() will return this character.
 */

void ungetcc(lwchar_t c)
{
  struct ungot* ug = (struct ungot*)utils::ecalloc(1, sizeof(struct ungot));

  ug->ug_char = c;
  ug->ug_next = ungot;
  ungot       = ug;
}

/*
 * Unget a whole string of command characters.
 * The next sequence of getcc()'s will return this string.
 */

void ungetsc(char* s)
{
  char* p;

  for (p = s + strlen(s) - 1; p >= s; p--)
    ungetcc(*p);
}

/*
 * Peek the next command character, without consuming it.
 */

lwchar_t peekcc(void)
{
  lwchar_t c = getcc();
  ungetcc(c);
  return c;
}

/*
 * Search for a pattern, possibly in multiple files.
 * If SRCH_FIRST_FILE is set, begin searching at the first file.
 * If SRCH_PAST_EOF is set, continue the search thru multiple files.
 */
static void multi_search(char* pattern, int n, int silent)
{
  int           nomore;
  ifile::Ifile* save_ifile;
  int           changed_file;

  changed_file = 0;
  save_ifile   = edit::save_curr_ifile();

  if (search_type & SRCH_FIRST_FILE) {
    /*
     * Start at the first (or last) file
     * in the command line list.
     */
    if (search_type & SRCH_FORW)
      nomore = edit::edit_first();
    else
      nomore = edit::edit_last();
    if (nomore) {
      edit::unsave_ifile(save_ifile);
      return;
    }
    changed_file = 1;
    search_type &= ~SRCH_FIRST_FILE;
  }

  for (;;) {
    n = search::search(search_type, pattern, n);
    /*
     * The SRCH_NO_MOVE flag doesn't "stick": it gets cleared
     * after being used once.  This allows "n" to work after
     * using a /@@ search.
     */
    search_type &= ~SRCH_NO_MOVE;
    if (n == 0) {
      /*
       * Found it.
       */
      edit::unsave_ifile(save_ifile);
      return;
    }

    if (n < 0)
      /*
       * Some kind of output::error in the search.
       * Error message has been printed by search().
       */
      break;

    if ((search_type & SRCH_PAST_EOF) == 0)
      /*
       * We didn't find a match, but we're
       * supposed to search only one file.
       */
      break;
    /*
     * Move on to the next file.
     */
    if (search_type & SRCH_FORW)
      nomore = edit::edit_next(1);
    else
      nomore = edit::edit_prev(1);
    if (nomore)
      break;
    changed_file = 1;
  }

  /*
   * Didn't find it.
   * Print an output::error message if we haven't already.
   */
  if (n > 0 && !silent)
    output::error((char*)"Pattern not found", NULL_PARG);

  if (changed_file) {
    /*
     * Restore the file we were originally viewing.
     */
    edit::reedit_ifile(save_ifile);
  } else {
    edit::unsave_ifile(save_ifile);
  }
}

/*
 * Forward forever, or until a highlighted line appears.
 */
static int forw_loop(int until_hilite)
{
  position_t curr_len;

  if (ch::getflags() & CH_HELPFILE)
    return (A_NOACTION);

  cmd_exec();
  jump::jump_forw_buffered();
  curr_len                  = ch::length();
  highest_hilite            = until_hilite ? curr_len : NULL_POSITION;
  less::Globals::ignore_eoi = 1;
  while (!less::Globals::sigs) {
    if (until_hilite && highest_hilite > curr_len) {
      screen::bell();
      break;
    }
    make_display();
    forwback::forward(1, 0, 0);
  }
  less::Globals::ignore_eoi = 0;
  ch::set_eof();

  /*
   * This gets us back in "F mode" after processing
   * a non-abort signal (e.g. window-change).
   */
  if (less::Globals::sigs && !is_abort_signal(less::Globals::sigs))
    return (until_hilite ? A_F_UNTIL_HILITE : A_F_FOREVER);

  return (A_NOACTION);
}

/*
 * Main command processor.
 * Accept and execute commands until a quit command.
 */

void commands(void)
{
  int           c;
  int           action;
  char*         cbuf;
  int           newaction;
  int           save_search_type;
  char*         extra;
  char          tbuf[2];
  parg_t        parg;
  ifile::Ifile* old_ifile;
  ifile::Ifile* new_ifile;
  char*         tagfile;

  search_type = SRCH_FORW;
  wscroll     = (sc_height + 1) / 2;
  newaction   = A_NOACTION;

  for (;;) {
    c = '\0';
    debug::debug("Top of command for loop");
    clear_mca();
    cmdbuf::cmd_accept();
    number  = 0;
    curropt = nullptr;

    /*
     * See if any signals need processing.
     */
    if (less::Globals::sigs) {
      sig::psignals();
      if (quitting)
        utils::quit(QUIT_SAVED_STATUS);
    }

    /*
     * See if window size changed, for systems that don't
     * generate SIGWINCH.
     */
    screen::check_winch();

    /*
     * Display prompt and accept a character.
     */
    cmdbuf::cmd_reset();
    prompt();
    if (less::Globals::sigs)
      continue;
    if (newaction == A_NOACTION) {
      c = getcc();
      debug::debug((char*)"getcc called with:");
      char s = static_cast<char>(c);
      debug::debug(&s);
    }

  again:
    debug::debug("at again:");

    if (less::Globals::sigs)
      continue;

    if (newaction != A_NOACTION) {
      action    = newaction;
      newaction = A_NOACTION;
    } else {
      /*
       * If we are in a multicharacter command, call mca_char.
       * Otherwise we call decode::fcmd_decode to determine the
       * action to be performed.
       */
      char s; // temp debug use
      if (mca)
        switch (mca_char(c)) {
        case MCA_MORE:
          /*
           * Need another character.
           */
          c = getcc();
          debug::debug("need another char which is:");
          s = static_cast<char>(c);
          debug::debug(&s);

          goto again;

        case MCA_DONE:
          /*
           * Command has been handled by mca_char.
           * Start clean with a prompt.
           */
          continue;
        case NO_MCA:
          /*
           * Not a multi-char command
           * (at least, not anymore).
           */
          break;
        }

      /*
       * Decode the command character and decide what to do.
       */
      if (mca) {
        debug::debug("decode the command char");
        /*
         * We're in a multichar command.
         * Add the character to the command buffer
         * and display it on the screen.
         * If the user backspaces past the start
         * of the line, abort the command.
         */
        if (cmdbuf::cmd_char(c) == CC_QUIT || cmdbuf::len_cmdbuf() == 0)
          continue;
        cbuf = cmdbuf::get_cmdbuf();

        debug::debug("MCA command - cbuf is:");
        debug::debug(cbuf);
      } else {
        debug::debug("dont use cmdbuf::cmd_char");
        /*
         * Don't use cmdbuf::cmd_char if we're starting fresh
         * at the beginning of a command, because we
         * don't want to echo the command until we know
         * it is a multichar command.  We also don't
         * want erase_char/kill_char to be treated
         * as line editing characters.
         */
        tbuf[0] = c;
        tbuf[1] = '\0';
        cbuf    = tbuf;
      }
      extra  = nullptr;
      action = decode::fcmd_decode(cbuf, &extra);
      /*
       * If an "extra" string was returned,
       * process it as a string of command characters.
       */
      if (extra != nullptr)
        ungetsc(extra);
    }
    /*
     * Clear the cmdbuf string.
     * (But not if we're in the prefix of a command,
     * because the partial command string is kept there.)
     */
    if (action != A_PREFIX)
      cmdbuf::cmd_reset();

    switch (action) {
    case A_DIGIT:
      /*
       * First digit of a number.
       */
      start_mca(A_DIGIT, ":", (void*)nullptr, CF_QUIT_ON_ERASE);
      goto again;

    case A_F_WINDOW:
      /*
       * Forward one window (and set the window size).
       */
      if (number > 0)
        swindow = (int)number;
      /* FALLTHRU */
    case A_F_SCREEN:
      /*
       * Forward one screen.
       */
      if (number <= 0)
        number = optfunc::get_swindow();
      cmd_exec();
      if (show_attn)
        input::set_attnpos(bottompos);
      forwback::forward((int)number, 0, 1);
      break;

    case A_B_WINDOW:
      /*
       * Backward one window (and set the window size).
       */
      if (number > 0)
        swindow = (int)number;
      /* FALLTHRU */
    case A_B_SCREEN:
      /*
       * Backward one screen.
       */
      if (number <= 0)
        number = optfunc::get_swindow();
      cmd_exec();
      forwback::backward((int)number, 0, 1);
      break;

    case A_F_LINE:
      /*
       * Forward N (default 1) line.
       */
      if (number <= 0)
        number = 1;
      cmd_exec();
      if (show_attn == option::OPT_ONPLUS && number > 1)
        input::set_attnpos(bottompos);
      forwback::forward((int)number, 0, 0);
      break;

    case A_B_LINE:
      /*
       * Backward N (default 1) line.
       */
      if (number <= 0)
        number = 1;
      cmd_exec();
      forwback::backward((int)number, 0, 0);
      break;

    case A_F_MOUSE:
      /*
       * Forward wheel_lines lines.
       */
      cmd_exec();
      forwback::forward(wheel_lines, 0, 0);
      break;

    case A_B_MOUSE:
      /*
       * Backward wheel_lines lines.
       */
      cmd_exec();
      forwback::backward(wheel_lines, 0, 0);
      break;

    case A_FF_LINE:
      /*
       * Force forward N (default 1) line.
       */
      if (number <= 0)
        number = 1;
      cmd_exec();
      if (show_attn == option::OPT_ONPLUS && number > 1)
        input::set_attnpos(bottompos);
      forwback::forward((int)number, 1, 0);
      break;

    case A_BF_LINE:
      /*
       * Force backward N (default 1) line.
       */
      if (number <= 0)
        number = 1;
      cmd_exec();
      forwback::backward((int)number, 1, 0);
      break;

    case A_FF_SCREEN:
      /*
       * Force forward one screen.
       */
      if (number <= 0)
        number = optfunc::get_swindow();
      cmd_exec();
      if (show_attn == option::OPT_ONPLUS)
        input::set_attnpos(bottompos);
      forwback::forward((int)number, 1, 0);
      break;

    case A_F_FOREVER:
      /*
       * Forward forever, ignoring EOF.
       */
      if (show_attn)
        input::set_attnpos(bottompos);
      newaction = forw_loop(0);
      break;

    case A_F_UNTIL_HILITE:
      newaction = forw_loop(1);
      break;

    case A_F_SCROLL:
      /*
       * Forward N lines
       * (default same as last 'd' or 'u' command).
       */
      if (number > 0)
        wscroll = (int)number;
      cmd_exec();
      if (show_attn == option::OPT_ONPLUS)
        input::set_attnpos(bottompos);
      forwback::forward(wscroll, 0, 0);
      break;

    case A_B_SCROLL:
      /*
       * Forward N lines
       * (default same as last 'd' or 'u' command).
       */
      if (number > 0)
        wscroll = (int)number;
      cmd_exec();
      forwback::backward(wscroll, 0, 0);
      break;

    case A_FREPAINT:
      /*
       * Flush buffers, then jump::repaint screen.
       * Don't output::flush the buffers on a pipe!
       */
      clear_buffers();
      /* FALLTHRU */
    case A_REPAINT:
      /*
       * Repaint screen.
       */
      cmd_exec();
      jump::repaint();
      break;

    case A_GOLINE:
      /*
       * Go to line N, default beginning of file.
       */
      if (number <= 0)
        number = 1;
      cmd_exec();
      jump::jump_back(number);
      break;

    case A_PERCENT:
      /*
       * Go to a specified os::percentage into the file.
       */
      if (number < 0) {
        number   = 0;
        fraction = 0;
      }
      if (number > 100 || (number == 100 && fraction != 0)) {
        number   = 100;
        fraction = 0;
      }
      cmd_exec();
      jump::jump_percent((int)number, fraction);
      break;

    case A_GOEND:
      /*
       * Go to line N, default end of file.
       */
      cmd_exec();
      if (number <= 0)
        jump::jump_forw();
      else
        jump::jump_back(number);
      break;

    case A_GOEND_BUF:
      /*
       * Go to line N, default last buffered byte.
       */
      cmd_exec();
      if (number <= 0)
        jump::jump_forw_buffered();
      else
        jump::jump_back(number);
      break;

    case A_GOPOS:
      /*
       * Go to a specified byte position in the file.
       */
      cmd_exec();
      if (number < 0)
        number = 0;
      jump::jump_line_loc(number, jump_sline);
      break;

    case A_STAT:
      /*
       * Print file name, etc.
       */
      if (ch::getflags() & CH_HELPFILE)
        break;
      cmd_exec();
      parg.p_string = prompt::eq_message();
      output::error((char*)"%s", parg);
      break;

    case A_VERSION:
      /*
       * Print version number, without the "@(#)".
       */
      cmd_exec();
      dispversion();
      break;

    case A_QUIT:
      /*
       * Exit.
       */
      if (ifile::getCurrentIfile() != nullptr && ch::getflags() & CH_HELPFILE) {
        /*
         * Quit while viewing the help file
         * just means return to viewing the
         * previous file.
         */
        hshift  = save_hshift;
        bs_mode = save_bs_mode;
        if (edit::edit_prev(1) == 0)
          break;
      }
      if (extra != nullptr)
        utils::quit(*extra);
      utils::quit(QUIT_OK);
      break;

/*
 * Define abbreviation for a commonly used sequence below.
 */
#define DO_SEARCH() \
  if (number <= 0)  \
    number = 1;     \
  mca_search();     \
  cmd_exec();       \
  multi_search((char*)nullptr, (int)number, 0);

    case A_F_SEARCH:
      /*
       * Search forward for a pattern.
       * Get the first char of the pattern.
       */
      search_type = SRCH_FORW;
      if (number <= 0)
        number = 1;
      mca_search();
      c = getcc();
      debug::debug("getcc 1477");
      goto again;

    case A_B_SEARCH:
      /*
       * Search backward for a pattern.
       * Get the first char of the pattern.
       */
      search_type = SRCH_BACK;
      if (number <= 0)
        number = 1;
      mca_search();
      c = getcc();
      debug::debug("getcc 1489");
      goto again;

    case A_FILTER:
#if HILITE_SEARCH
      search_type = SRCH_FORW | SRCH_FILTER;
      mca_search();
      c = getcc();
      debug::debug("getcc 1496");
      goto again;
#else
      output::error("Command not available", NULL_PARG);
      break;
#endif

    case A_AGAIN_SEARCH:
      /*
       * Repeat previous search.
       */
      DO_SEARCH();
      break;

    case A_T_AGAIN_SEARCH:
      /*
       * Repeat previous search, multiple files.
       */
      search_type |= SRCH_PAST_EOF;
      DO_SEARCH();
      break;

    case A_REVERSE_SEARCH:
      /*
       * Repeat previous search, in reverse direction.
       */
      save_search_type = search_type;
      search_type      = SRCH_REVERSE(search_type);
      DO_SEARCH();
      search_type = save_search_type;
      break;

    case A_T_REVERSE_SEARCH:
      /*
       * Repeat previous search,
       * multiple files in reverse direction.
       */
      save_search_type = search_type;
      search_type      = SRCH_REVERSE(search_type);
      search_type |= SRCH_PAST_EOF;
      DO_SEARCH();
      search_type = save_search_type;
      break;

    case A_UNDO_SEARCH:
      /*
       * Clear search string highlighting.
       */
      search::undo_search();
      break;

    case A_HELP:
      /*
       * Help.
       */
      if (ch::getflags() & CH_HELPFILE)
        break;
      cmd_exec();
      save_hshift  = hshift;
      hshift       = 0;
      save_bs_mode = bs_mode;
      bs_mode      = BS_SPECIAL;
      (void)edit::edit(FAKE_HELPFILE);
      break;

    case A_EXAMINE:
      /*
       * Edit a new file.  Get the filename.
       */
#if EXAMINE
      start_mca(A_EXAMINE, "Examine: ", ml_examine, 0);
      c = getcc();
      debug::debug("getcc 1567");
      goto again;

#endif
      output::error((char*)"Command not available", NULL_PARG);
      break;

    case A_VISUAL:
      /*
       * Invoke an editor on the input file.
       */
#if EDITOR
      if (ch::getflags() & CH_HELPFILE)
        break;
      if (strcmp(ifile::getCurrentIfile()->getFilename(), "-") == 0) {
        output::error((char*)"Cannot edit standard input", NULL_PARG);
        break;
      }
      if (ifile::getCurrentIfile()->getAltfilename() != nullptr) {
        output::error((char*)"WARNING: This file was viewed via LESSOPEN", NULL_PARG);
      }
      start_mca(A_SHELL, "!", ml_shell, 0);
      /*
       * Expand the editor prototype string
       * and pass it to the system to execute.
       * (Make sure the screen is displayed so the
       * expansion of "+%lm" works.)
       */
      make_display();
      cmd_exec();
      lsystem::lsystem(prompt::pr_expand(editproto, 0), (char*)nullptr);
      break;

#endif
      output::error((char*)"Command not available", NULL_PARG);
      break;

    case A_NEXT_FILE:
      /*
       * Examine next file.
       */
#if TAGS
      if (tags::ntags()) {
        output::error((char*)"No next file", NULL_PARG);
        break;
      }
#endif
      if (number <= 0)
        number = 1;
      if (edit::edit_next((int)number)) {
        if (option::get_quit_at_eof() && forwback::eof_displayed() && !(ch::getflags() & CH_HELPFILE))
          utils::quit(QUIT_OK);
        parg.p_string = (number > 1) ? (char*)"(N-th) " : (char*)"";
        output::error((char*)"No %snext file", parg);
      }
      break;

    case A_PREV_FILE:
      /*
       * Examine previous file.
       */
#if TAGS
      if (tags::ntags()) {
        output::error((char*)"No previous file", NULL_PARG);
        break;
      }
#endif
      if (number <= 0)
        number = 1;
      if (edit::edit_prev((int)number)) {
        parg.p_string = (number > 1) ? (char*)"(N-th) " : (char*)"";
        output::error((char*)"No %sprevious file", parg);
      }
      break;

    case A_NEXT_TAG:
      /*
       * Jump to the next tag in the current tag list.
       */
#if TAGS
      if (number <= 0)
        number = 1;
      tagfile = tags::nexttag((int)number);
      if (tagfile == nullptr) {
        output::error((char*)"No next tag", NULL_PARG);
        break;
      }
      cmd_exec();
      if (edit::edit(tagfile) == 0) {
        position_t pos = tags::tagsearch();
        if (pos != NULL_POSITION)
          jump::jump_loc(pos, jump_sline);
      }
#else
      output::error((char*)"Command not available", NULL_PARG);
#endif
      break;

    case A_PREV_TAG:
      /*
       * Jump to the previous tag in the current tag list.
       */
#if TAGS
      if (number <= 0)
        number = 1;
      tagfile = tags::prevtag((int)number);
      if (tagfile == nullptr) {
        output::error((char*)"No previous tag", NULL_PARG);
        break;
      }
      cmd_exec();
      if (edit::edit(tagfile) == 0) {
        position_t pos = tags::tagsearch();
        if (pos != NULL_POSITION)
          jump::jump_loc(pos, jump_sline);
      }
#else
      output::error((char*)"Command not available", NULL_PARG);
#endif
      break;

    case A_INDEX_FILE:
      /*
       * Examine a particular file.
       */
      if (number <= 0)
        number = 1;
      if (edit::edit_index((int)number))
        output::error((char*)"No such file", NULL_PARG);
      break;

    case A_REMOVE_FILE:
      /*
       * Remove a file from the input file list.
       */
      if (ch::getflags() & CH_HELPFILE)
        break;
      old_ifile = ifile::getCurrentIfile();
      new_ifile = ifile::getOffIfile(ifile::getCurrentIfile());
      if (new_ifile == nullptr) {
        screen::bell();
        break;
      }
      if (edit::edit_ifile(new_ifile) != 0) {
        edit::reedit_ifile(old_ifile);
        break;
      }
      ifile::deleteIfile(old_ifile);
      break;

    case A_OPT_TOGGLE:
      /*
       * Change the setting of an  option.
       */
      optflag    = option::OPT_TOGGLE;
      optgetname = false;
      mca_opt_toggle();
      c = getcc();
      debug::debug("getcc 1736");

      cbuf = option::opt_toggle_disallowed(c);
      if (cbuf != nullptr) {
        debug::debug("cbuf is : ", cbuf);
        output::error(cbuf, NULL_PARG);
        break;
      }
      goto again;

    case A_DISP_OPTION:
      /*
       * Report the setting of an option.
       */
      optflag    = option::OPT_NO_TOGGLE;
      optgetname = false;
      mca_opt_toggle();
      c = getcc();
      debug::debug("getcc 1754");
      goto again;

    case A_FIRSTCMD:
      /*
       * Set an initial command for new files.
       */
      start_mca(A_FIRSTCMD, "+", (void*)nullptr, 0);
      c = getcc();
      debug::debug("getcc 1762");
      goto again;

    case A_SHELL:
      /*
       * Shell escape.
       */
#if SHELL_ESCAPE

      start_mca(A_SHELL, "!", ml_shell, 0);
      c = getcc();
      debug::debug("getcc 1772");
      goto again;

#endif
      output::error((char*)"Command not available", NULL_PARG);
      break;

    case A_SETMARK:
    case A_SETMARKBOT:
      /*
       * Set a mark.
       */
      if (ch::getflags() & CH_HELPFILE)
        break;
      start_mca(A_SETMARK, "set mark: ", (void*)nullptr, 0);
      c = getcc();
      debug::debug("getcc 1787");
      if (is_erase_char(c) || is_newline_char(c))
        break;
      mark::setmark(c, action == A_SETMARKBOT ? BOTTOM : TOP);
      jump::repaint();
      break;

    case A_CLRMARK:
      /*
       * Clear a mark.
       */
      start_mca(A_CLRMARK, "screen::clear mark: ", (void*)nullptr, 0);
      c = getcc();
      debug::debug("getcc 1799");
      if (is_erase_char(c) || is_newline_char(c))
        break;
      mark::clrmark(c);
      jump::repaint();
      break;

    case A_GOMARK:
      /*
       * Jump to a marked position.
       */
      start_mca(A_GOMARK, "goto mark: ", (void*)nullptr, 0);
      c = getcc();
      debug::debug("getcc 1811");
      if (is_erase_char(c) || is_newline_char(c))
        break;
      cmd_exec();
      mark::gomark(c);
      break;

    case A_PIPE:
      /*
       * Write part of the input to a pipe to a shell command.
       */
#if PIPEC
      start_mca(A_PIPE, "|mark: ", (void*)nullptr, 0);
      c = getcc();
      if (is_erase_char(c))
        break;
      if (is_newline_char(c))
        c = '.';
      if (mark::badmark(c))
        break;
      pipec = c;
      start_mca(A_PIPE, "!", ml_shell, 0);
      c = getcc();
      debug::debug("getcc 1833");
      goto again;
#endif
      output::error((char*)"Command not available", NULL_PARG);
      break;

    case A_B_BRACKET:
    case A_F_BRACKET:
      start_mca(action, "Brackets: ", (void*)nullptr, 0);
      c = getcc();
      debug::debug("getcc 1842");
      goto again;

    case A_LSHIFT:
      /*
       * Shift view left.
       */
      if (number > 0)
        shift_count = number;
      else
        number = (shift_count > 0) ? shift_count : sc_width / 2;
      if (number > hshift)
        number = hshift;
      hshift -= number;
      screen_trashed = TRASHED;
      break;

    case A_RSHIFT:
      /*
       * Shift view right.
       */
      if (number > 0)
        shift_count = number;
      else
        number = (shift_count > 0) ? shift_count : sc_width / 2;
      hshift += number;
      screen_trashed = TRASHED;
      break;

    case A_LLSHIFT:
      /*
       * Shift view left to margin.
       */
      hshift         = 0;
      screen_trashed = TRASHED;
      break;

    case A_RRSHIFT:
      /*
       * Shift view right to view rightmost char on screen.
       */
      hshift         = line::rrshift();
      screen_trashed = TRASHED;
      break;

    case A_PREFIX:
      /*
       * The command is incomplete (more chars are needed).
       * Display the current char, so the user knows
       * what's going on, and get another character.
       */
      if (mca != A_PREFIX) {
        cmdbuf::cmd_reset();
        start_mca(A_PREFIX, " ", (void*)nullptr, CF_QUIT_ON_ERASE);
        (void)cmdbuf::cmd_char(c);
      }
      c = getcc();
      debug::debug("getcc 1899");
      goto again;

    case A_NOACTION:
      break;

    default:
      screen::bell();
      break;
    }
  }
}
} // namespace command