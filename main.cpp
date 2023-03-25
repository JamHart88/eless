/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Entry point, initialization, miscellaneous routines.
 */

#include "charset.hpp"
#include "cmdbuf.hpp"
#include "command.hpp"
#include "decode.hpp"
#include "edit.hpp"
#include "filename.hpp"
#include "forwback.hpp"
#include "ifile.hpp"
#include "less.hpp"
#include "line.hpp"
#include "mark.hpp"
#include "option.hpp"
#include "opttbl.hpp"
#include "output.hpp"
#include "prompt.hpp"
#include "screen.hpp"
#include "search.hpp"
#include "signal.hpp"
#include "tags.hpp"
#include "ttyin.hpp"
#include "utils.hpp"

char* every_first_cmd = NULL;

bool new_file;

int is_tty;

struct scrpos initial_scrpos;

position_t start_attnpos = NULL_POSITION;

position_t end_attnpos = NULL_POSITION;

int wscroll;

char* progname;

int dohelp;

#if EDITOR
char* editor;
char* editproto;
#endif

#if TAGS
extern char* tags_ptr;
extern char* tagoption;
extern int   jump_sline;
#endif

int        one_screen;
extern int missing_cap;
extern int know_dumb;
extern int pr_type;
extern int quit_if_one_screen;
extern int no_init;

/*
 * Entry point.
 */
int main(int argc, char* argv[])
{
  // int argc;
  // char *argv[];

  char* s;

  progname = *argv++;
  argc--;

  /*
   * Process command line arguments and LESS environment arguments.
   * Command line arguments override environment arguments.
   */
  is_tty = isatty(1);
  mark::init_mark();
  decode::init_cmds();
  screen::get_term();
  charset::init_charset();
  line::init_line();
  cmdbuf::init_cmdhist();
  opttbl::init_option();
  search::init_search();

  /*
   * If the name of the executable program is "more",
   * act like LESS_IS_MORE is set.
   */
  s = filename::last_component(progname);
  if (strcmp(s, "more") == 0)
    option::Option::less_is_more = 1;

  prompt::init_prompt();

  s = decode::lgetenv(option::Option::less_is_more ? (char*)"MORE" : (char*)"LESS");
  if (s != NULL)
    option::scan_option(utils::save(s));

    // TODO: convert to template
#define isoptstring(s) (((s)[0] == '-' || (s)[0] == '+') && (s)[1] != '\0')
  while (argc > 0 && (isoptstring(*argv) || option::isoptpending())) {
    s = *argv++;
    argc--;
    if (strcmp(s, "--") == 0)
      break;
    option::scan_option(s);
  }
#undef isoptstring

  if (option::isoptpending()) {
    /*
     * Last command line option was a flag requiring a
     * following string, but there was no following string.
     */
    option::nopendopt();
    utils::quit(QUIT_OK);
  }

  decode::expand_cmd_tables();

#if EDITOR
  editor = decode::lgetenv((char*)"VISUAL");
  if (editor == NULL || *editor == '\0') {
    editor = decode::lgetenv((char*)"EDITOR");
    if (decode::isnullenv(editor))
      editor = (char*)EDIT_PGM;
  }
  editproto = decode::lgetenv((char*)"LESSEDIT");
  if (decode::isnullenv(editproto))
    editproto = (char*)"%E ?lm+%lm. %g";
#endif

  /*
   * Call get_ifile with all the command line filenames
   * to "register" them with the ifile system.
   */
  if (dohelp)
    ifile::createIfile(FAKE_HELPFILE);
  while (argc-- > 0) {
    ifile::createIfile(*argv++);
  }
  /*
   * Set up terminal, etc.
   */
  if (!is_tty) {
    /*
     * Output is not a tty.
     * Just copy the input file(s) to output.
     */
    // SET_BINARY(1);// no longer needed?
    if (edit::edit_first() == 0) {
      do {
        edit::cat_file();
      } while (edit::edit_next(1) == 0);
    }
    utils::quit(QUIT_OK);
  }

  if (missing_cap && !know_dumb)
    output::error((char*)"WARNING: terminal is not fully functional", NULL_PARG);
  ttyin::open_getchr();
  screen::raw_mode(1);
  sig::init_signals(1);

  /*
   * Select the first file to examine.
   */
#if TAGS
  if (tagoption != NULL || strcmp(tags_ptr, "-") == 0) {
    /*
     * A -t option was given.
     * Verify that no filenames were also given.
     * Edit the file selected by the "tags_ptr" search,
     * and search for the proper line in the file.
     */
    if (ifile::numIfiles() > 0) {
      output::error((char*)"No filenames allowed with -t option",
          NULL_PARG);
      utils::quit(QUIT_ERROR);
    }
    tags::findtag(tagoption);
    if (tags::edit_tagfile()) /* Edit file which contains the tag */
      utils::quit(QUIT_ERROR);
    /*
     * Search for the line which contains the tag.
     * Set up initial_scrpos so we display that line.
     */
    initial_scrpos.pos = tags::tagsearch();
    if (initial_scrpos.pos == NULL_POSITION)
      utils::quit(QUIT_ERROR);
    initial_scrpos.ln = jump_sline;
  } else
#endif
  {
    if (edit::edit_first())
      utils::quit(QUIT_ERROR);
    /*
     * See if file fits on one screen to decide whether
     * to send terminal init. But don't need this
     * if -X (no_init) overrides this (see init()).
     */
    if (quit_if_one_screen) {
      if (ifile::numIfiles()
          > 1) /* If more than one file, -F cannot be used */
        quit_if_one_screen = false;
      else if (!no_init)
        one_screen = forwback::get_one_screen();
    }
  }

  screen::init();
  command::commands();
  utils::quit(QUIT_OK);
  /*NOTREACHED*/
  return (0);
}
