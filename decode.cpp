/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Routines to decode user commands.
 *
 * This is all table driven.
 * A command table is a sequence of command descriptors.
 * Each command descriptor is a sequence of bytes with the following format:
 *    <c1><c2>...<cN><0><action>
 * The characters c1,c2,...,cN are the command string; that is,
 * the characters which the user must type.
 * It is terminated by a null <0> byte.
 * The byte after the null byte is the action code associated
 * with the command string.
 * If an action byte is OR-ed with A_EXTRA, this indicates
 * that the option byte is followed by an extra string.
 *
 * There may be many command tables.
 * The first (default) table is built-in.
 * Other tables are read in from "lesskey" files.
 * All the tables are linked together and are searched in order.
 */

#include "decode.hpp"
#include "cmd.hpp"
#include "command.hpp"
#include "filename.hpp"
#include "forwback.hpp"
#include "less.hpp"
#include "lesskey.hpp"
#include "mark.hpp"
#include "option.hpp"
#include "output.hpp"
#include "screen.hpp"
#include "utils.hpp"

// TODO: move these to namespaces
extern int erase_char, erase2_char, kill_char;
extern int mousecap;
extern int sc_height;

namespace decode {

#define SK(k) SK_SPECIAL_KEY, (k), 6, 1, 1, 1
/*
 * Command table is ordered roughly according to expected
 * frequency of use, so the common commands are near the beginning.
 */

// clang-format off
static unsigned char cmdtable[] =
{
    '\r',              0, A_F_LINE,
    '\n',              0,A_F_LINE,
    'e',               0,A_F_LINE,
    'j',               0,A_F_LINE,
    SK(SK_DOWN_ARROW), 0,A_F_LINE,
    control<int>('E'), 0,A_F_LINE,
    control<int>('N'), 0,A_F_LINE,
    'k',               0,A_B_LINE,
    'y',               0,A_B_LINE,
    control<int>('Y'), 0,A_B_LINE,
    SK(SK_CONTROL_K),  0,A_B_LINE,
    control<int>('P'), 0,A_B_LINE,
    SK(SK_UP_ARROW),   0,A_B_LINE,
    'J',               0,A_FF_LINE,
    'K',               0,A_BF_LINE,
    'Y',               0,A_BF_LINE,
    'd',               0,A_F_SCROLL,
    control<int>('D'), 0,A_F_SCROLL,
    'u',               0,A_B_SCROLL,
    control<int>('U'), 0,A_B_SCROLL,
    esc,'[','M',       0,A_X11MOUSE_IN,
    esc,'[','<',       0,A_X116MOUSE_IN,
    ' ',               0,A_F_SCREEN,
    'f',               0,A_F_SCREEN,
    control<int>('F'), 0,A_F_SCREEN,
    control<int>('V'), 0,A_F_SCREEN,
    SK(SK_PAGE_DOWN),  0,A_F_SCREEN,
    'b',               0,A_B_SCREEN,
    control<int>('B'), 0,A_B_SCREEN,
    esc,'v',           0,A_B_SCREEN,
    SK(SK_PAGE_UP),    0,A_B_SCREEN,
    'z',               0,A_F_WINDOW,
    'w',               0,A_B_WINDOW,
    esc,' ',           0,A_FF_SCREEN,
    'F',               0,A_F_FOREVER,
    esc,'F',           0,A_F_UNTIL_HILITE,
    'R',               0,A_FREPAINT,
    'r',               0,A_REPAINT,
    control<int>('R'), 0,A_REPAINT,
    control<int>('L'), 0,A_REPAINT,
    esc,'u',           0,A_UNDO_SEARCH,
    'g',               0,A_GOLINE,
    SK(SK_HOME),       0,A_GOLINE,
    '<',               0,A_GOLINE,
    esc,'<',           0,A_GOLINE,
    'p',               0,A_PERCENT,
    '%',               0,A_PERCENT,
    esc,'[',           0,A_LSHIFT,
    esc,']',           0,A_RSHIFT,
    esc,'(',           0,A_LSHIFT,
    esc,')',           0,A_RSHIFT,
    esc,'{',           0,A_LLSHIFT,
    esc,'}',           0,A_RRSHIFT,
    SK(SK_RIGHT_ARROW),0,A_RSHIFT,
    SK(SK_LEFT_ARROW), 0,A_LSHIFT,
    SK(SK_CTL_RIGHT_ARROW),0,A_RRSHIFT,
    SK(SK_CTL_LEFT_ARROW),0,A_LLSHIFT,
    '{',               0,A_F_BRACKET|A_EXTRA,'{','}',0,
    '}',               0,A_B_BRACKET|A_EXTRA,'{','}',0,
    '(',               0,A_F_BRACKET|A_EXTRA,'(',')',0,
    ')',               0,A_B_BRACKET|A_EXTRA,'(',')',0,
    '[',               0,A_F_BRACKET|A_EXTRA,'[',']',0,
    ']',               0,A_B_BRACKET|A_EXTRA,'[',']',0,
    esc,control<int>('F'), 0,A_F_BRACKET,
    esc,control<int>('B'), 0,A_B_BRACKET,
    'G',               0,A_GOEND,
    esc,'G',           0,A_GOEND_BUF,
    esc,'>',           0,A_GOEND,
    '>',               0,A_GOEND,
    SK(SK_END),        0,A_GOEND,
    'P',               0,A_GOPOS,
    '0',               0,A_DIGIT,
    '1',               0,A_DIGIT,
    '2',               0,A_DIGIT,
    '3',               0,A_DIGIT,
    '4',               0,A_DIGIT,
    '5',               0,A_DIGIT,
    '6',               0,A_DIGIT,
    '7',               0,A_DIGIT,
    '8',               0,A_DIGIT,
    '9',               0,A_DIGIT,
    '.',               0,A_DIGIT,
    '=',               0,A_STAT,
    control<int>('G'), 0,A_STAT,
    ':','f',           0,A_STAT,
    '/',               0,A_F_SEARCH,
    '?',               0,A_B_SEARCH,
    esc,'/',           0,A_F_SEARCH|A_EXTRA,'*',0,
    esc,'?',           0,A_B_SEARCH|A_EXTRA,'*',0,
    'n',               0,A_AGAIN_SEARCH,
    esc,'n',           0,A_T_AGAIN_SEARCH,
    'N',               0,A_REVERSE_SEARCH,
    esc,'N',           0,A_T_REVERSE_SEARCH,
    '&',               0,A_FILTER,
    'm',               0,A_SETMARK,
    'M',               0,A_SETMARKBOT,
    esc,'m',           0,A_CLRMARK,
    '\'',              0,A_GOMARK,
    control<int>('X'),control<int>('X'),0,A_GOMARK,
    'E',               0,A_EXAMINE,
    ':','e',           0,A_EXAMINE,
    control<int>('X'),control<int>('V'),0,A_EXAMINE,
    ':','n',           0,A_NEXT_FILE,
    ':','p',           0,A_PREV_FILE,
    't',               0,A_NEXT_TAG,
    'T',               0,A_PREV_TAG,
    ':','x',           0,A_INDEX_FILE,
    ':','d',           0,A_REMOVE_FILE,
    '-',               0,A_OPT_TOGGLE,
    ':','t',           0,A_OPT_TOGGLE|A_EXTRA,'t',0,
    's',               0,A_OPT_TOGGLE|A_EXTRA,'o',0,
    '_',               0,A_DISP_OPTION,
    '|',               0,A_PIPE,
    'v',               0,A_VISUAL,
    '!',               0,A_SHELL,
    '+',               0,A_FIRSTCMD,
    'H',               0,A_HELP,
    'h',               0,A_HELP,
    SK(SK_F1),         0,A_HELP,
    'V',               0,A_VERSION,
    'q',               0,A_QUIT,
    'Q',               0,A_QUIT,
    ':','q',           0,A_QUIT,
    ':','Q',           0,A_QUIT,
    'Z','Z',           0,A_QUIT
};

static unsigned char edittable[] =
{
    '\t',                  0, EC_F_COMPLETE,    /* TAB */
    '\17',                 0, EC_B_COMPLETE,    /* BACKTAB */
    SK(SK_BACKTAB),        0, EC_B_COMPLETE,    /* BACKTAB */
    esc,'\t',              0, EC_B_COMPLETE,    /* esc TAB */
    control<int>('L'),     0, EC_EXPAND,        /* CTRL-L */
    control<int>('V'),     0, EC_LITERAL,       /* BACKSLASH */
    control<int>('A'),     0, EC_LITERAL,       /* BACKSLASH */
    esc,'l',               0, EC_RIGHT,         /* esc l */
    SK(SK_RIGHT_ARROW),    0, EC_RIGHT,         /* RIGHTARROW */
    esc,'h',               0, EC_LEFT,          /* esc h */
    SK(SK_LEFT_ARROW),     0, EC_LEFT,          /* LEFTARROW */
    esc,'b',               0, EC_W_LEFT,        /* esc b */
    esc,SK(SK_LEFT_ARROW), 0, EC_W_LEFT,        /* esc LEFTARROW */
    SK(SK_CTL_LEFT_ARROW), 0, EC_W_LEFT,        /* CTRL-LEFTARROW */
    esc,'w',               0, EC_W_RIGHT,       /* esc w */
    esc,SK(SK_RIGHT_ARROW),0, EC_W_RIGHT,       /* esc RIGHTARROW */
    SK(SK_CTL_RIGHT_ARROW),0, EC_W_RIGHT,       /* CTRL-RIGHTARROW */
    esc,'i',               0, EC_INSERT,        /* esc i */
    SK(SK_INSERT),         0, EC_INSERT,        /* INSERT */
    esc,'x',               0, EC_DELETE,        /* esc x */
    SK(SK_DELETE),         0, EC_DELETE,        /* DELETE */
    esc,'X',               0, EC_W_DELETE,      /* esc X */
    esc,SK(SK_DELETE),     0, EC_W_DELETE,      /* esc DELETE */
    SK(SK_CTL_DELETE),     0, EC_W_DELETE,      /* CTRL-DELETE */
    SK(SK_CTL_BACKSPACE),  0, EC_W_BACKSPACE,   /* CTRL-BACKSPACE */
    esc,'\b',              0, EC_W_BACKSPACE,   /* esc BACKSPACE */
    esc,'0',               0, EC_HOME,          /* esc 0 */
    SK(SK_HOME),           0, EC_HOME,          /* HOME */
    esc,'$',               0, EC_END,           /* esc $ */
    SK(SK_END),            0, EC_END,           /* END */
    esc,'k',               0, EC_UP,            /* esc k */
    SK(SK_UP_ARROW),       0, EC_UP,            /* UPARROW */
    esc,'j',               0, EC_DOWN,          /* esc j */
    SK(SK_DOWN_ARROW),     0, EC_DOWN,          /* DOWNARROW */
    control<int>('G'),     0, EC_ABORT,         /* CTRL-G */
};
// clang-format on
/*
 * Structure to support a list of command tables.
 */
struct tablelist {
  struct tablelist* t_next;
  char*             t_start;
  char*             t_end;
};

/*
 * List of command tables and list of line-edit tables.
 */
static struct tablelist* list_fcmd_tables   = NULL;
static struct tablelist* list_ecmd_tables   = NULL;
static struct tablelist* list_var_tables    = NULL;
static struct tablelist* list_sysvar_tables = NULL;

/*
 * Expand special key abbreviations in a command table.
 */
static void expand_special_keys(char* table, int len)
{
  char* fm;
  char* to;
  int   a;
  char* repl;
  int   klen;

  for (fm = table; fm < table + len;) {
    /*
     * Rewrite each command in the table with any
     * special key abbreviations expanded.
     */
    for (to = fm; *fm != '\0';) {
      if (*fm != SK_SPECIAL_KEY) {
        *to++ = *fm++;
        continue;
      }
      /*
       * After SK_SPECIAL_KEY, next byte is the type
       * of special key (one of the SK_* contants),
       * and the byte after that is the number of bytes,
       * N, reserved by the abbreviation (including the
       * SK_SPECIAL_KEY and key type bytes).
       * Replace all N bytes with the actual bytes
       * output by the special key on this terminal.
       */
      repl = screen::special_key_str(fm[1]);
      klen = fm[2] & 0377;
      fm += klen;
      if (repl == NULL || (int)strlen(repl) > klen)
        repl = (char*)"\377";
      while (*repl != '\0')
        *to++ = *repl++;
    }
    *to++ = '\0';
    /*
     * Fill any unused bytes between end of command and
     * the action byte with A_SKIP.
     */
    while (to <= fm)
      *to++ = A_SKIP;
    fm++;
    a = *fm++ & 0377;
    if (a & A_EXTRA) {
      while (*fm++ != '\0')
        continue;
    }
  }
}

/*
 * Expand special key abbreviations in a list of command tables.
 */
static void expand_cmd_table(struct tablelist* tlist)
{
  struct tablelist* t;
  for (t = tlist; t != NULL; t = t->t_next) {
    expand_special_keys(t->t_start, t->t_end - t->t_start);
  }
}

/*
 * Expand special key abbreviations in all command tables.
 */

void expand_cmd_tables(void)
{
  expand_cmd_table(list_fcmd_tables);
  expand_cmd_table(list_ecmd_tables);
  expand_cmd_table(list_var_tables);
  expand_cmd_table(list_sysvar_tables);
}

/*
 * Initialize the command lists.
 */

void init_cmds(void)
{
  /*
   * Add the default command tables.
   */
  add_fcmd_table((char*)cmdtable, sizeof(cmdtable));
  add_ecmd_table((char*)edittable, sizeof(edittable));
#if USERFILE
  /*
   * For backwards compatibility,
   * try to add tables in the OLD system lesskey file.
   */
#ifdef BINDIR
  add_hometable(NULL, (char*)BINDIR "/.sysless", 1);
#endif
  /*
   * Try to add the tables in the system lesskey file.
   */
  add_hometable((char*)"LESSKEY_SYSTEM", (char*)LESSKEYFILE_SYS, 1);
  /*
   * Try to add the tables in the standard lesskey file "$HOME/.less".
   */
  add_hometable((char*)"LESSKEY", (char*)LESSKEYFILE, 0);
#endif
}

/*
 * Add a command table.
 */
static int add_cmd_table(struct tablelist** tlist, char* buf, int len)
{
  struct tablelist* t;

  if (len == 0)
    return (0);
  /*
   * Allocate a tablelist structure, initialize it,
   * and link it into the list of tables.
   */
  if ((t = (struct tablelist*)calloc(1, sizeof(struct tablelist))) == NULL) {
    return (-1);
  }
  t->t_start = buf;
  t->t_end   = buf + len;
  t->t_next  = *tlist;
  *tlist     = t;
  return (0);
}

/*
 * Add a command table.
 */

void add_fcmd_table(char* buf, int len)
{
  if (add_cmd_table(&list_fcmd_tables, buf, len) < 0)
    output::error((char*)"Warning: some commands disabled", NULL_PARG);
}

/*
 * Add an editing command table.
 */

void add_ecmd_table(char* buf, int len)
{
  if (add_cmd_table(&list_ecmd_tables, buf, len) < 0)
    output::error((char*)"Warning: some edit commands disabled", NULL_PARG);
}

/*
 * Add an environment variable table.
 */
static void add_var_table(struct tablelist** tlist, char* buf, int len)
{
  if (add_cmd_table(tlist, buf, len) < 0)
    output::error((char*)"Warning: environment variables from lesskey file "
                 "unavailable",
        NULL_PARG);
}

/*
 * Return action for a mouse wheel down event.
 */
static int mouse_wheel_down(void)
{
  return ((mousecap == option::OPT_ONPLUS) ? A_B_MOUSE : A_F_MOUSE);
}

/*
 * Return action for a mouse wheel up event.
 */
static int mouse_wheel_up(void)
{
  return ((mousecap == option::OPT_ONPLUS) ? A_F_MOUSE : A_B_MOUSE);
}

/*
 * Return action for a mouse button release event.
 */
static int mouse_button_rel(int x, int y)
{
  /*
   * {{ It would be better to return an action and then do this
   *    in commands() but it's nontrivial to pass y to it. }}
   */
  if (y < sc_height - 1) {
    mark::setmark('#', y);
    screen_trashed = TRASHED;
  }
  return (A_NOACTION);
}

/*
 * Read a decimal integer. Return the integer and set *pterm to the terminating
 * char.
 */
static int getcc_int(char* pterm)
{
  int num    = 0;
  int digits = 0;
  for (;;) {
    char ch = command::getcc();
    if (ch < '0' || ch > '9') {
      if (pterm != NULL)
        *pterm = ch;
      if (digits == 0)
        return (-1);
      return (num);
    }
    num = (10 * num) + (ch - '0');
    ++digits;
  }
}

/*
 * Read suffix of mouse input and return the action to take.
 * The prefix ("\e[M") has already been read.
 */
static int x11mouse_action(void)
{
  int b = command::getcc() - X11MOUSE_OFFSET;
  int x = command::getcc() - X11MOUSE_OFFSET - 1;
  int y = command::getcc() - X11MOUSE_OFFSET - 1;
  switch (b) {
  default:
    return (A_NOACTION);
  case X11MOUSE_WHEEL_DOWN:
    return mouse_wheel_down();
  case X11MOUSE_WHEEL_UP:
    return mouse_wheel_up();
  case X11MOUSE_BUTTON_REL:
    return mouse_button_rel(x, y);
  }
}

/*
 * Read suffix of mouse input and return the action to take.
 * The prefix ("\e[<") has already been read.
 */
static int x116mouse_action(void)
{
  char ch;
  int  x, y;
  int  b = getcc_int(&ch);
  if (b < 0 || ch != ';')
    return (A_NOACTION);
  x = getcc_int(&ch) - 1;
  if (x < 0 || ch != ';')
    return (A_NOACTION);
  y = getcc_int(&ch) - 1;
  if (y < 0)
    return (A_NOACTION);
  switch (b) {
  case X11MOUSE_WHEEL_DOWN:
    return mouse_wheel_down();
  case X11MOUSE_WHEEL_UP:
    return mouse_wheel_up();
  default:
    if (ch != 'm')
      return (A_NOACTION);
    return mouse_button_rel(x, y);
  }
}

/*
 * Search a single command table for the command string in cmd.
 */
static int cmd_search(char* cmd, char* table, char* endtable, char** sp)
{
  char* p;
  char* q;
  int   a;

  *sp = NULL;
  for (p = table, q = cmd; p < endtable; p++, q++) {
    if (*p == *q) {
      /*
       * Current characters match.
       * If we're at the end of the string, we've found it.
       * Return the action code, which is the character
       * after the null at the end of the string
       * in the command table.
       */
      if (*p == '\0') {
        a = *++p & 0377;
        while (a == A_SKIP)
          a = *++p & 0377;
        if (a == A_END_LIST) {
          /*
           * We get here only if the original
           * cmd string passed in was empty ("").
           * I don't think that can happen,
           * but just in case ...
           */
          return (A_UINVALID);
        }
        /*
         * Check for an "extra" string.
         */
        if (a & A_EXTRA) {
          *sp = ++p;
          a &= ~A_EXTRA;
        }
        if (a == A_X11MOUSE_IN)
          a = x11mouse_action();
        else if (a == A_X116MOUSE_IN)
          a = x116mouse_action();
        return (a);
      }
    } else if (*q == '\0') {
      /*
       * Hit the end of the user's command,
       * but not the end of the string in the command table.
       * The user's command is incomplete.
       */
      return (A_PREFIX);
    } else {
      /*
       * Not a match.
       * Skip ahead to the next command in the
       * command table, and reset the pointer
       * to the beginning of the user's command.
       */
      if (*p == '\0' && p[1] == A_END_LIST) {
        /*
         * A_END_LIST is a special marker that tells
         * us to abort the cmd search.
         */
        return (A_UINVALID);
      }
      while (*p++ != '\0')
        continue;
      while (*p == A_SKIP)
        p++;
      if (*p & A_EXTRA)
        while (*++p != '\0')
          continue;
      q = cmd - 1;
    }
  }
  /*
   * No match found in the entire command table.
   */
  return (A_INVALID);
}

/*
 * Decode a command character and return the associated action.
 * The "extra" string, if any, is returned in sp.
 */
static int cmd_decode(struct tablelist* tlist, char* cmd, char** sp)
{
  struct tablelist* t;
  int               action = A_INVALID;

  /*
   * Search thru all the command tables.
   * Stop when we find an action which is not A_INVALID.
   */
  for (t = tlist; t != NULL; t = t->t_next) {
    action = cmd_search(cmd, t->t_start, t->t_end, sp);
    if (action != A_INVALID)
      break;
  }
  if (action == A_UINVALID)
    action = A_INVALID;
  return (action);
}

/*
 * Decode a command from the cmdtables list.
 */

int fcmd_decode(char* cmd, char** sp)
{
  return (cmd_decode(list_fcmd_tables, cmd, sp));
}

/*
 * Decode a command from the edittables list.
 */

int ecmd_decode(char* cmd, char** sp)
{
  return (cmd_decode(list_ecmd_tables, cmd, sp));
}

/*
 * Get the value of an environment variable.
 * Looks first in the lesskey file, then in the real environment.
 */

char* lgetenv(char* var)
{
  int   a;
  char* s;

  a = cmd_decode(list_var_tables, var, &s);
  if (a == EV_OK)
    return (s);
  s = getenv(var);
  if (s != NULL && *s != '\0')
    return (s);
  a = cmd_decode(list_sysvar_tables, var, &s);
  if (a == EV_OK)
    return (s);
  return (NULL);
}

/*
 * Is a string null or empty?
 */

int isnullenv(char* s)
{
  return (s == NULL || *s == '\0');
}

#if USERFILE
/*
 * Get an "integer" from a lesskey file.
 * Integers are stored in a funny format:
 * two bytes, low order first, in radix KRADIX.
 */
static int gint(char** sp)
{
  int n;

  n = *(*sp)++;
  n += *(*sp)++ * lesskey::KRADIX;
  return (n);
}

/*
 * Process an old (pre-v241) lesskey file.
 */
static int old_lesskey(char* buf, int len)
{
  /*
   * Old-style lesskey file.
   * The file must end with either
   *     ...,cmd,0,action
   * or  ...,cmd,0,action|A_EXTRA,string,0
   * So the last byte or the second to last byte must be zero.
   */
  if (buf[len - 1] != '\0' && buf[len - 2] != '\0')
    return (-1);
  add_fcmd_table(buf, len);
  return (0);
}

/*
 * Process a new (post-v241) lesskey file.
 */
static int new_lesskey(char* buf, int len, int sysvar)
{
  char* p;
  int   c;
  int   n;

  /*
   * New-style lesskey file.
   * Extract the pieces.
   */
  if (buf[len - 3] != lesskey::C0_END_LESSKEY_MAGIC || buf[len - 2] != lesskey::C1_END_LESSKEY_MAGIC || buf[len - 1] != lesskey::C2_END_LESSKEY_MAGIC)
    return (-1);
  p = buf + 4;
  for (;;) {
    c = *p++;
    switch (c) {
    case lesskey::CMD_SECTION:
      n = gint(&p);
      add_fcmd_table(p, n);
      p += n;
      break;
    case lesskey::EDIT_SECTION:
      n = gint(&p);
      add_ecmd_table(p, n);
      p += n;
      break;
    case lesskey::VAR_SECTION:
      n = gint(&p);
      add_var_table((sysvar) ? &list_sysvar_tables : &list_var_tables, p, n);
      p += n;
      break;
    case lesskey::END_SECTION:
      return (0);
    default:
      /*
       * Unrecognized section type.
       */
      return (-1);
    }
  }
}

/*
 * Set up a user command table, based on a "lesskey" file.
 */

int lesskey(char* filename, int sysvar)
{
  char*      buf;
  position_t len;
  long       n;
  int        f;

  /*
   * Try to open the lesskey file.
   */
  f = open(filename, OPEN_READ);
  if (f < 0)
    return (1);

  /*
   * Read the file into a buffer.
   * We first figure out the size of the file and allocate space for it.
   * {{ Minimal output::error checking is done here.
   *    A garbage .less file will produce strange results.
   *    To avoid a large amount of output::error checking code here, we
   *    rely on the lesskey program to generate a good .less file. }}
   */
  len = filename::filesize(f);
  if (len == NULL_POSITION || len < 3) {
    /*
     * Bad file (valid file must have at least 3 chars).
     */
    close(f);
    return (-1);
  }
  if ((buf = (char*)calloc((int)len, sizeof(char))) == NULL) {
    close(f);
    return (-1);
  }
  if (lseek(f, (off_t)0, SEEK_SET) == BAD_LSEEK) {
    free(buf);
    close(f);
    return (-1);
  }
  n = read(f, buf, (unsigned int)len);
  close(f);
  if (n != len) {
    free(buf);
    return (-1);
  }

  /*
   * Figure out if this is an old-style (before version 241)
   * or new-style lesskey file format.
   */
  if (buf[0] != lesskey::C0_LESSKEY_MAGIC || buf[1] != lesskey::C1_LESSKEY_MAGIC || buf[2] != lesskey::C2_LESSKEY_MAGIC || buf[3] != lesskey::C3_LESSKEY_MAGIC)
    return (old_lesskey(buf, (int)len));
  return (new_lesskey(buf, (int)len, sysvar));
}

/*
 * Add the standard lesskey file "$HOME/.less"
 */

void add_hometable(char* envname, char* def_filename, int sysvar)
{
  char*  filename;
  parg_t parg;

  if (envname != NULL && (filename = lgetenv(envname)) != NULL)
    filename = utils::save(filename);
  else if (sysvar)
    filename = utils::save(def_filename);
  else
    filename = filename::homefile(def_filename);
  if (filename == NULL)
    return;
  if (lesskey(filename, sysvar) < 0) {
    parg.p_string = filename;
    output::error((char*)"Cannot use lesskey file \"%s\"", parg);
  }
  free(filename);
}
#endif

/*
 * See if a char is a special line-editing command.
 */

int editchar(int c, int flags)
{
  int   action;
  int   nch;
  char* s;
  char  usercmd[MAX_CMDLEN + 1];

  /*
   * An editing character could actually be a sequence of characters;
   * for example, an escape sequence sent by pressing the uparrow key.
   * To match the editing string, we use the command decoder
   * but give it the edit-commands command table
   * This table is constructed to match the user's keyboard.
   */
  if (c == erase_char || c == erase2_char)
    return (EC_BACKSPACE);
  if (c == kill_char) {
    return (EC_LINEKILL);
  }

  /*
   * Collect characters in a buffer.
   * Start with the one we have, and get more if we need them.
   */
  nch = 0;
  do {
    if (nch > 0)
      c = command::getcc();
    usercmd[nch]     = c;
    usercmd[nch + 1] = '\0';
    nch++;
    action = ecmd_decode(usercmd, &s);
  } while (action == A_PREFIX);

  if (flags & EC_NORIGHTLEFT) {
    switch (action) {
    case EC_RIGHT:
    case EC_LEFT:
      action = A_INVALID;
      break;
    }
  }
#if CMD_HISTORY
  if (flags & EC_NOHISTORY) {
    /*
     * The caller says there is no history list.
     * Reject any history-manipulation action.
     */
    switch (action) {
    case EC_UP:
    case EC_DOWN:
      action = A_INVALID;
      break;
    }
  }
#endif
#if TAB_COMPLETE_FILENAME
  if (flags & EC_NOCOMPLETE) {
    /*
     * The caller says we don't want any filename completion cmds.
     * Reject them.
     */
    switch (action) {
    case EC_F_COMPLETE:
    case EC_B_COMPLETE:
    case EC_EXPAND:
      action = A_INVALID;
      break;
    }
  }
#endif
  if ((flags & EC_PEEK) || action == A_INVALID) {
    /*
     * We're just peeking, or we didn't understand the command.
     * Unget all the characters we read in the loop above.
     * This does NOT include the original character that was
     * passed in as a parameter.
     */
    while (nch > 1) {
      command::ungetcc(usercmd[--nch]);
    }
  } else {
    if (s != NULL)
      command::ungetsc(s);
  }
  return action;
}

} // namespace decode