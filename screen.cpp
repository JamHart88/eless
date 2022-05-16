/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Routines which deal with the characteristics of the terminal.
 * Uses termcap to be as terminal-independent as possible.
 */

#include "cmd.hpp"
#include "less.hpp"
#include "decode.hpp"

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_TERMIOS_H && HAVE_TERMIOS_FUNCS
#include <termios.h>
#else
#if HAVE_TERMIO_H
#include <termio.h>
#else
#if HAVE_SGSTAT_H
#include <sgstat.h>
#else
#include <sgtty.h>
#endif
#endif
#endif

#if HAVE_TERMCAP_H
#include <termcap.h>
#endif
#if HAVE_SYS_STREAM_H
#include <sys/stream.h>
#endif
#if HAVE_SYS_PTEM_H
#include <sys/ptem.h>
#endif

/*
 * Check for broken termios package that forces you to manually
 * set the line discipline.
 */
#ifdef __ultrix__
#define MUST_SET_LINE_DISCIPLINE 1
#else
#define MUST_SET_LINE_DISCIPLINE 0
#endif

#define DEFAULT_TERM "unknown"

/*
 * Strings passed to tputs() to do various terminal functions.
 */
static char *sc_pad, /* Pad string */
    *sc_home,        /* Cursor home */
    *sc_addline,     /* Add line, scroll down following lines */
    *sc_lower_left,  /* Cursor to last line, first column */
    *sc_return,      /* Cursor to beginning of current line */
    *sc_move,        /* General cursor positioning */
    *sc_clear,       /* Clear screen */
    *sc_eol_clear,   /* Clear to end of line */
    *sc_eos_clear,   /* Clear to end of screen */
    *sc_s_in,        /* Enter standout (highlighted) mode */
    *sc_s_out,       /* Exit standout mode */
    *sc_u_in,        /* Enter underline mode */
    *sc_u_out,       /* Exit underline mode */
    *sc_b_in,        /* Enter bold mode */
    *sc_b_out,       /* Exit bold mode */
    *sc_bl_in,       /* Enter blink mode */
    *sc_bl_out,      /* Exit blink mode */
    *sc_visual_bell, /* Visual bell (flash screen) sequence */
    *sc_backspace,   /* Backspace cursor */
    *sc_s_keypad,    /* Start keypad mode */
    *sc_e_keypad,    /* End keypad mode */
    *sc_s_mousecap,  /* Start mouse capture mode */
    *sc_e_mousecap,  /* End mouse capture mode */
    *sc_init,        /* Startup terminal initialization */
    *sc_deinit;      /* Exit terminal de-initialization */

static int init_done = 0;

public
int auto_wrap; /* Terminal does \r\n when write past margin */
public
int ignaw; /* Terminal ignores \n immediately after wrap */
public
int erase_char; /* The user's erase char */
public
int erase2_char; /* The user's other erase char */
public
int kill_char; /* The user's line-kill char */
public
int werase_char; /* The user's word-erase char */
public
int sc_width, sc_height; /* Height & width of screen */
public
int bo_s_width, bo_e_width; /* Printing width of boldface seq */
public
int ul_s_width, ul_e_width; /* Printing width of underline seq */
public
int so_s_width, so_e_width; /* Printing width of standout seq */
public
int bl_s_width, bl_e_width; /* Printing width of blink seq */
public
int above_mem, below_mem; /* Memory retained above/below screen */
public
int can_goto_line; /* Can move cursor to any line */
public
int clear_bg; /* Clear fills with background color */
public
int missing_cap = 0; /* Some capability is missing */
public
char *kent = NULL; /* Keypad ENTER sequence */

static int attrmode = AT_NORMAL;
static int termcap_debug = -1;
extern int binattr;
extern int one_screen;

static char *cheaper LESSPARAMS((char *t1, char *t2, char *def));
static void tmodes LESSPARAMS((char *incap, char *outcap, char **instr,
                               char **outstr, char *def_instr, char *def_outstr,
                               char **spp));

/*
 * These two variables are sometimes defined in,
 * and needed by, the termcap library.
 */
#if MUST_DEFINE_OSPEED
extern short ospeed; /* Terminal output baud rate */
extern char PC;      /* Pad character */
#endif

extern int quiet; /* If VERY_QUIET, use visual bell for bell */
extern int no_back_scroll;
extern int swindow;
extern int no_init;
extern int no_keypad;
extern int sigs;
extern int wscroll;
extern int screen_trashed;
extern int top_scroll;
extern int quit_if_one_screen;
extern int oldbot;
extern int mousecap;
#if HILITE_SEARCH
extern int hilite_search;
#endif
extern int tty;

extern char *tgetstr();
extern char *tgoto();

/*
 * Change terminal to "raw mode", or restore to "normal" mode.
 * "Raw mode" means
 *    1. An outstanding read will complete on receipt of a single keystroke.
 *    2. Input is not echoed.
 *    3. On output, \n is mapped to \r\n.
 *    4. \t is NOT expanded into spaces.
 *    5. Signal-causing characters such as ctrl-C (interrupt),
 *       etc. are NOT disabled.
 * It doesn't matter whether an input \n is mapped to \r, or vice versa.
 */
public
void raw_mode(int on) {
  static int curr_on = 0;

  if (on == curr_on)
    return;
  erase2_char = '\b'; /* in case OS doesn't know about erase2 */
#if HAVE_TERMIOS_H && HAVE_TERMIOS_FUNCS
  {
    struct termios s;
    static struct termios save_term;
    static int saved_term = 0;

    if (on) {
      /*
       * Get terminal modes.
       */
      tcgetattr(tty, &s);

      /*
       * Save modes and set certain variables dependent on modes.
       */
      if (!saved_term) {
        save_term = s;
        saved_term = 1;
      }
#if HAVE_OSPEED
      switch (cfgetospeed(&s)) {
#ifdef B0
      case B0:
        ospeed = 0;
        break;
#endif
#ifdef B50
      case B50:
        ospeed = 1;
        break;
#endif
#ifdef B75
      case B75:
        ospeed = 2;
        break;
#endif
#ifdef B110
      case B110:
        ospeed = 3;
        break;
#endif
#ifdef B134
      case B134:
        ospeed = 4;
        break;
#endif
#ifdef B150
      case B150:
        ospeed = 5;
        break;
#endif
#ifdef B200
      case B200:
        ospeed = 6;
        break;
#endif
#ifdef B300
      case B300:
        ospeed = 7;
        break;
#endif
#ifdef B600
      case B600:
        ospeed = 8;
        break;
#endif
#ifdef B1200
      case B1200:
        ospeed = 9;
        break;
#endif
#ifdef B1800
      case B1800:
        ospeed = 10;
        break;
#endif
#ifdef B2400
      case B2400:
        ospeed = 11;
        break;
#endif
#ifdef B4800
      case B4800:
        ospeed = 12;
        break;
#endif
#ifdef B9600
      case B9600:
        ospeed = 13;
        break;
#endif
#ifdef EXTA
      case EXTA:
        ospeed = 14;
        break;
#endif
#ifdef EXTB
      case EXTB:
        ospeed = 15;
        break;
#endif
#ifdef B57600
      case B57600:
        ospeed = 16;
        break;
#endif
#ifdef B115200
      case B115200:
        ospeed = 17;
        break;
#endif
      default:;
      }
#endif
      erase_char = s.c_cc[VERASE];
#ifdef VERASE2
      erase2_char = s.c_cc[VERASE2];
#endif
      kill_char = s.c_cc[VKILL];
#ifdef VWERASE
      werase_char = s.c_cc[VWERASE];
#else
      werase_char = CONTROL('W');
#endif

      /*
       * Set the modes to the way we want them.
       */
      s.c_lflag &= ~(0
#ifdef ICANON
                     | ICANON
#endif
#ifdef ECHO
                     | ECHO
#endif
#ifdef ECHOE
                     | ECHOE
#endif
#ifdef ECHOK
                     | ECHOK
#endif
#if ECHONL
                     | ECHONL
#endif
      );

      s.c_oflag |= (0
#ifdef OXTABS
                    | OXTABS
#else
#ifdef TAB3
                    | TAB3
#else
#ifdef XTABS
                    | XTABS
#endif
#endif
#endif
#ifdef OPOST
                    | OPOST
#endif
#ifdef ONLCR
                    | ONLCR
#endif
      );

      s.c_oflag &= ~(0
#ifdef ONOEOT
                     | ONOEOT
#endif
#ifdef OCRNL
                     | OCRNL
#endif
#ifdef ONOCR
                     | ONOCR
#endif
#ifdef ONLRET
                     | ONLRET
#endif
      );
      s.c_cc[VMIN] = 1;
      s.c_cc[VTIME] = 0;
#ifdef VLNEXT
      s.c_cc[VLNEXT] = 0;
#endif
#ifdef VDSUSP
      s.c_cc[VDSUSP] = 0;
#endif
#if MUST_SET_LINE_DISCIPLINE
      /*
       * System's termios is broken; need to explicitly
       * request TERMIODISC line discipline.
       */
      s.c_line = TERMIODISC;
#endif
    } else {
      /*
       * Restore saved modes.
       */
      s = save_term;
    }
#if HAVE_FSYNC
    fsync(tty);
#endif
    tcsetattr(tty, TCSADRAIN, &s);
#if MUST_SET_LINE_DISCIPLINE
    if (!on) {
      /*
       * Broken termios *ignores* any line discipline
       * except TERMIODISC.  A different old line discipline
       * is therefore not restored, yet.  Restore the old
       * line discipline by hand.
       */
      ioctl(tty, TIOCSETD, &save_term.c_line);
    }
#endif
  }
#else
#ifdef TCGETA
  {
    struct termio s;
    static struct termio save_term;
    static int saved_term = 0;

    if (on) {
      /*
       * Get terminal modes.
       */
      ioctl(tty, TCGETA, &s);

      /*
       * Save modes and set certain variables dependent on modes.
       */
      if (!saved_term) {
        save_term = s;
        saved_term = 1;
      }
#if HAVE_OSPEED
      ospeed = s.c_cflag & CBAUD;
#endif
      erase_char = s.c_cc[VERASE];
      kill_char = s.c_cc[VKILL];
#ifdef VWERASE
      werase_char = s.c_cc[VWERASE];
#else
      werase_char = CONTROL('W');
#endif

      /*
       * Set the modes to the way we want them.
       */
      s.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);
      s.c_oflag |= (OPOST | ONLCR | TAB3);
      s.c_oflag &= ~(OCRNL | ONOCR | ONLRET);
      s.c_cc[VMIN] = 1;
      s.c_cc[VTIME] = 0;
    } else {
      /*
       * Restore saved modes.
       */
      s = save_term;
    }
    ioctl(tty, TCSETAW, &s);
  }
#else
#ifdef TIOCGETP
  {
    struct sgttyb s;
    static struct sgttyb save_term;
    static int saved_term = 0;

    if (on) {
      /*
       * Get terminal modes.
       */
      ioctl(tty, TIOCGETP, &s);

      /*
       * Save modes and set certain variables dependent on modes.
       */
      if (!saved_term) {
        save_term = s;
        saved_term = 1;
      }
#if HAVE_OSPEED
      ospeed = s.sg_ospeed;
#endif
      erase_char = s.sg_erase;
      kill_char = s.sg_kill;
      werase_char = CONTROL('W');

      /*
       * Set the modes to the way we want them.
       */
      s.sg_flags |= CBREAK;
      s.sg_flags &= ~(ECHO | XTABS);
    } else {
      /*
       * Restore saved modes.
       */
      s = save_term;
    }
    ioctl(tty, TIOCSETN, &s);
  }
#else

  /* MS-DOS, Windows, or OS2 */
  erase_char = '\b';
  kill_char = ESC;
  werase_char = CONTROL('W');
#endif
#endif
#endif
  curr_on = on;
}

/*
 * Some glue to prevent calling termcap functions if tgetent() failed.
 */
static int hardcopy;

static char *ltget_env(char *capname) {
  char name[64];

  if (termcap_debug) {
    struct env {
      struct env *next;
      char *name;
      char *value;
    };
    static struct env *envs = NULL;
    struct env *p;
    for (p = envs; p != NULL; p = p->next)
      if (strcmp(p->name, capname) == 0)
        return p->value;
    p = (struct env *)ecalloc(1, sizeof(struct env));
    p->name = save(capname);
    p->value = (char *)ecalloc(strlen(capname) + 3, sizeof(char));
    sprintf(p->value, "<%s>", capname);
    p->next = envs;
    envs = p;
    return p->value;
  }
  SNPRINTF1(name, sizeof(name), "LESS_TERMCAP_%s", capname);
  return (lgetenv(name));
}

static int ltgetflag(char *capname) {
  char *s;

  if ((s = ltget_env(capname)) != NULL)
    return (*s != '\0' && *s != '0');
  if (hardcopy)
    return (0);
  return (tgetflag(capname));
}

static int ltgetnum(char *capname) {
  char *s;

  if ((s = ltget_env(capname)) != NULL)
    return (atoi(s));
  if (hardcopy)
    return (-1);
  return (tgetnum(capname));
}

static char *ltgetstr(char *capname, char **pp) {
  char *s;

  if ((s = ltget_env(capname)) != NULL)
    return (s);
  if (hardcopy)
    return (NULL);
  return (tgetstr(capname, pp));
}

/*
 * Get size of the output screen.
 */
public
void scrsize(VOID_PARAM) {
  char *s;
  int sys_height;
  int sys_width;
  int n;

#define DEF_SC_WIDTH 80
#define DEF_SC_HEIGHT 24

  sys_width = sys_height = 0;

#ifdef TIOCGWINSZ
  {
    struct winsize w;
    if (ioctl(2, TIOCGWINSZ, &w) == 0) {
      if (w.ws_row > 0)
        sys_height = w.ws_row;
      if (w.ws_col > 0)
        sys_width = w.ws_col;
    }
  }
#else
#ifdef WIOCGETD
  {
    struct uwdata w;
    if (ioctl(2, WIOCGETD, &w) == 0) {
      if (w.uw_height > 0)
        sys_height = w.uw_height / w.uw_vs;
      if (w.uw_width > 0)
        sys_width = w.uw_width / w.uw_hs;
    }
  }
#endif
#endif

  if (sys_height > 0)
    sc_height = sys_height;
  else if ((s = lgetenv((char *)"LINES")) != NULL)
    sc_height = atoi(s);
  else if ((n = ltgetnum((char *)"li")) > 0)
    sc_height = n;
  if (sc_height <= 0)
    sc_height = DEF_SC_HEIGHT;

  if (sys_width > 0)
    sc_width = sys_width;
  else if ((s = lgetenv((char *)"COLUMNS")) != NULL)
    sc_width = atoi(s);
  else if ((n = ltgetnum((char *)"co")) > 0)
    sc_width = n;
  if (sc_width <= 0)
    sc_width = DEF_SC_WIDTH;
}

/*
 * Return the characters actually input by a "special" key.
 */
public
char *special_key_str(int key) {
  static char tbuf[40];
  char *s;

  char *sp = tbuf;

  switch (key) {

  case SK_RIGHT_ARROW:
    s = ltgetstr((char *)"kr", &sp);
    break;
  case SK_LEFT_ARROW:
    s = ltgetstr((char *)"kl", &sp);
    break;
  case SK_UP_ARROW:
    s = ltgetstr((char *)"ku", &sp);
    break;
  case SK_DOWN_ARROW:
    s = ltgetstr((char *)"kd", &sp);
    break;
  case SK_PAGE_UP:
    s = ltgetstr((char *)"kP", &sp);
    break;
  case SK_PAGE_DOWN:
    s = ltgetstr((char *)"kN", &sp);
    break;
  case SK_HOME:
    s = ltgetstr((char *)"kh", &sp);
    break;
  case SK_END:
    s = ltgetstr((char *)"@7", &sp);
    break;
  case SK_DELETE:
    s = ltgetstr((char *)"kD", &sp);
    if (s == NULL) {
      tbuf[0] = '\177';
      tbuf[1] = '\0';
      s = tbuf;
    }
    break;
  case SK_CONTROL_K:
    tbuf[0] = CONTROL('K');
    tbuf[1] = '\0';
    s = tbuf;
    break;
  default:
    return (NULL);
  }
  return (s);
}

/*
 * Get terminal capabilities via termcap.
 */
public
void get_term(VOID_PARAM) {
  termcap_debug = !isnullenv(lgetenv((char *)"LESS_TERMCAP_DEBUG"));

  {
    char *sp;
    char *t1, *t2;
    char *term;
    /*
     * Some termcap libraries assume termbuf is static
     * (accessible after tgetent returns).
     */
    static char termbuf[TERMBUF_SIZE];
    static char sbuf[TERMSBUF_SIZE];

    /*
     * Find out what kind of terminal this is.
     */
    if ((term = lgetenv((char *)"TERM")) == NULL)
      term = (char *)DEFAULT_TERM;
    hardcopy = 0;
    /* {{ Should probably just pass NULL instead of termbuf. }} */
    if (tgetent(termbuf, term) != TGETENT_OK)
      hardcopy = 1;
    if (ltgetflag((char *)"hc"))
      hardcopy = 1;

    /*
     * Get size of the screen.
     */
    scrsize();
    pos_init();

    auto_wrap = ltgetflag((char *)"am");
    ignaw = ltgetflag((char *)"xn");
    above_mem = ltgetflag((char *)"da");
    below_mem = ltgetflag((char *)"db");
    clear_bg = ltgetflag((char *)"ut");

    /*
     * Assumes termcap variable "sg" is the printing width of:
     * the standout sequence, the end standout sequence,
     * the underline sequence, the end underline sequence,
     * the boldface sequence, and the end boldface sequence.
     */
    if ((so_s_width = ltgetnum((char *)"sg")) < 0)
      so_s_width = 0;
    so_e_width = so_s_width;

    bo_s_width = bo_e_width = so_s_width;
    ul_s_width = ul_e_width = so_s_width;
    bl_s_width = bl_e_width = so_s_width;

#if HILITE_SEARCH
    if (so_s_width > 0 || so_e_width > 0)
      /*
       * Disable highlighting by default on magic cookie terminals.
       * Turning on highlighting might change the displayed width
       * of a line, causing the display to get messed up.
       * The user can turn it back on with -g,
       * but she won't like the results.
       */
      hilite_search = 0;
#endif

    /*
     * Get various string-valued capabilities.
     */
    sp = sbuf;

#if HAVE_OSPEED
    sc_pad = ltgetstr((char *)"pc", &sp);
    if (sc_pad != NULL)
      PC = *sc_pad;
#endif

    sc_s_keypad = ltgetstr((char *)"ks", &sp);
    if (sc_s_keypad == NULL)
      sc_s_keypad = (char *)"";
    sc_e_keypad = ltgetstr((char *)"ke", &sp);
    if (sc_e_keypad == NULL)
      sc_e_keypad = (char *)"";
    kent = ltgetstr((char *)"@8", &sp);

    sc_s_mousecap = ltgetstr((char *)"MOUSE_START", &sp);
    if (sc_s_mousecap == NULL)
      sc_s_mousecap = (char *)ESCS "[?1000h" ESCS "[?1006h";
    sc_e_mousecap = ltgetstr((char *)"MOUSE_END", &sp);
    if (sc_e_mousecap == NULL)
      sc_e_mousecap = (char *)ESCS "[?1006l" ESCS "[?1000l";

    sc_init = ltgetstr((char *)"ti", &sp);
    if (sc_init == NULL)
      sc_init = (char *)"";

    sc_deinit = ltgetstr((char *)"te", &sp);
    if (sc_deinit == NULL)
      sc_deinit = (char *)"";

    sc_eol_clear = ltgetstr((char *)"ce", &sp);
    if (sc_eol_clear == NULL || *sc_eol_clear == '\0') {
      missing_cap = 1;
      sc_eol_clear = (char *)"";
    }

    sc_eos_clear = ltgetstr((char *)"cd", &sp);
    if (below_mem && (sc_eos_clear == NULL || *sc_eos_clear == '\0')) {
      missing_cap = 1;
      sc_eos_clear = (char *)"";
    }

    sc_clear = ltgetstr((char *)"cl", &sp);
    if (sc_clear == NULL || *sc_clear == '\0') {
      missing_cap = 1;
      sc_clear = (char *)"\n\n";
    }

    sc_move = ltgetstr((char *)"cm", &sp);
    if (sc_move == NULL || *sc_move == '\0') {
      /*
       * This is not an error here, because we don't
       * always need sc_move.
       * We need it only if we don't have home or lower-left.
       */
      sc_move = (char *)"";
      can_goto_line = 0;
    } else
      can_goto_line = 1;

    tmodes((char *)"so", (char *)"se", &sc_s_in, &sc_s_out, (char *)"",
           (char *)"", &sp);
    tmodes((char *)"us", (char *)"ue", &sc_u_in, &sc_u_out, sc_s_in, sc_s_out,
           &sp);
    tmodes((char *)"md", (char *)"me", &sc_b_in, &sc_b_out, sc_s_in, sc_s_out,
           &sp);
    tmodes((char *)"mb", (char *)"me", &sc_bl_in, &sc_bl_out, sc_s_in, sc_s_out,
           &sp);

    sc_visual_bell = ltgetstr((char *)"vb", &sp);
    if (sc_visual_bell == NULL)
      sc_visual_bell = (char *)"";

    if (ltgetflag((char *)"bs"))
      sc_backspace = (char *)"\b";
    else {
      sc_backspace = ltgetstr((char *)"bc", &sp);
      if (sc_backspace == NULL || *sc_backspace == '\0')
        sc_backspace = (char *)"\b";
    }

    /*
     * Choose between using "ho" and "cm" ("home" and "cursor move")
     * to move the cursor to the upper left corner of the screen.
     */
    t1 = ltgetstr((char *)"ho", &sp);
    if (t1 == NULL)
      t1 = (char *)"";
    if (*sc_move == '\0')
      t2 = (char *)"";
    else {
      strcpy(sp, tgoto(sc_move, 0, 0));
      t2 = sp;
      sp += strlen(sp) + 1;
    }
    sc_home = cheaper(t1, t2, (char *)"|\b^");

    /*
     * Choose between using "ll" and "cm"  ("lower left" and "cursor move")
     * to move the cursor to the lower left corner of the screen.
     */
    t1 = ltgetstr((char *)"ll", &sp);
    if (t1 == NULL)
      t1 = (char *)"";
    if (*sc_move == '\0')
      t2 = (char *)"";
    else {
      strcpy(sp, tgoto(sc_move, 0, sc_height - 1));
      t2 = sp;
      sp += strlen(sp) + 1;
    }
    sc_lower_left = cheaper(t1, t2, (char *)"\r");

    /*
     * Get carriage return string.
     */
    sc_return = ltgetstr((char *)"cr", &sp);
    if (sc_return == NULL)
      sc_return = (char *)"\r";

    /*
     * Choose between using "al" or "sr" ("add line" or "scroll reverse")
     * to add a line at the top of the screen.
     */
    t1 = ltgetstr((char *)"al", &sp);
    if (t1 == NULL)
      t1 = (char *)"";
    t2 = ltgetstr((char *)"sr", &sp);
    if (t2 == NULL)
      t2 = (char *)"";
    if (above_mem)
      sc_addline = t1;
    else
      sc_addline = cheaper(t1, t2, (char *)"");
    if (*sc_addline == '\0') {
      /*
       * Force repaint on any backward movement.
       */
      no_back_scroll = 1;
    }
  }
}

/*
 * Return the cost of displaying a termcap string.
 * We use the trick of calling tputs, but as a char printing function
 * we give it inc_costcount, which just increments "costcount".
 * This tells us how many chars would be printed by using this string.
 * {{ Couldn't we just use strlen? }}
 */
static int costcount;

/*ARGSUSED*/
static int inc_costcount(int c) {
  costcount++;
  return (c);
}

static int cost(char *t) {
  costcount = 0;
  tputs(t, sc_height, inc_costcount);
  return (costcount);
}

/*
 * Return the "best" of the two given termcap strings.
 * The best, if both exist, is the one with the lower
 * cost (see cost() function).
 */
static char *cheaper(char *t1, char *t2, char *def) {
  if (*t1 == '\0' && *t2 == '\0') {
    missing_cap = 1;
    return (def);
  }
  if (*t1 == '\0')
    return (t2);
  if (*t2 == '\0')
    return (t1);
  if (cost(t1) < cost(t2))
    return (t1);
  return (t2);
}

static void tmodes(char *incap, char *outcap, char **instr, char **outstr,
                   char *def_instr, char *def_outstr, char **spp) {
  *instr = ltgetstr(incap, spp);
  if (*instr == NULL) {
    /* Use defaults. */
    *instr = def_instr;
    *outstr = def_outstr;
    return;
  }

  *outstr = ltgetstr(outcap, spp);
  if (*outstr == NULL)
    /* No specific out capability; use "me". */
    *outstr = ltgetstr((char *)"me", spp);
  if (*outstr == NULL)
    /* Don't even have "me"; use a null string. */
    *outstr = (char *)"";
}

/*
 * Below are the functions which perform all the
 * terminal-specific screen manipulation.
 */

/*
 * Configure the termimal so mouse clicks and wheel moves
 * produce input to less.
 */
public
void init_mouse(VOID_PARAM) {
  if (!mousecap)
    return;
  tputs(sc_s_mousecap, sc_height, putchr);
}

/*
 * Configure the terminal so mouse clicks and wheel moves
 * are handled by the system (so text can be selected, etc).
 */
public
void deinit_mouse(VOID_PARAM) {
  if (!mousecap)
    return;
  tputs(sc_e_mousecap, sc_height, putchr);
}

/*
 * Initialize terminal
 */
public
void init(VOID_PARAM) {
  if (!(quit_if_one_screen && one_screen)) {
    if (!no_init)
      tputs(sc_init, sc_height, putchr);
    if (!no_keypad)
      tputs(sc_s_keypad, sc_height, putchr);
    init_mouse();
  }
  if (top_scroll) {
    int i;

    /*
     * This is nice to terminals with no alternate screen,
     * but with saved scrolled-off-the-top lines.  This way,
     * no previous line is lost, but we start with a whole
     * screen to ourself.
     */
    for (i = 1; i < sc_height; i++)
      putchr('\n');
  } else
    line_left();
  init_done = 1;
}

/*
 * Deinitialize terminal
 */
public
void deinit(VOID_PARAM) {
  if (!init_done)
    return;
  if (!(quit_if_one_screen && one_screen)) {
    deinit_mouse();
    if (!no_keypad)
      tputs(sc_e_keypad, sc_height, putchr);
    if (!no_init)
      tputs(sc_deinit, sc_height, putchr);
  }
  init_done = 0;
}

/*
 * Home cursor (move to upper left corner of screen).
 */
public
void home(VOID_PARAM) { tputs(sc_home, 1, putchr); }

/*
 * Add a blank line (called with cursor at home).
 * Should scroll the display down.
 */
public
void add_line(VOID_PARAM) { tputs(sc_addline, sc_height, putchr); }

/*
 * Move cursor to lower left corner of screen.
 */
public
void lower_left(VOID_PARAM) {
  if (!init_done)
    return;
  tputs(sc_lower_left, 1, putchr);
}

/*
 * Move cursor to left position of current line.
 */
public
void line_left(VOID_PARAM) { tputs(sc_return, 1, putchr); }

/*
 * Check if the console size has changed and reset internals
 * (in lieu of SIGWINCH for WIN32).
 */
public
void check_winch(VOID_PARAM) {}

/*
 * Goto a specific line on the screen.
 */
public
void goto_line(int sindex) { tputs(tgoto(sc_move, 0, sindex), 1, putchr); }

/*
 * Output the "visual bell", if there is one.
 */
public
void vbell(VOID_PARAM) {
  if (*sc_visual_bell == '\0')
    return;
  tputs(sc_visual_bell, sc_height, putchr);
}

/*
 * Make a noise.
 */
static void beep(VOID_PARAM) { putchr(CONTROL('G')); }

/*
 * Ring the terminal bell.
 */
public
void bell(VOID_PARAM) {
  if (quiet == VERY_QUIET)
    vbell();
  else
    beep();
}

/*
 * Clear the screen.
 */
public
void clear(VOID_PARAM) { tputs(sc_clear, sc_height, putchr); }

/*
 * Clear from the cursor to the end of the cursor's line.
 * {{ This must not move the cursor. }}
 */
public
void clear_eol(VOID_PARAM) { tputs(sc_eol_clear, 1, putchr); }

/*
 * Clear the current line.
 * Clear the screen if there's off-screen memory below the display.
 */
static void clear_eol_bot(VOID_PARAM) {
  if (below_mem)
    tputs(sc_eos_clear, 1, putchr);
  else
    tputs(sc_eol_clear, 1, putchr);
}

/*
 * Clear the bottom line of the display.
 * Leave the cursor at the beginning of the bottom line.
 */
public
void clear_bot(VOID_PARAM) {
  /*
   * If we're in a non-normal attribute mode, temporarily exit
   * the mode while we do the clear.  Some terminals fill the
   * cleared area with the current attribute.
   */
  if (oldbot)
    lower_left();
  else
    line_left();

  if (attrmode == AT_NORMAL)
    clear_eol_bot();
  else {
    int saved_attrmode = attrmode;

    at_exit();
    clear_eol_bot();
    at_enter(saved_attrmode);
  }
}

public
void at_enter(int attr) {
  attr = apply_at_specials(attr);

  /* The one with the most priority is last.  */
  if (attr & AT_UNDERLINE)
    tputs(sc_u_in, 1, putchr);
  if (attr & AT_BOLD)
    tputs(sc_b_in, 1, putchr);
  if (attr & AT_BLINK)
    tputs(sc_bl_in, 1, putchr);
  if (attr & AT_STANDOUT)
    tputs(sc_s_in, 1, putchr);
  attrmode = attr;
}

public
void at_exit(VOID_PARAM) {
  /* Undo things in the reverse order we did them.  */
  if (attrmode & AT_STANDOUT)
    tputs(sc_s_out, 1, putchr);
  if (attrmode & AT_BLINK)
    tputs(sc_bl_out, 1, putchr);
  if (attrmode & AT_BOLD)
    tputs(sc_b_out, 1, putchr);
  if (attrmode & AT_UNDERLINE)
    tputs(sc_u_out, 1, putchr);
  attrmode = AT_NORMAL;
}

public
void at_switch(int attr) {
  int new_attrmode = apply_at_specials(attr);
  int ignore_modes = AT_ANSI;

  if ((new_attrmode & ~ignore_modes) != (attrmode & ~ignore_modes)) {
    at_exit();
    at_enter(attr);
  }
}

public
int is_at_equiv(int attr1, int attr2) {
  attr1 = apply_at_specials(attr1);
  attr2 = apply_at_specials(attr2);

  return (attr1 == attr2);
}

public
int apply_at_specials(int attr) {
  if (attr & AT_BINARY)
    attr |= binattr;
  if (attr & AT_HILITE)
    attr |= AT_STANDOUT;
  attr &= ~(AT_BINARY | AT_HILITE);

  return attr;
}

/*
 * Output a plain backspace, without erasing the previous char.
 */
public
void putbs(VOID_PARAM) {
  if (termcap_debug)
    putstr((char *)"<bs>");
  else {
    tputs(sc_backspace, 1, putchr);
  }
}
