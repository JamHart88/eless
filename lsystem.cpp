/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Routines to execute other programs.
 * Necessarily very OS dependent.
 */

#include "lsystem.hpp"
#include "ch.hpp"
#include "decode.hpp"
#include "edit.hpp"
#include "filename.hpp"
#include "forwback.hpp"
#include "less.hpp"
#include "mark.hpp"
#include "output.hpp"
#include "position.hpp"
#include "screen.hpp"
#include "signal.hpp"
#include "utils.hpp"

#include <csignal>

namespace lsystem {

#if HAVE_SYSTEM

/*
 * Pass the specified command to a shell to be executed.
 * Like plain "system()", but handles resetting terminal modes, etc.
 */

void lsystem(char* cmd, char* donemsg)
{
  int           inp;
  char*         shell;
  char*         p;
  ifile::Ifile* save_ifile;

  /*
   * Print the command which is to be executed,
   * unless the command starts with a "-".
   */
  if (cmd[0] == '-')
    cmd++;
  else {
    screen::clear_bot();
    output::putstr("!");
    output::putstr(cmd);
    output::putstr("\n");
  }

  /*
   * Close the current input file.
   */
  save_ifile = edit::save_curr_ifile();
  (void)edit::edit_ifile(nullptr);

  /*
   * De-initialize the terminal and take out of raw mode.
   */
  screen::deinit();
  output::flush(); /* Make sure the screen::deinit chars get out */
  screen::raw_mode(0);
  /*
   * Restore signals to their defaults.
   */
  sig::init_signals(0);

  /*
   * Force standard input to be the user's terminal
   * (the normal standard input), even if less's standard input
   * is coming from a pipe.
   */
  inp = dup(0);
  close(0);
  if (open("/dev/tty", OPEN_READ) < 0)
    ignore_result(dup(inp));

  /*
   * Pass the command to the system to be executed.
   * If we have a SHELL environment variable, use
   * <$SHELL -c "command"> instead of just <command>.
   * If the command is empty, just invoke a shell.
   */
  p = NULL;
  if ((shell = decode::lgetenv((char*)"SHELL")) != NULL && *shell != '\0') {
    if (*cmd == '\0')
      p = utils::save(shell);
    else {
      char* esccmd = filename::shell_quote(cmd);
      if (esccmd != NULL) {
        int len = (int)(strlen(shell) + strlen(esccmd) + 5);
        p       = (char*)utils::ecalloc(len, sizeof(char));
        snprintf(p, len, "%s %s %s", shell, filename::shell_coption(), esccmd);
        free(esccmd);
      }
    }
  }
  if (p == NULL) {
    if (*cmd == '\0')
      p = utils::save("sh");
    else
      p = utils::save(cmd);
  }
  ignore_result(system(p));
  free(p);

  /*
   * Restore standard input, reset signals, raw mode, etc.
   */
  close(0);
  ignore_result(dup(inp));
  close(inp);

  sig::init_signals(1);
  screen::raw_mode(1);
  if (donemsg != NULL) {
    output::putstr(donemsg);
    output::putstr("  (press RETURN)");
    output::get_return();
    output::putchr('\n');
    output::flush();
  }
  screen::init();
  screen_trashed = TRASHED;

  /*
   * Reopen the current input file.
   */
  edit::reedit_ifile(save_ifile);

#if defined(SIGWINCH) || defined(SIGWIND)
  /*
   * Since we were ignoring window change signals while we executed
   * the system command, we must assume the window changed.
   * Warning: this leaves a signal pending (in "sigs"),
   * so sig::psignals() should be called soon after lsystem().
   */
  sig::winch(0);
#endif
}

#endif

#if PIPEC

/*
 * Pipe a section of the input file into the given shell command.
 * The section to be piped is the section "between" the current
 * position and the position marked by the given letter.
 *
 * If the mark is after the current screen, the section between
 * the top line displayed and the mark is piped.
 * If the mark is before the current screen, the section between
 * the mark and the bottom line displayed is piped.
 * If the mark is on the current screen, or if the mark is ".",
 * the whole current screen is piped.
 */

int pipe_mark(int c, char* cmd)
{
  position_t mpos, tpos, bpos;

  /*
   * mpos = the marked position.
   * tpos = top of screen.
   * bpos = bottom of screen.
   */
  mpos = mark::markpos(c);
  if (mpos == NULL_POSITION)
    return (-1);
  tpos = position::position(TOP);
  if (tpos == NULL_POSITION)
    tpos = ch_zero;
  bpos = position::position(BOTTOM);

  if (c == '.')
    return (pipe_data(cmd, tpos, bpos));
  else if (mpos <= tpos)
    return (pipe_data(cmd, mpos, bpos));
  else if (bpos == NULL_POSITION)
    return (pipe_data(cmd, tpos, bpos));
  else
    return (pipe_data(cmd, tpos, mpos));
}

/*
 * Create a pipe to the given shell command.
 * Feed it the file contents between the positions spos and epos.
 */

int pipe_data(char* cmd, position_t spos, position_t epos)
{
  FILE* f;
  int   c;

  /*
   * This is structured much like lsystem().
   * Since we're running a shell program, we must be careful
   * to perform the necessary deinitialization before running
   * the command, and reinitialization after it.
   */
  if (ch::seek(spos) != 0) {
    output::error((char*)"Cannot seek to start position", NULL_PARG);
    return (-1);
  }

  if ((f = popen(cmd, "w")) == NULL) {
    output::error((char*)"Cannot create pipe", NULL_PARG);
    return (-1);
  }
  screen::clear_bot();
  output::putstr("!");
  output::putstr(cmd);
  output::putstr("\n");

  screen::deinit();
  output::flush();
  screen::raw_mode(0);
  sig::init_signals(0);
#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif

  c = EOI;
  while (epos == NULL_POSITION || spos++ <= epos) {
    /*
     * Read a character from the file and give it to the pipe.
     */
    c = ch::forw_get();
    if (c == EOI)
      break;
    if (putc(c, f) == EOF)
      break;
  }

  /*
   * Finish up the last line.
   */
  while (c != '\n' && c != EOI) {
    c = ch::forw_get();
    if (c == EOI)
      break;
    if (putc(c, f) == EOF)
      break;
  }

  pclose(f);

#ifdef SIGPIPE
  signal(SIGPIPE, SIG_DFL);
#endif
  sig::init_signals(1);
  screen::raw_mode(1);
  screen::init();
  screen_trashed = TRASHED;
#if defined(SIGWINCH) || defined(SIGWIND)
  /* {{ Probably don't need this here. }} */
  sig::winch(0);
#endif
  return (0);
}

#endif

} // namespace lsystem