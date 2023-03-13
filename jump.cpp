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

extern int jump_sline;
extern int squished;
extern int sc_width, sc_height;
extern int show_attn;
extern int top_scroll;

/*
 * Jump to the end of the file.
 */

void jump_forw(void)
{
    position_t pos;
    position_t end_pos;

    if (ch::ch_end_seek()) {
        error((char*)"Cannot seek to end of file", NULL_PARG);
        return;
    }
    /*
     * Note; lastmark will be called later by jump_loc, but it fails
     * because the position table has been cleared by pos_clear below.
     * So call it here before calling pos_clear.
     */
    lastmark();
    /*
     * Position the last line in the file at the last screen line.
     * Go back one line from the end of the file
     * to get to the beginning of the last line.
     */
    pos_clear();
    end_pos = ch::ch_tell();
    pos = back_line(end_pos);
    if (pos == NULL_POSITION)
        jump_loc(ch_zero, sc_height - 1);
    else {
        jump_loc(pos, sc_height - 1);
        if (position(sc_height - 1) != end_pos)
            repaint();
    }
}

/*
 * Jump to the last buffered line in the file.
 */

void jump_forw_buffered(void)
{
    position_t end;

    if (ch::ch_end_buffer_seek()) {
        error((char*)"Cannot seek to end of buffers", NULL_PARG);
        return;
    }
    end = ch::ch_tell();
    if (end != NULL_POSITION && end > 0)
        jump_line_loc(end - 1, sc_height - 1);
}

/*
 * Jump to line n in the file.
 */

void jump_back(linenum_t linenum)
{
    position_t pos;
    parg_t parg;

    /*
     * Find the position of the specified line.
     * If we can seek there, just jump to it.
     * If we can't seek, but we're trying to go to line number 1,
     * use ch_beg_seek() to get as close as we can.
     */
    pos = find_pos(linenum);
    if (pos != NULL_POSITION && ch::ch_seek(pos) == 0) {
        if (show_attn)
            set_attnpos(pos);
        jump_loc(pos, jump_sline);
    } else if (linenum <= 1 && ch::ch_beg_seek() == 0) {
        jump_loc(ch::ch_tell(), jump_sline);
        error((char*)"Cannot seek to beginning of file", NULL_PARG);
    } else {
        parg.p_linenum = linenum;
        error((char*)"Cannot seek to line number %n", parg);
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
    get_scrpos(&scrpos, TOP);
    int it = static_cast<int>(scrpos.pos);
    debug("position : ", it);
    pos_clear();
    if (scrpos.pos == NULL_POSITION)
        /* Screen hasn't been drawn yet. */
        jump_loc(ch_zero, 1);
    else
        jump_loc(scrpos.pos, scrpos.ln);
}

/*
 * Jump to a specified percentage into the file.
 */

void jump_percent(int percent, long fraction)
{
    position_t pos, len;

    /*
     * Determine the position in the file
     * (the specified percentage of the file's length).
     */
    len = ch::ch_length();

    if (len == NULL_POSITION) {
        ierror((char*)"Determining length of file", NULL_PARG);
        ch::ch_end_seek();
    }

    len = ch::ch_length();

    if (len == NULL_POSITION) {
        error((char*)"Don't know length of file", NULL_PARG);
        return;
    }

    pos = percent_pos(len, percent, fraction);

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

    if (ch::ch_seek(pos) == 0) {
        /*
         * Back up to the beginning of the line.
         */
        while ((c = ch::ch_back_get()) != '\n' && c != EOI)
            ;
        if (c == '\n')
            (void)ch::ch_forw_get();
        pos = ch::ch_tell();
    }
    if (show_attn)
        set_attnpos(pos);
    jump_loc(pos, sline);
}

/*
 * Jump to a specified position in the file.
 * The position must be the first character in a line.
 * Place the target line on a specified line on the screen.
 */

void jump_loc(position_t pos, int sline)
{
    int nline;
    int sindex;
    position_t tpos;
    position_t bpos;

    /*
     * Normalize sline.
     */
    sindex = sindex_from_sline(sline);

    if ((nline = onscreen(pos)) >= 0) {
        /*
         * The line is currently displayed.
         * Just scroll there.
         */
        nline -= sindex;
        if (nline > 0)
            forw(nline, position(BOTTOM_PLUS_ONE), 1, 0, 0);
        else if (nline < 0)
            back(-nline, position(TOP), 1, 0);
#if HILITE_SEARCH
        if (show_attn)
            repaint_hilite(1);
#endif
        return;
    }

    /*
     * Line is not on screen.
     * Seek to the desired location.
     */
    if (ch::ch_seek(pos)) {
        error((char*)"Cannot seek to that file position", NULL_PARG);
        return;
    }

    /*
     * See if the desired line is before or after
     * the currently displayed screen.
     */
    tpos = position(TOP);
    bpos = position(BOTTOM_PLUS_ONE);
    if (tpos == NULL_POSITION || pos >= tpos) {
        /*
         * The desired line is after the current screen.
         * Move back in the file far enough so that we can
         * call forw() and put the desired line at the
         * sline-th line on the screen.
         */
        for (nline = 0; nline < sindex; nline++) {
            if (bpos != NULL_POSITION && pos <= bpos) {
                /*
                 * Surprise!  The desired line is
                 * close enough to the current screen
                 * that we can just scroll there after all.
                 */
                forw(sc_height - sindex + nline - 1, bpos, 1, 0, 0);
#if HILITE_SEARCH
                if (show_attn)
                    repaint_hilite(1);
#endif
                return;
            }
            pos = back_line(pos);
            if (pos == NULL_POSITION) {
                /*
                 * Oops.  Ran into the beginning of the file.
                 * Exit the loop here and rely on forw()
                 * below to draw the required number of
                 * blank lines at the top of the screen.
                 */
                break;
            }
        }
        lastmark();
        squished = 0;
        screen_trashed = NOT_TRASHED;
        forw(sc_height - 1, pos, 1, 0, sindex - nline);
    } else {
        /*
         * The desired line is before the current screen.
         * Move forward in the file far enough so that we
         * can call back() and put the desired line at the
         * sindex-th line on the screen.
         */
        for (nline = sindex; nline < sc_height - 1; nline++) {
            pos = forw_line(pos);
            if (pos == NULL_POSITION) {
                /*
                 * Ran into end of file.
                 * This shouldn't normally happen,
                 * but may if there is some kind of read error.
                 */
                break;
            }
#if HILITE_SEARCH
            pos = next_unfiltered(pos);
#endif
            if (pos >= tpos) {
                /*
                 * Surprise!  The desired line is
                 * close enough to the current screen
                 * that we can just scroll there after all.
                 */
                back(nline + 1, tpos, 1, 0);
#if HILITE_SEARCH
                if (show_attn)
                    repaint_hilite(1);
#endif
                return;
            }
        }
        lastmark();
        if (!top_scroll)
            clear();
        else
            home();
        screen_trashed = NOT_TRASHED;
        add_back_pos(pos);
        back(sc_height - 1, pos, 1, 0);
    }
}
