/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Routines which jump to a new location in the file.
 */

#include "jump.hpp"
#include "ch.hpp"
#include "forwback.hpp"
#include "input.hpp"
#include "less.hpp"
#include "linenum.hpp"
#include "mark.hpp"
#include "os.hpp"
#include "output.hpp"
#include "position.hpp"
#include "screen.hpp"
#include "search.hpp"

// TODO: Move to namespaces
extern int jump_sline;
extern int squished;
extern int sc_width, sc_height;
extern int show_attn;
extern int top_scroll;

namespace jump {
/*
 * Jump to the end of the file.
 */

void jump_forw(void)
{
  position_t pos;
  position_t end_pos;

  if (ch::end_seek()) {
    output::error((char*)"Cannot seek to end of file", NULL_PARG);
    return;
  }
  /*
   * Note; lastmark will be called later by jump_loc, but it fails
   * because the position table has been cleared by position::pos_clear below.
   * So call it here before calling position::pos_clear.
   */
  lastmark();
  /*
   * Position the last line in the file at the last screen line.
   * Go back one line from the end of the file
   * to get to the beginning of the last line.
   */
  position::pos_clear();
  end_pos = ch::tell();
  pos     = input::back_line(end_pos);
  if (pos == NULL_POSITION)
    jump_loc(ch_zero, sc_height - 1);
  else {
    jump_loc(pos, sc_height - 1);
    if (position::position(sc_height - 1) != end_pos)
      repaint();
  }
}

/*
 * Jump to the last buffered line in the file.
 */

void jump_forw_buffered(void)
{
  position_t end;

  if (ch::end_buffer_seek()) {
    output::error((char*)"Cannot seek to end of buffers", NULL_PARG);
    return;
  }
  end = ch::tell();
  if (end != NULL_POSITION && end > 0)
    jump_line_loc(end - 1, sc_height - 1);
}

/*
 * Jump to line n in the file.
 */

void jump_back(linenum_t linenum)
{
  position_t pos;
  parg_t     parg;

  /*
   * Find the position of the specified line.
   * If we can seek there, just jump to it.
   * If we can't seek, but we're trying to go to line number 1,
   * use beg_seek() to get as close as we can.
   */
  pos = find_pos(linenum);
  if (pos != NULL_POSITION && ch::seek(pos) == 0) {
    if (show_attn)
      input::set_attnpos(pos);
    jump_loc(pos, jump_sline);
  } else if (linenum <= 1 && ch::beg_seek() == 0) {
    jump_loc(ch::tell(), jump_sline);
    output::error((char*)"Cannot seek to beginning of file", NULL_PARG);
  } else {
    parg.p_linenum = linenum;
    output::error((char*)"Cannot seek to line number %n", parg);
  }
}

/*
 * Repaint the screen.
 */

void repaint(void)
{
  struct scrpos scrpos;
  /*
   * Start at the line currently at the top of the screen
   * and redisplay the screen.
   */
  position::get_scrpos(&scrpos, TOP);
  int it = static_cast<int>(scrpos.pos);
  debug::debug("position : ", it);
  position::pos_clear();
  if (scrpos.pos == NULL_POSITION)
    /* Screen hasn't been drawn yet. */
    jump_loc(ch_zero, 1);
  else
    jump_loc(scrpos.pos, scrpos.ln);
}

/*
 * Jump to a specified os::percentage into the file.
 */

void jump_percent(int percent, long fraction)
{
  position_t pos, len;

  /*
   * Determine the position in the file
   * (the specified os::percentage of the file's length).
   */
  len = ch::length();

  if (len == NULL_POSITION) {
    output::ierror((char*)"Determining length of file", NULL_PARG);
    ch::end_seek();
  }

  len = ch::length();

  if (len == NULL_POSITION) {
    output::error((char*)"Don't know length of file", NULL_PARG);
    return;
  }

  pos = os::percent_pos(len, percent, fraction);

  if (pos >= len)
    pos = len - 1;

  jump_line_loc(pos, jump_sline);
}

/*
 * Jump to a specified position in the file.
 * Like jump_loc, but the position need not be
 * the first character in a line.
 */

void jump_line_loc(position_t pos, int sline)
{
  int c;

  if (ch::seek(pos) == 0) {
    /*
     * Back up to the beginning of the line.
     */
    while ((c = ch::back_get()) != '\n' && c != EOI)
      ;
    if (c == '\n')
      (void)ch::forw_get();
    pos = ch::tell();
  }
  if (show_attn)
    input::set_attnpos(pos);
  jump_loc(pos, sline);
}

/*
 * Jump to a specified position in the file.
 * The position must be the first character in a line.
 * Place the target line on a specified line on the screen.
 */

void jump_loc(position_t pos, int sline)
{
  int        nline;
  int        sindex;
  position_t tpos;
  position_t bpos;

  /*
   * Normalize sline.
   */
  sindex = position::sindex_from_sline(sline);

  if ((nline = position::onscreen(pos)) >= 0) {
    /*
     * The line is currently displayed.
     * Just scroll there.
     */
    nline -= sindex;
    if (nline > 0)
      forwback::forw(nline, position::position(BOTTOM_PLUS_ONE), 1, 0, 0);
    else if (nline < 0)
      forwback::back(-nline, position::position(TOP), 1, 0);
#if HILITE_SEARCH
    if (show_attn)
      search::repaint_hilite(1);
#endif
    return;
  }

  /*
   * Line is not on screen.
   * Seek to the desired location.
   */
  if (ch::seek(pos)) {
    output::error((char*)"Cannot seek to that file position", NULL_PARG);
    return;
  }

  /*
   * See if the desired line is before or after
   * the currently displayed screen.
   */
  tpos = position::position(TOP);
  bpos = position::position(BOTTOM_PLUS_ONE);
  if (tpos == NULL_POSITION || pos >= tpos) {
    /*
     * The desired line is after the current screen.
     * Move back in the file far enough so that we can
     * call forwback::forw() and put the desired line at the
     * sline-th line on the screen.
     */
    for (nline = 0; nline < sindex; nline++) {
      if (bpos != NULL_POSITION && pos <= bpos) {
        /*
         * Surprise!  The desired line is
         * close enough to the current screen
         * that we can just scroll there after all.
         */
        forwback::forw(sc_height - sindex + nline - 1, bpos, 1, 0, 0);
#if HILITE_SEARCH
        if (show_attn)
          search::repaint_hilite(1);
#endif
        return;
      }
      pos = input::back_line(pos);
      if (pos == NULL_POSITION) {
        /*
         * Oops.  Ran into the beginning of the file.
         * Exit the loop here and rely on forwback::forw()
         * below to draw the required number of
         * blank lines at the top of the screen.
         */
        break;
      }
    }
    lastmark();
    squished       = 0;
    screen_trashed = NOT_TRASHED;
    forwback::forw(sc_height - 1, pos, 1, 0, sindex - nline);
  } else {
    /*
     * The desired line is before the current screen.
     * Move forward in the file far enough so that we
     * can call back() and put the desired line at the
     * sindex-th line on the screen.
     */
    for (nline = sindex; nline < sc_height - 1; nline++) {
      pos = input::forw_line(pos);
      if (pos == NULL_POSITION) {
        /*
         * Ran into end of file.
         * This shouldn't normally happen,
         * but may if there is some kind of read output::error.
         */
        break;
      }
#if HILITE_SEARCH
      pos = search::next_unfiltered(pos);
#endif
      if (pos >= tpos) {
        /*
         * Surprise!  The desired line is
         * close enough to the current screen
         * that we can just scroll there after all.
         */
        forwback::back(nline + 1, tpos, 1, 0);
#if HILITE_SEARCH
        if (show_attn)
          search::repaint_hilite(1);
#endif
        return;
      }
    }
    lastmark();
    if (!top_scroll)
      screen::clear();
    else
      screen::home();
    screen_trashed = NOT_TRASHED;
    position::add_back_pos(pos);
    forwback::back(sc_height - 1, pos, 1, 0);
  }
}

} // namespace jump