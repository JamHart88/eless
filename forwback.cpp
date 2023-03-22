/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Primitives for displaying the file on the screen,
 * scrolling either forward or backward.
 */

#include "forwback.hpp"
#include "ch.hpp"
#include "edit.hpp"
#include "input.hpp"
#include "jump.hpp"
#include "less.hpp"
#include "linenum.hpp"
#include "option.hpp"
#include "output.hpp"
#include "position.hpp"
#include "screen.hpp"
#include "search.hpp"
#include "utils.hpp"

int squished;

int no_back_scroll = 0;

int forw_prompt;

int same_pos_bell = 1;

extern int top_scroll;
extern int sc_width, sc_height;
extern int forw_scroll;
extern int back_scroll;
extern int clear_bg;
extern int final_attr;
extern int oldbot;
#if HILITE_SEARCH
extern int size_linebuf;
extern int hilite_search;
extern int status_col;
#endif
#if TAGS
extern char* tagoption;
#endif

screen_trashed_t screen_trashed = TRASHED;

namespace forwback {

/*
 * Sound the screen::bell to indicate user is trying to move past end of file.
 */
static void eof_bell(void)
{
  if (option::Option::quiet == option::NOT_QUIET)
    screen::bell();
  else
    screen::vbell();
}

/*
 * Check to see if the end of file is currently displayed.
 */

int eof_displayed(void)
{
  position_t pos;

  if (less::Globals::ignore_eoi)
    return (0);

  if (ch::length() == NULL_POSITION)
    /*
     * If the file length is not known,
     * we can't possibly be displaying EOF.
     */
    return (0);

  /*
   * If the bottom line is empty, we are at EOF.
   * If the bottom line ends at the file length,
   * we must be just at EOF.
   */
  pos = position::position(BOTTOM_PLUS_ONE);
  return (pos == NULL_POSITION || pos == ch::length());
}

/*
 * Check to see if the entire file is currently displayed.
 */

int entire_file_displayed(void)
{
  position_t pos;

  /* Make sure last line of file is displayed. */
  if (!eof_displayed())
    return (0);

  /* Make sure first line of file is displayed. */
  pos = position::position(0);
  return (pos == NULL_POSITION || pos == 0);
}

/*
 * If the screen is "squished", jump::repaint it.
 * "Squished" means the first displayed line is not at the top
 * of the screen; this can happen when we display a short file
 * for the first time.
 */

void squish_check(void)
{
  if (!squished)
    return;
  squished = 0;
  jump::repaint();
}

/*
 * Display n lines, scrolling forward,
 * starting at position pos in the input file.
 * "force" means display the n lines even if we hit end of file.
 * "only_last" means display only the last screenful if n > screen size.
 * "nblank" is the number of blank lines to draw before the first
 *   real line.  If nblank > 0, the pos must be NULL_POSITION.
 *   The first real line after the blanks will start at ch_zero.
 */

void forw(int n, position_t pos,
    int force,
    int only_last,
    int nblank)
{
  int        nlines = 0;
  int        do_repaint;
  static int first_time = 1;

  squish_check();

  /*
   * do_repaint tells us not to display anything till the end,
   * then just jump::repaint the entire screen.
   * We jump::repaint if we are supposed to display only the last
   * screenful and the request is for more than a screenful.
   * Also if the request exceeds the forward scroll limit
   * (but not if the request is for exactly a screenful, since
   * repainting itself involves scrolling forward a screenful).
   */
  do_repaint = (only_last && n > sc_height - 1) || (forw_scroll >= 0 && n > forw_scroll && n != sc_height - 1);

#if HILITE_SEARCH
  if (hilite_search == option::OPT_ONPLUS || search::is_filtering() || status_col) {
    search::prep_hilite(pos, pos + 4 * size_linebuf, less::Globals::ignore_eoi ? 1 : -1);
    pos = search::next_unfiltered(pos);
  }
#endif

  if (!do_repaint) {
    debug::debug("not do_repaint");
    if (top_scroll && n >= sc_height - 1 && pos != ch::length()) {
      debug::debug("start new screen");
      /*
       * Start a new screen.
       * {{ This is not really desirable if we happen
       *    to hit eof in the middle of this screen,
       *    but we don't yet know if that will happen. }}
       */
      position::pos_clear();
      position::add_forw_pos(pos);
      force = 1;
      screen::clear();
      screen::home();
    }

    if (pos != position::position(BOTTOM_PLUS_ONE) || position::empty_screen()) {
      debug::debug("screen::clear screen and start a new one");
      /*
       * This is not contiguous with what is
       * currently displayed.  Clear the screen image
       * (position table) and start a new screen.
       */
      position::pos_clear();
      position::add_forw_pos(pos);
      force = 1;
      if (top_scroll) {
        screen::clear();
        screen::home();
      } else if (!first_time) {
        output::putstr("...skipping...\n");
      }
    }
  }

  while (--n >= 0) {
    debug::debug("read next line of input");
    /*
     * Read the next line of input.
     */
    if (nblank > 0) {
      /*
       * Still drawing blanks; don't get a line
       * from the file yet.
       * If this is the last blank line, get ready to
       * read a line starting at ch_zero next time.
       */
      if (--nblank == 0)
        pos = ch_zero;
    } else {
      debug::debug("Get next line from the file");
      /*
       * Get the next line from the file.
       */
      pos = input::forw_line(pos);
#if HILITE_SEARCH
      pos = search::next_unfiltered(pos);
#endif
      if (pos == NULL_POSITION) {
        debug::debug("end of the file - stop");
        /*
         * End of file: stop here unless the top line
         * is still empty, or "force" is true.
         * Even if force is true, stop when the last
         * line in the file reaches the top of screen.
         */
        if (!force && position::position(TOP) != NULL_POSITION)
          break;
        if (!position::empty_lines(0, 0) && !position::empty_lines(1, 1) && position::empty_lines(2, sc_height - 1))
          break;
      }
    }
    /*
     * Add the position of the next line to the position table.
     * Display the current line on the screen.
     */
    debug::debug("add the position of the next line to pos table");
    position::add_forw_pos(pos);
    nlines++;
    if (do_repaint) {
      debug::debug("do-jump::repaint - continue");
      continue;
    }
    /*
     * If this is the first screen displayed and
     * we hit an early EOF (i.e. before the requested
     * number of lines), we "squish" the display down
     * at the bottom of the screen.
     * But don't do this if a + option or a -t option
     * was given.  These options can cause us to
     * start the display after the beginning of the file,
     * and it is not appropriate to squish in that case.
     */
    if (first_time && pos == NULL_POSITION && !top_scroll &&
#if TAGS
        tagoption == NULL &&
#endif
        !option::Option::plusoption) {
      squished = 1;
      continue;
    }
    output::put_line();

    forw_prompt = 1;
  }

  if (nlines == 0 && !less::Globals::ignore_eoi && same_pos_bell)
    eof_bell();
  else if (do_repaint) {
    debug::debug("jump::repaint from forw");
    jump::repaint();
  }
  first_time = 0;
  (void)currline(BOTTOM);
}

/*
 * Display n lines, scrolling backward.
 */

void back(int n, position_t pos, int force, int only_last)
{
  int nlines = 0;
  int do_repaint;

  squish_check();
  do_repaint = (n > get_back_scroll() || (only_last && n > sc_height - 1));
#if HILITE_SEARCH
  if (hilite_search == option::OPT_ONPLUS || search::is_filtering() || status_col) {
    search::prep_hilite((pos < 3 * size_linebuf) ? 0 : pos - 3 * size_linebuf, pos, -1);
  }
#endif
  while (--n >= 0) {
    /*
     * Get the previous line of input.
     */
#if HILITE_SEARCH
    pos = search::prev_unfiltered(pos);
#endif

    pos = input::back_line(pos);
    if (pos == NULL_POSITION) {
      /*
       * Beginning of file: stop here unless "force" is true.
       */
      if (!force)
        break;
    }
    /*
     * Add the position of the previous line to the position table.
     * Display the line on the screen.
     */
    position::add_back_pos(pos);
    nlines++;
    if (!do_repaint) {
      screen::home();
      screen::add_line();
      output::put_line();
    }
  }

  if (nlines == 0 && same_pos_bell)
    eof_bell();
  else if (do_repaint)
    jump::repaint();
  else if (!oldbot)
    screen::lower_left();
  (void)currline(BOTTOM);
}

/*
 * Display n more lines, forward.
 * Start just after the line currently displayed at the bottom of the screen.
 */

void forward(int n, int force, int only_last)
{
  position_t pos;

  if (option::get_quit_at_eof() && eof_displayed() && !(ch::getflags() & CH_HELPFILE)) {
    /*
     * If the -e flag is set and we're trying to go
     * forward from end-of-file, go on to the next file.
     */
    if (edit::edit_next(1))
      utils::quit(QUIT_OK);
    return;
  }

  pos = position::position(BOTTOM_PLUS_ONE);
  if (pos == NULL_POSITION && (!force || position::empty_lines(2, sc_height - 1))) {
    if (less::Globals::ignore_eoi) {
      /*
       * less::Globals::ignore_eoi is to support A_F_FOREVER.
       * Back up until there is a line at the bottom
       * of the screen.
       */
      if (position::empty_screen())
        pos = ch_zero;
      else {
        do {
          back(1, position::position(TOP), 1, 0);
          pos = position::position(BOTTOM_PLUS_ONE);
        } while (pos == NULL_POSITION);
      }
    } else {
      eof_bell();
      return;
    }
  }
  forw(n, pos, force, only_last, 0);
}

/*
 * Display n more lines, backward.
 * Start just before the line currently displayed at the top of the screen.
 */

void backward(int n, int force, int only_last)
{
  position_t pos;

  pos = position::position(TOP);
  if (pos == NULL_POSITION && (!force || position::position(BOTTOM) == 0)) {
    eof_bell();
    return;
  }
  back(n, pos, force, only_last);
}

/*
 * Get the backwards scroll limit.
 * Must call this function instead of just using the value of
 * back_scroll, because the default case depends on sc_height and
 * top_scroll, as well as back_scroll.
 */

int get_back_scroll(void)
{
  if (no_back_scroll)
    return (0);
  if (back_scroll >= 0)
    return (back_scroll);
  if (top_scroll)
    return (sc_height - 2);
  return (10000); /* infinity */
}

/*
 * Will the entire file fit on one screen?
 */

int get_one_screen(void)
{
  int        nlines;
  position_t pos = ch_zero;

  for (nlines = 0; nlines < sc_height; nlines++) {
    pos = input::forw_line(pos);
    if (pos == NULL_POSITION)
      break;
  }
  return (nlines < sc_height);
}

} // namespace forwback