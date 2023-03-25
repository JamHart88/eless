/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Handling functions for command line options.
 *
 * Most options are handled by the generic code in option.c.
 * But all string options, and a few non-string options, require
 * special handling specific to the particular option.
 * This special processing is done by the "handling functions" in this file.
 *
 * Each handling function is passed a "type" and, if it is a string
 * option, the string which should be "assigned" to the option.
 * The type may be one of:
 *    option::INIT      The option is being initialized from the command line.
 *    option::TOGGLE    The option is being changed from within the program.
 *    option::QUERY     The setting of the option is merely being queried.
 */

#include "optfunc.hpp"
#include "ch.hpp"
#include "charset.hpp"
#include "command.hpp"
#include "decode.hpp"
#include "edit.hpp"
#include "filename.hpp"
#include "jump.hpp"
#include "less.hpp"
#include "option.hpp"
#include "output.hpp"
#include "screen.hpp"
#include "search.hpp"
#include "tags.hpp"
#include "ttyin.hpp"
#include "utils.hpp"

extern int   nbufs;
extern int   bufspace;
extern int   pr_type;
extern bool  plusoption;
extern int   swindow;
extern int   sc_width;
extern int   sc_height;
extern int   dohelp;
extern bool  any_display;
extern char* prproto[];
extern char* eqproto;
extern char* hproto;
extern char* wproto;
extern char* every_first_cmd;
extern char  version[];
extern int   jump_sline;
extern long  jump_sline_fraction;
extern int   shift_count;
extern long  shift_count_fraction;
extern char  rscroll_char;
extern int   rscroll_attr;
extern int   mousecap;
extern int   wheel_lines;
extern int   tabstops[];
extern int   ntabstops;
extern int   tabdefault;

#if TAGS
char*        tagoption = NULL;
extern char* tags_ptr;
extern char  ztags[];
#endif

namespace optfunc {

/*
 * Handler for -o option.
 */
void opt_o(int type, char* s)
{
  parg_t parg;
  char*  filename;

  switch (type) {
  case option::INIT:
    less::Globals::namelogfile = utils::save(s);
    break;
  case option::TOGGLE:
    if (ch::getflags() & CH_CANSEEK) {
      output::error((char*)"Input is not a pipe", NULL_PARG);
      return;
    }
    if (less::Globals::logfile >= 0) {
      output::error((char*)"Log file is already in use", NULL_PARG);
      return;
    }
    s = utils::skipsp(s);
    if (less::Globals::namelogfile != NULL)
      free(less::Globals::namelogfile);
    filename                   = filename::lglob(s);
    less::Globals::namelogfile = filename::shell_unquote(filename);
    free(filename);
    edit::use_logfile(less::Globals::namelogfile);
    ch::sync_logfile();
    break;
  case option::QUERY:
    if (less::Globals::logfile < 0)
      output::error((char*)"No log file", NULL_PARG);
    else {
      parg.p_string = less::Globals::namelogfile;
      output::error((char*)"Log file \"%s\"", parg);
    }
    break;
  }
}

/*
 * Handler for -O option.
 */
void opt__O(int type, char* s)
{
  less::Globals::force_logfile = true;
  opt_o(type, s);
}

/*
 * Handlers for -j option.
 */
void opt_j(int type, char* s)
{
  parg_t parg;
  char   buf[30]; // Make bigger than Long int in chars
  int    len;
  int    err;

  switch (type) {
  case option::INIT:
  case option::TOGGLE:
    if (*s == '.') {
      s++;
      jump_sline_fraction = option::getfraction(&s, (char*)"j", &err);
      if (err)
        output::error((char*)"Invalid line fraction", NULL_PARG);
      else
        calc_jump_sline();
    } else {
      int sline = option::getnum(&s, (char*)"j", &err);
      if (err)
        output::error((char*)"Invalid line number", NULL_PARG);
      else {
        jump_sline          = sline;
        jump_sline_fraction = -1;
      }
    }
    break;
  case option::QUERY:
    if (jump_sline_fraction < 0) {
      parg.p_int = jump_sline;
      output::error((char*)"Position target at screen line %d", parg);
    } else {

      sprintf(buf, ".%06ld", jump_sline_fraction);
      len = (int)strlen(buf);
      while (len > 2 && buf[len - 1] == '0')
        len--;
      buf[len]      = '\0';
      parg.p_string = buf;
      output::error((char*)"Position target at screen position %s", parg);
    }
    break;
  }
}

void calc_jump_sline(void)
{
  if (jump_sline_fraction < 0)
    return;
  jump_sline = sc_height * jump_sline_fraction / NUM_FRAC_DENOM;
}

/*
 * Handlers for -# option.
 */
void opt_shift(int type, char* s)
{
  parg_t parg;
  char   buf[30]; // Make bigger than Long int in chars
  int    len;
  int    err;

  switch (type) {
  case option::INIT:
  case option::TOGGLE:
    if (*s == '.') {
      s++;
      shift_count_fraction = option::getfraction(&s, (char*)"#", &err);
      if (err)
        output::error((char*)"Invalid column fraction", NULL_PARG);
      else
        calc_shift_count();
    } else {
      int hs = option::getnum(&s, (char*)"#", &err);
      if (err)
        output::error((char*)"Invalid column number", NULL_PARG);
      else {
        shift_count          = hs;
        shift_count_fraction = -1;
      }
    }
    break;
  case option::QUERY:
    if (shift_count_fraction < 0) {
      parg.p_int = shift_count;
      output::error((char*)"Horizontal shift %d columns", parg);
    } else {

      sprintf(buf, ".%06ld", shift_count_fraction);
      len = (int)strlen(buf);
      while (len > 2 && buf[len - 1] == '0')
        len--;
      buf[len]      = '\0';
      parg.p_string = buf;
      output::error((char*)"Horizontal shift %s of screen width", parg);
    }
    break;
  }
}

void calc_shift_count(void)
{
  if (shift_count_fraction < 0)
    return;
  shift_count = sc_width * shift_count_fraction / NUM_FRAC_DENOM;
}

#if USERFILE
void opt_k(int type, char* s)
{
  parg_t parg;

  switch (type) {
  case option::INIT:
    if (decode::lesskey(s, 0)) {
      parg.p_string = s;
      output::error((char*)"Cannot use decode::lesskey file \"%s\"", parg);
    }
    break;
  }
}
#endif

#if TAGS
/*
 * Handler for -t option.
 */
void opt_t(int type, char* s)
{
  ifile::Ifile* save_ifile;
  position_t    pos;

  switch (type) {
  case option::INIT:
    tagoption = utils::save(s);
    /* Do the rest in main() */
    break;
  case option::TOGGLE:

    tags::findtag(utils::skipsp(s));
    save_ifile = edit::save_curr_ifile();
    /*
     * Try to open the file containing the tag
     * and search for the tag in that file.
     */
    if (tags::edit_tagfile() || (pos = tags::tagsearch()) == NULL_POSITION) {
      /* Failed: reopen the old file. */
      edit::reedit_ifile(save_ifile);
      break;
    }
    edit::unsave_ifile(save_ifile);
    jump::jump_loc(pos, jump_sline);
    break;
  }
}

/*
 * Handler for -T option.
 */
void opt__T(int type, char* s)
{
  parg_t parg;
  char*  filename;

  switch (type) {
  case option::INIT:
    tags_ptr = utils::save(s);
    break;
  case option::TOGGLE:
    s = utils::skipsp(s);
    if (tags_ptr != NULL && tags_ptr != ztags)
      free(tags_ptr);
    filename = filename::lglob(s);
    tags_ptr     = filename::shell_unquote(filename);
    free(filename);
    break;
  case option::QUERY:
    parg.p_string = tags_ptr;
    output::error((char*)"Tags file \"%s\"", parg);
    break;
  }
}
#endif

/*
 * Handler for -p option.
 */
void opt_p(int type, char* s)
{
  switch (type) {
  case option::INIT:
    /*
     * Unget a command for the specified string.
     */
    if (option::Option::less_is_more) {
      /*
       * In "more" mode, the -p argument is a command,
       * not a search string, so we don't need a slash.
       */
      every_first_cmd = utils::save(s);
    } else {
      option::Option::plusoption = true;
      command::ungetcc(CHAR_END_COMMAND);
      command::ungetsc(s);
      /*
       * {{ This won't work if the "/" command is
       *    changed or invalidated by a .decode::lesskey file. }}
       */
      command::ungetsc((char*)"/");
    }
    break;
  }
}

/*
 * Handler for -P option.
 */
void opt__P(int type, char* s)
{
  char** proto;
  parg_t parg;

  switch (type) {
  case option::INIT:
  case option::TOGGLE:
    /*
     * Figure out which prototype string should be changed.
     */
    switch (*s) {
    case 's':
      proto = &prproto[PR_SHORT];
      s++;
      break;
    case 'm':
      proto = &prproto[PR_MEDIUM];
      s++;
      break;
    case 'M':
      proto = &prproto[PR_LONG];
      s++;
      break;
    case '=':
      proto = &eqproto;
      s++;
      break;
    case 'h':
      proto = &hproto;
      s++;
      break;
    case 'w':
      proto = &wproto;
      s++;
      break;
    default:
      proto = &prproto[PR_SHORT];
      break;
    }
    free(*proto);
    *proto = utils::save(s);
    break;
  case option::QUERY:
    parg.p_string = prproto[pr_type];
    output::error((char*)"%s", parg);
    break;
  }
}

/*
 * Handler for the -b option.
 */
/*ARGSUSED*/
void opt_b(int type, char* s)
{
  switch (type) {
  case option::INIT:
  case option::TOGGLE:
    /*
     * Set the new number of buffers.
     */
    ch::setbufspace(bufspace);
    break;
  case option::QUERY:
    break;
  }
}

/*
 * Handler for the -i option.
 */
/*ARGSUSED*/
void opt_i(int type, char* s)
{
  switch (type) {
  case option::TOGGLE:
    search::chg_caseless();
    break;
  case option::QUERY:
  case option::INIT:
    break;
  }
}

/*
 * Handler for the -V option.
 */
/*ARGSUSED*/
void opt__V(int type, char* s)
{
  switch (type) {
  case option::TOGGLE:
  case option::QUERY:
    command::dispversion();
    break;
  case option::INIT:
    /*
     * Force output to stdout per GNU standard for --version output.
     */
    any_display = true;
    output::putstr("less ");
    output::putstr(version);
    output::putstr(" (");
    output::putstr(pattern::pattern_lib_name());
    output::putstr(" regular expressions)\n");
    output::putstr("Copyright (C) 1984-2020  Mark Nudelman\n\n");
    output::putstr("less comes with NO WARRANTY, to the extent permitted by law.\n");
    output::putstr("For information about the terms of redistribution,\n");
    output::putstr("see the file named README in the less distribution.\n");
    output::putstr("Home page: http://www.greenwoodsoftware.com/less\n");
    utils::quit(QUIT_OK);
    break;
  }
}

/*
 * Handler for the -x option.
 */
void opt_x(int type, char* s)
{
  constexpr const int MSG_SIZE = 60 + (4 * TABSTOP_MAX);
  char                msg[MSG_SIZE];
  int                 i;
  parg_t              p;

  switch (type) {
  case option::INIT:
  case option::TOGGLE:
    /* Start at 1 because tabstops[0] is always zero. */
    for (i = 1; i < TABSTOP_MAX;) {
      int n = 0;
      s     = utils::skipsp(s);
      while (*s >= '0' && *s <= '9')
        n = (10 * n) + (*s++ - '0');
      if (n > tabstops[i - 1])
        tabstops[i++] = n;
      s = utils::skipsp(s);
      if (*s++ != ',')
        break;
    }
    if (i < 2)
      return;
    ntabstops  = i;
    tabdefault = tabstops[ntabstops - 1] - tabstops[ntabstops - 2];
    break;
  case option::QUERY:
    strncpy(msg, "Tab stops ", MSG_SIZE - 1);
    if (ntabstops > 2) {
      for (i = 1; i < ntabstops; i++) {
        if (i > 1)
          strncat(msg, ",", MSG_SIZE - strlen(msg) - 1);
        ignore_result(sprintf(msg + strlen(msg), "%d", tabstops[i]));
      }
      ignore_result(sprintf(msg + strlen(msg), " and then "));
    }
    ignore_result(sprintf(msg + strlen(msg), "every %d spaces",
        tabdefault));
    p.p_string = msg;
    output::error((char*)"%s", p);
    break;
  }
}

/*
 * Handler for the -" option.
 */
void opt_quote(int type, char* s)
{
  char   buf[3];
  parg_t parg;

  switch (type) {
  case option::INIT:
  case option::TOGGLE:
    if (s[0] == '\0') {
      less::Globals::openquote = less::Globals::closequote = '\0';
      break;
    }
    if (s[1] != '\0' && s[2] != '\0') {
      output::error((char*)"-\" must be followed by 1 or 2 chars", NULL_PARG);
      return;
    }
    less::Globals::openquote = s[0];
    if (s[1] == '\0')
      less::Globals::closequote = less::Globals::openquote;
    else
      less::Globals::closequote = s[1];
    break;
  case option::QUERY:
    buf[0]        = less::Globals::openquote;
    buf[1]        = less::Globals::closequote;
    buf[2]        = '\0';
    parg.p_string = buf;
    output::error((char*)"quotes %s", parg);
    break;
  }
}

/*
 * Handler for the --rscroll option.
 */
/*ARGSUSED*/
void opt_rscroll(int type, char* s)
{
  parg_t p;

  switch (type) {
  case option::INIT:
  case option::TOGGLE: {
    char* fmt;
    int   attr = AT_STANDOUT;
    charset::setfmt(s, &fmt, &attr, (char*)"*s>");
    if (strcmp(fmt, "-") == 0) {
      rscroll_char = 0;
    } else {
      rscroll_char = *fmt ? *fmt : '>';
      rscroll_attr = attr;
    }
    break;
  }
  case option::QUERY: {
    p.p_string = rscroll_char ? charset::prchar(rscroll_char) : (char*)"-";
    output::error((char*)"rscroll char is %s", p);
    break;
  }
  }
}

/*
 * "-?" means display a help message.
 * If from the command line, exit immediately.
 */
/*ARGSUSED*/
void opt_query(int type, char* s)
{
  switch (type) {
  case option::QUERY:
  case option::TOGGLE:
    output::error((char*)"Use \"h\" for help", NULL_PARG);
    break;
  case option::INIT:
    dohelp = 1;
  }
}

/*
 * Handler for the --mouse option.
 */
/*ARGSUSED*/
void opt_mousecap(int type, char* s)
{
  switch (type) {
  case option::TOGGLE:
    if (mousecap == option::OPT_OFF)
      screen::deinit_mouse();
    else
      screen::init_mouse();
    break;
  case option::INIT:
  case option::QUERY:
    break;
  }
}

/*
 * Handler for the --wheel-lines option.
 */
/*ARGSUSED*/
void opt_wheel_lines(int type, char* s)
{
  switch (type) {
  case option::INIT:
  case option::TOGGLE:
    if (wheel_lines <= 0)
      wheel_lines = ttyin::default_wheel_lines();
    break;
  case option::QUERY:
    break;
  }
}

/*
 * Get the "screen window" size.
 */
int get_swindow(void)
{
  if (swindow > 0)
    return (swindow);
  return (sc_height + swindow);
}

} // namespace optfunc