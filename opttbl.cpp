/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * The option table.
 */

#include "decode.hpp"
#include "less.hpp"
#include "optfunc.hpp"
#include "option.hpp"
#include "utils.hpp"

/*
 * Variables controlled by command line options.
 */
// TODO: Look at moving these to a globals/settings package. Also there are
//       bools here that need to be type defined correctly
//       Move to less.hpp Less struct - like follow_mode

int how_search;          /* Where should forward searches start? */
int top_scroll;          /* Repaint screen from top?
                            (alternative is scroll from bottom) */
int  pr_type;            /* Type of prompt (short, medium, long) */
int  bs_mode;            /* How to process backspaces */
int  know_dumb;          /* Don't complain about dumb terminals */
int  quit_if_one_screen; /* Quit if EOF on first screen */
int  squeeze;            /* Squeeze multiple blank lines into one */
int  tabstop;            /* Tab settings */
int  back_scroll;        /* Repaint screen on backwards movement */
int  forw_scroll;        /* Repaint screen on forward movement */
int  caseless;           /* Do "caseless" searches */
int  linenums;           /* Use line numbers */
int  bufspace;           /* Max buffer space per file (K) */
int  ctldisp;            /* Send control chars to screen untranslated */
int  force_open;         /* Open the file even if not regular file */
int  swindow;            /* Size of scrolling window */
int  jump_sline;         /* Screen line of "jump target" */
long jump_sline_fraction  = -1;
long shift_count_fraction = -1;
int  chopline;     /* Truncate displayed lines at screen width */
int  no_init;      /* Disable sending ti/te termcap strings */
int  no_keypad;    /* Disable sending ks/ke termcap strings */
int  twiddle;      /* Show tildes after EOF */
int  show_attn;    /* Hilite first unread line */
int  shift_count;  /* Number of positions to shift horizontally */
int  status_col;   /* Display a status column */
int  use_lessopen; /* Use the LESSOPEN filter */
int  quit_on_intr; /* Quit on interrupt */
int  oldbot;       /* Old bottom of screen behavior {{REMOVE}} */
char rscroll_char; /* Char which marks chopped lines with -S */
int  rscroll_attr; /* Attribute of rscroll_char */
int  no_hist_dups; /* Remove dups from history list */
int  mousecap;     /* Allow mouse for scrolling */
int  wheel_lines;  /* Number of lines to scroll on mouse wheel scroll */
int  perma_marks;  /* Save marks in history file */
#if HILITE_SEARCH
int hilite_search; /* Highlight matched search patterns? */
#endif

namespace opttbl {

using namespace option;

/*
 * Long option names.
 */
static struct optname a_optname  = { (char*)"search-skip-screen", NULL };
static struct optname b_optname  = { (char*)"buffers", NULL };
static struct optname B__optname = { (char*)"auto-buffers", NULL };
static struct optname c_optname  = { (char*)"screen::clear-screen", NULL };
static struct optname d_optname  = { (char*)"dumb", NULL };
static struct optname e_optname  = { (char*)"quit-at-eof", NULL };
static struct optname f_optname  = { (char*)"force", NULL };
static struct optname F__optname = { (char*)"quit-if-one-screen", NULL };
#if HILITE_SEARCH
static struct optname g_optname = { (char*)"hilite-search", NULL };
#endif
static struct optname h_optname  = { (char*)"max-back-scroll", NULL };
static struct optname i_optname  = { (char*)"ignore-case", NULL };
static struct optname j_optname  = { (char*)"jump-target", NULL };
static struct optname J__optname = { (char*)"status-column", NULL };
#if USERFILE
static struct optname k_optname = { (char*)"decode::lesskey-file", NULL };
#endif
static struct optname K__optname = { (char*)"quit-on-intr", NULL };
static struct optname L__optname = { (char*)"no-lessopen", NULL };
static struct optname m_optname  = { (char*)"long-prompt", NULL };
static struct optname n_optname  = { (char*)"line-numbers", NULL };
static struct optname o_optname  = { (char*)"log-file", NULL };
static struct optname O__optname = { (char*)"LOG-FILE", NULL };
static struct optname p_optname  = { (char*)"pattern", NULL };
static struct optname P__optname = { (char*)"prompt", NULL };
static struct optname q2_optname = { (char*)"silent", NULL };
static struct optname q_optname  = { (char*)"quiet", &q2_optname };
static struct optname r_optname  = { (char*)"raw-control-chars", NULL };
static struct optname s_optname  = { (char*)"squeeze-blank-lines", NULL };
static struct optname S__optname = { (char*)"chop-long-lines", NULL };
#if TAGS
static struct optname t_optname  = { (char*)"tag", NULL };
static struct optname T__optname = { (char*)"tag-file", NULL };
#endif
static struct optname u_optname             = { (char*)"underline-special", NULL };
static struct optname V__optname            = { (char*)"version", NULL };
static struct optname w_optname             = { (char*)"hilite-unread", NULL };
static struct optname x_optname             = { (char*)"tabs", NULL };
static struct optname X__optname            = { (char*)"no-init", NULL };
static struct optname y_optname             = { (char*)"max-forwback::forw-scroll", NULL };
static struct optname z_optname             = { (char*)"window", NULL };
static struct optname quote_optname         = { (char*)"quotes", NULL };
static struct optname tilde_optname         = { (char*)"tilde", NULL };
static struct optname query_optname         = { (char*)"help", NULL };
static struct optname pound_optname         = { (char*)"shift", NULL };
static struct optname keypad_optname        = { (char*)"no-keypad", NULL };
static struct optname oldbot_optname        = { (char*)"old-bot", NULL };
static struct optname follow_optname        = { (char*)"follow-name", NULL };
static struct optname use_backslash_optname = { (char*)"use-backslash", NULL };
static struct optname rscroll_optname       = { (char*)"rscroll", NULL };
static struct optname nohistdups_optname    = { (char*)"no-histdups", NULL };
static struct optname mousecap_optname      = { (char*)"mouse", NULL };
static struct optname wheel_lines_optname   = { (char*)"wheel-lines", NULL };
static struct optname perma_marks_optname   = { (char*)"save-marks", NULL };
// clang-format on

/*
 * Table of all options and their semantics.
 *
 * For BOOL and TRIPLE options, odesc[0], odesc[1], odesc[2] are
 * the description of the option when set to 0, 1 or 2, respectively.
 * For NUMBER options, odesc[0] is the prompt to use when entering
 * a new value, and odesc[1] is the description, which should contain
 * one %d which is replaced by the value of the number.
 * For STRING options, odesc[0] is the prompt to use when entering
 * a new value, and odesc[1], if not NULL, is the set of characters
 * that are valid in the string.
 */

static struct option::loption option[] = {
  { 'a', &a_optname,
      TRIPLE, option::OPT_ONPLUS, &how_search, NULL,
      { (char*)"Search includes displayed screen",
          (char*)"Search skips displayed screen",
          (char*)"Search includes all of displayed screen" } },
  { 'b', &b_optname,
      NUMBER | INIT_HANDLER, 64, &bufspace, optfunc::opt_b,
      { (char*)"Max buffer space per file (K): ",
          (char*)"Max buffer space per file: %dK",
          NULL } },
  { 'B', &B__optname,
      BOOL, option::OPT_ON, &less::Globals::autobuf, NULL,
      { (char*)"Don't automatically allocate buffers",
          (char*)"Automatically allocate buffers when needed",
          NULL } },
  { 'c', &c_optname,
      TRIPLE, OPT_OFF, &top_scroll, NULL,
      { (char*)"Repaint by scrolling from bottom of screen",
          (char*)"Repaint by painting from top of screen",
          (char*)"Repaint by painting from top of screen" } },
  { 'd', &d_optname,
      BOOL | NO_TOGGLE, OPT_OFF, &know_dumb, NULL,
      { (char*)"Assume intelligent terminal",
          (char*)"Assume dumb terminal",
          NULL } },
  { 'e', &e_optname,
      TRIPLE,
      OPT_OFF,
      reinterpret_cast<int*>(&option::Option::quit_at_eof), // var ptr
      NULL,
      { (char*)"Don't quit at end-of-file",
          (char*)"Quit at end-of-file",
          (char*)"Quit immediately at end-of-file" } },
  { 'f', &f_optname,
      BOOL, OPT_OFF, &force_open, NULL,
      { (char*)"Open only regular files",
          (char*)"Open even non-regular files",
          NULL } },
  { 'F', &F__optname,
      BOOL, OPT_OFF, &quit_if_one_screen, NULL,
      { (char*)"Don't quit if end-of-file on first screen",
          (char*)"Quit if end-of-file on first screen",
          NULL } },
#if HILITE_SEARCH
  { 'g', &g_optname,
      TRIPLE | HL_REPAINT, option::OPT_ONPLUS, &hilite_search, NULL,
      {
          (char*)"Don't highlight search matches",
          (char*)"Highlight matches for previous search only",
          (char*)"Highlight all matches for previous search pattern",
      } },
#endif
  { 'h', &h_optname,
      NUMBER, -1, &back_scroll, NULL,
      { (char*)"Backwards scroll limit: ",
          (char*)"Backwards scroll limit is %d lines",
          NULL } },
  { 'i', &i_optname,
      TRIPLE | HL_REPAINT, OPT_OFF, &caseless, optfunc::opt_i,
      { (char*)"Case is significant in searches",
          (char*)"Ignore case in searches",
          (char*)"Ignore case in searches and in patterns" } },
  { 'j', &j_optname,
      STRING, 0, NULL, optfunc::opt_j,
      { (char*)"Target line: ",
          (char*)"0123456789.-",
          NULL } },
  { 'J', &J__optname,
      BOOL | REPAINT, OPT_OFF, &status_col, NULL,
      { (char*)"Don't display a status column",
          (char*)"Display a status column",
          NULL } },
#if USERFILE
  { 'k', &k_optname,
      STRING | NO_TOGGLE | NO_QUERY, 0, NULL, optfunc::opt_k,
      { NULL,
          NULL,
          NULL } },
#endif
  { 'K', &K__optname,
      BOOL, OPT_OFF, &quit_on_intr, NULL,
      { (char*)"Interrupt (ctrl-C) returns to prompt",
          (char*)"Interrupt (ctrl-C) exits less",
          NULL } },
  { 'L', &L__optname,
      BOOL, option::OPT_ON, &use_lessopen, NULL,
      { (char*)"Don't use the LESSOPEN filter",
          (char*)"Use the LESSOPEN filter",
          NULL } },
  { 'm', &m_optname,
      TRIPLE, OPT_OFF, &pr_type, NULL,
      { (char*)"Short prompt",
          (char*)"Medium prompt",
          (char*)"Long prompt" } },
  { 'n', &n_optname,
      TRIPLE | REPAINT, option::OPT_ON, &linenums, NULL,
      { (char*)"Don't use line numbers",
          (char*)"Use line numbers",
          (char*)"constly display line numbers" } },
  { 'o', &o_optname,
      STRING, 0, NULL, optfunc::opt_o,
      { (char*)"log file: ",
          NULL,
          NULL } },
  { 'O', &O__optname,
      STRING, 0, NULL, optfunc::opt__O,
      { (char*)"Log file: ",
          NULL,
          NULL } },
  { 'p', &p_optname,
      STRING | NO_TOGGLE | NO_QUERY, 0, NULL, optfunc::opt_p,
      { NULL,
          NULL,
          NULL } },
  { 'P', &P__optname,
      STRING, 0, NULL, optfunc::opt__P,
      { (char*)"prompt: ",
          NULL,
          NULL } },
  { 'q',                                              // oletter
      &q_optname,                                     // optname ptr
      TRIPLE,                                         // otype
      OPT_OFF,                                        // default value
      reinterpret_cast<int*>(&option::Option::quiet), // var ptr
      NULL,                                           // Pointer to special handling function
      {                                               // Description of each value
          (char*)"Ring the screen::bell for errors AND at eof/bof",
          (char*)"Ring the screen::bell for errors but not at eof/bof",
          (char*)"Never ring the screen::bell" } },
  { 'r', &r_optname,
      TRIPLE | REPAINT, OPT_OFF, &ctldisp, NULL,
      { (char*)"Display control characters as ^X",
          (char*)"Display control characters directly",
          (char*)"Display control characters directly, processing ANSI sequences" } },
  { 's', &s_optname,
      BOOL | REPAINT, OPT_OFF, &squeeze, NULL,
      { (char*)"Display all blank lines",
          (char*)"Squeeze multiple blank lines",
          NULL } },
  { 'S', &S__optname,
      BOOL | REPAINT, OPT_OFF, &chopline, NULL,
      { (char*)"Fold long lines",
          (char*)"Chop long lines",
          NULL } },
#if TAGS
  { 't', &t_optname,
      STRING | NO_QUERY, 0, NULL, optfunc::opt_t,
      { (char*)"tag: ", NULL, NULL } },
  { 'T', &T__optname,
      STRING, 0, NULL, optfunc::opt__T,
      { (char*)"tags file: ", NULL, NULL } },
#endif
  { 'u', &u_optname,
      TRIPLE | REPAINT, OPT_OFF, &bs_mode, NULL,
      { (char*)"Display underlined text in underline mode",
          (char*)"Backspaces cause overstrike",
          (char*)"Print backspace as ^H" } },
  { 'V', &V__optname,
      NOVAR, 0, NULL, optfunc::opt__V,
      { NULL, NULL, NULL } },
  { 'w', &w_optname,
      TRIPLE | REPAINT, OPT_OFF, &show_attn, NULL,
      {
          (char*)"Don't highlight first unread line",
          (char*)"Highlight first unread line after forward-screen",
          (char*)"Highlight first unread line after any forward movement",
      } },
  { 'x', &x_optname,
      STRING | REPAINT, 0, NULL, optfunc::opt_x,
      { (char*)"Tab stops: ",
          (char*)"0123456789,",
          NULL } },
  { 'X', &X__optname,
      BOOL | NO_TOGGLE, OPT_OFF, &no_init, NULL,
      { (char*)"Send init/screen::deinit strings to terminal",
          (char*)"Don't use init/screen::deinit strings",
          NULL } },
  { 'y', &y_optname,
      NUMBER, -1, &forw_scroll, NULL,
      { (char*)"Forward scroll limit: ",
          (char*)"Forward scroll limit is %d lines",
          NULL } },
  { 'z', &z_optname,
      NUMBER, -1, &swindow, NULL,
      { (char*)"Scroll window size: ",
          (char*)"Scroll window size is %d lines",
          NULL } },
  { '"', &quote_optname,
      STRING, 0, NULL, optfunc::opt_quote,
      { (char*)"quotes: ",
          NULL,
          NULL } },
  { '~', &tilde_optname,
      BOOL | REPAINT, option::OPT_ON, &twiddle, NULL,
      { (char*)"Don't show tildes after end of file",
          (char*)"Show tildes after end of file",
          NULL } },
  { '?', &query_optname,
      NOVAR, 0, NULL, optfunc::opt_query,
      { NULL, NULL, NULL } },
  { '#', &pound_optname,
      STRING, 0, NULL, optfunc::opt_shift,
      { (char*)"Horizontal shift: ",
          (char*)"0123456789.",
          NULL } },
  { OLETTER_NONE, &keypad_optname,
      BOOL | NO_TOGGLE, OPT_OFF, &no_keypad, NULL,
      { (char*)"Use keypad mode",
          (char*)"Don't use keypad mode",
          NULL } },
  { OLETTER_NONE, &oldbot_optname,
      BOOL, OPT_OFF, &oldbot, NULL,
      { (char*)"Use new bottom of screen behavior",
          (char*)"Use old bottom of screen behavior",
          NULL } },
  { OLETTER_NONE, &follow_optname,
      BOOL, FOLLOW_DESC, &less::Globals::follow_mode, NULL,
      { (char*)"F command follows file descriptor",
          (char*)"F command follows file name",
          NULL } },
  { OLETTER_NONE, &use_backslash_optname,
      BOOL,
      OPT_OFF,
      reinterpret_cast<int*>(&option::Option::opt_use_backslash),
      NULL,
      { (char*)"Use backslash escaping in command line parameters",
          (char*)"Don't use backslash escaping in command line parameters",
          NULL } },
  { OLETTER_NONE, &rscroll_optname,
      STRING | REPAINT | INIT_HANDLER, 0, NULL, optfunc::opt_rscroll,
      { (char*)"right scroll character: ",
          NULL,
          NULL } },
  { OLETTER_NONE, &nohistdups_optname,
      BOOL, OPT_OFF, &no_hist_dups, NULL,
      { (char*)"Allow duplicates in history list",
          (char*)"Remove duplicates from history list",
          NULL } },
  { OLETTER_NONE, &mousecap_optname,
      TRIPLE, OPT_OFF, &mousecap, optfunc::opt_mousecap,
      { (char*)"Ignore mouse input",
          (char*)"Use the mouse for scrolling",
          (char*)"Use the mouse for scrolling (reverse)" } },
  { OLETTER_NONE, &wheel_lines_optname,
      NUMBER | INIT_HANDLER, 0, &wheel_lines, optfunc::opt_wheel_lines,
      { (char*)"Lines to scroll on mouse wheel: ",
          (char*)"Scroll %d line(s) on mouse wheel",
          NULL } },
  { OLETTER_NONE, &perma_marks_optname,
      BOOL, OPT_OFF, &perma_marks, NULL,
      { (char*)"Don't save marks in history file",
          (char*)"Save marks in history file",
          NULL } },
  { '\0', NULL,
      NOVAR, 0, NULL, NULL,
      { NULL,
          NULL,
          NULL } }
};

/*
 * Initialize each option to its default value.
 */

void init_option(void)
{
  struct option::loption* o;
  char*                   p;

  p = decode::lgetenv((char*)"LESS_IS_MORE");
  if (!decode::isnullenv(p))
    option::Option::less_is_more = 1;

  for (o = option; o->oletter != '\0'; o++) {
    /*
     * Set each variable to its default.
     */
    if (o->ovar != NULL)
      *(o->ovar) = o->odefault;
    if (o->otype & INIT_HANDLER)
      (*(o->ofunc))(option::INIT, (char*)NULL);
  }
}

/*
 * Find an option in the option table, given its option letter.
 */

struct option::loption* findopt(int c)
{
  struct option::loption* o;

  for (o = option; o->oletter != '\0'; o++) {
    if (o->oletter == c)
      return (o);
    if ((o->otype & TRIPLE) && toupper(o->oletter) == c)
      return (o);
  }
  return (NULL);
}

/*
 *
 */
static int is_optchar(char c)
{
  int retval = 0;

  if (iswupper(c))
    retval = 1;
  if (iswlower(c))
    retval = 1;
  if (c == '-')
    retval = 1;
  return retval;
}

/*
 * Find an option in the option table, given its option name.
 * optname_ptr is the (possibly partial) name to look for, and
 * is updated to point after the matched name.
 * p_oname if non-NULL is set to point to the full option name.
 */
struct option::loption* findopt_name(char** optname_ptr, char** oname_ptr, int* err)
{
  char*                   optname = *optname_ptr;
  struct option::loption* o;
  struct option::optname* oname;
  int                     len;
  int                     uppercase;
  struct option::loption* maxo     = NULL;
  struct option::optname* maxoname = NULL;
  int                     maxlen   = 0;
  int                     ambig    = 0;
  int                     exact    = 0;

  /*
   * Check all options.
   */
  for (o = option; o->oletter != '\0'; o++) {
    /*
     * Check all names for this option.
     */
    for (oname = o->onames; oname != NULL; oname = oname->onext) {
      /*
       * Try normal match first (uppercase == 0),
       * then, then if it's a TRIPLE option,
       * try uppercase match (uppercase == 1).
       */
      for (uppercase = 0; uppercase <= 1; uppercase++) {
        len = utils::sprefix(optname, oname->oname, uppercase);
        if (len <= 0 || is_optchar(optname[len])) {
          /*
           * We didn't use all of the option name.
           */
          continue;
        }
        if (!exact && len == maxlen)
          /*
           * Already had a partial match,
           * and now there's another one that
           * matches the same length.
           */
          ambig = 1;
        else if (len > maxlen) {
          /*
           * Found a better match than
           * the one we had.
           */
          maxo     = o;
          maxoname = oname;
          maxlen   = len;
          ambig    = 0;
          exact    = (len == (int)strlen(oname->oname));
        }
        if (!(o->otype & TRIPLE))
          break;
      }
    }
  }
  if (ambig) {
    /*
     * Name matched more than one option.
     */
    if (err != nullptr)
      *err = option::OPT_AMBIG;
    return (nullptr);
  }
  *optname_ptr = optname + maxlen;
  if (oname_ptr != nullptr)
    *oname_ptr = maxoname == nullptr ? nullptr : maxoname->oname;
  return (maxo);
}

} // namespace opttbl