/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * High level routines dealing with getting lines of input
 * from the file being viewed.
 *
 * When we speak of "lines" here, we mean PRINTABLE lines;
 * lines processed with respect to the screen width.
 * We use the term "raw line" to refer to lines simply
 * delimited by newlines; not processed with respect to screen width.
 */

#include "input.hpp"
#include "ch.hpp"
#include "less.hpp"
#include "line.hpp"
#include "search.hpp"

extern int squeeze;
extern int chopline;
extern int hshift;
extern int quit_if_one_screen;
extern int status_col;
extern position_t start_attnpos;
extern position_t end_attnpos;
#if HILITE_SEARCH
extern int hilite_search;
extern int size_linebuf;
#endif

/*
 * Get the next line.
 * A "current" position is passed and a "new" position is returned.
 * The current position is the position of the first character of
 * a line.  The new position is the position of the first character
 * of the NEXT line.  The line obtained is the line starting at curr_pos.
 */
position_t forw_line(position_t curr_pos)
{
    position_t base_pos;
    position_t new_pos;
    int c;
    int blankline;
    int backchars;

get_forw_line:
    if (curr_pos == NULL_POSITION) {
        null_line();
        return (NULL_POSITION);
    }
#if HILITE_SEARCH
    if (hilite_search == OPT_ONPLUS || is_filtering() || status_col) {
        debug("line 62 status_col val = ", static_cast<int>(status_col));
        /*
         * If we are ignoring EOI (command F), only prepare
         * one line ahead, to avoid getting stuck waiting for
         * slow data without displaying the data we already have.
         * If we're not ignoring EOI, we *could* do the same, but
         * for efficiency we prepare several lines ahead at once.
         */
        prep_hilite(curr_pos, curr_pos + 3 * size_linebuf,
            less::Settings::ignore_eoi ? 1 : -1);
        curr_pos = next_unfiltered(curr_pos);
    }
#endif
    if (ch::ch_seek(curr_pos)) {
        null_line();
        return (NULL_POSITION);
    }

    /*
     * Step back to the beginning of the line.
     */
    base_pos = curr_pos;
    for (;;) {
        debug("forw_get - step back to beg of line - base_pos = ", base_pos);
        if (is_abort_signal(less::Settings::sigs)) {
            null_line();
            return (NULL_POSITION);
        }
        c = ch::ch_back_get();
        if (c == EOI)
            break;
        if (c == '\n') {
            (void)ch::ch_forw_get();
            break;
        }
        --base_pos;
    }

    debug("forw_get - read_forward again");
    /*
     * Read forward again to the position we should start at.
     */
    prewind();
    plinenum(base_pos);
    (void)ch::ch_seek(base_pos);
    new_pos = base_pos;
    while (new_pos < curr_pos) {
        if (is_abort_signal(less::Settings::sigs)) {
            null_line();
            return (NULL_POSITION);
        }
        c = ch::ch_forw_get();
        backchars = pappend(c, new_pos);
        new_pos++;
        if (backchars > 0) {
            pshift_all();
            new_pos -= backchars;
            while (--backchars >= 0)
                (void)ch::ch_back_get();
        }
    }
    (void)pflushmbc();
    pshift_all();

    debug("read the 1st char to display");
    /*
     * Read the first character to display.
     */
    c = ch::ch_forw_get();
    if (c == EOI) {
        null_line();
        return (NULL_POSITION);
    }
    blankline = (c == '\n' || c == '\r');

    debug("Read each character in the line and append to the line buffer");
    /*
     * Read each character in the line and append to the line buffer.
     */

    int chopped = false;
    bool endline = false;

    for (;;) {
        if (is_abort_signal(less::Settings::sigs)) {
            null_line();
            return (NULL_POSITION);
        }
        if (c == '\n' || c == EOI) {
            debug("fowr_line, got EOI or NL");
            /*
             * End of the line.
             */
            backchars = pflushmbc();
            new_pos = ch::ch_tell();
            if (backchars > 0 && !chopline && hshift == 0) {
                new_pos -= backchars + 1;
                endline = false;
            } else
                endline = true;
            break;
        }
        if (c != '\r')
            blankline = 0;

        /*
         * Append the char to the line and get the next char.
         */
        backchars = pappend(c, ch::ch_tell() - 1);
        if (backchars > 0) {
            debug("forw_line - line too long - backchars = ", backchars);
            /*
             * The char won't fit in the line; the line
             * is too long to print in the screen width.
             * End the line here.
             */
            if (chopline || hshift > 0) {
                do {
                    if (is_abort_signal(less::Settings::sigs)) {
                        null_line();
                        return (NULL_POSITION);
                    }
                    c = ch::ch_forw_get();
                } while (c != '\n' && c != EOI);
                new_pos = ch::ch_tell();
                endline = true;
                quit_if_one_screen = false;
                chopped = true;
            } else {
                new_pos = ch::ch_tell() - backchars;
                endline = false;
            }
            break;
        }
        c = ch::ch_forw_get();
    }

    pdone(endline, chopped, 1);

#if HILITE_SEARCH
    if (is_filtered(base_pos)) {
        /*
         * We don't want to display this line.
         * Get the next line.
         */
        curr_pos = new_pos;
        goto get_forw_line;
    }

    if (status_col && is_hilited(base_pos, ch::ch_tell() - 1, 1, NULL)) {
        debug("line 223 status_col val = ", static_cast<int>(status_col));
        set_status_col('*');
    }
#endif

    if (squeeze && blankline) {
        debug("squeeze and blankline");
        /*
         * This line is blank.
         * Skip down to the last contiguous blank line
         * and pretend it is the one which we are returning.
         */
        while ((c = ch::ch_forw_get()) == '\n' || c == '\r')
            if (is_abort_signal(less::Settings::sigs)) {
                null_line();
                return (NULL_POSITION);
            }
        if (c != EOI)
            (void)ch::ch_back_get();
        new_pos = ch::ch_tell();
    }

    debug("forw_line return new_pos = ", new_pos);
    return (new_pos);
}

/*
 * Get the previous line.
 * A "current" position is passed and a "new" position is returned.
 * The current position is the position of the first character of
 * a line.  The new position is the position of the first character
 * of the PREVIOUS line.  The line obtained is the one starting at new_pos.
 */
position_t back_line(position_t curr_pos)
{
    debug ("back_line called currpos", curr_pos);
    position_t new_pos, begin_new_pos, base_pos;
    int c;

    int backchars;

get_back_line:
    if (curr_pos == NULL_POSITION || curr_pos <= ch_zero) {
        null_line();
        return (NULL_POSITION);
    }
#if HILITE_SEARCH
    if (hilite_search == OPT_ONPLUS || is_filtering() || status_col) {
        debug("line 269 status_col val = ", static_cast<int>(status_col));
        prep_hilite((curr_pos < 3 * size_linebuf) ? 0 : curr_pos - 3 * size_linebuf, curr_pos, -1);
    }
#endif
    if (ch::ch_seek(curr_pos - 1)) {
        null_line();
        return (NULL_POSITION);
    }

    if (squeeze) {
        /*
         * Find out if the "current" line was blank.
         */
        (void)ch::ch_forw_get(); /* Skip the newline */
        c = ch::ch_forw_get(); /* First char of "current" line */
        (void)ch::ch_back_get(); /* Restore our position */
        (void)ch::ch_back_get();

        if (c == '\n' || c == '\r') {
            /*
             * The "current" line was blank.
             * Skip over any preceding blank lines,
             * since we skipped them in forw_line().
             */
            while ((c = ch::ch_back_get()) == '\n' || c == '\r')
                if (is_abort_signal(less::Settings::sigs)) {
                    null_line();
                    return (NULL_POSITION);
                }
            if (c == EOI) {
                null_line();
                return (NULL_POSITION);
            }
            (void)ch::ch_forw_get();
        }
    }

    /*
     * Scan backwards until we hit the beginning of the line.
     */
    for (;;) {
        if (is_abort_signal(less::Settings::sigs)) {
            null_line();
            return (NULL_POSITION);
        }
        c = ch::ch_back_get();
        if (c == '\n') {
            /*
             * This is the newline ending the previous line.
             * We have hit the beginning of the line.
             */
            base_pos = ch::ch_tell() + 1;
            break;
        }
        if (c == EOI) {
            /*
             * We have hit the beginning of the file.
             * This must be the first line in the file.
             * This must, of course, be the beginning of the line.
             */
            base_pos = ch::ch_tell();
            break;
        }
    }

    /*
     * Now scan forwards from the beginning of this line.
     * We keep discarding "printable lines" (based on screen width)
     * until we reach the curr_pos.
     *
     * {{ This algorithm is pretty inefficient if the lines
     *    are much longer than the screen width,
     *    but I don't know of any better way. }}
     */
    new_pos = base_pos;
    if (ch::ch_seek(new_pos)) {
        null_line();
        return (NULL_POSITION);
    }
    bool endline = false;
    bool chopped = false;
    prewind();
    plinenum(new_pos);
loop:
    begin_new_pos = new_pos;
    (void)ch::ch_seek(new_pos);

    do {
        c = ch::ch_forw_get();
        if (c == EOI || is_abort_signal(less::Settings::sigs)) {
            null_line();
            return (NULL_POSITION);
        }
        new_pos++;
        if (c == '\n') {
            backchars = pflushmbc();
            if (backchars > 0 && !chopline && hshift == 0) {
                backchars++;
                goto shift;
            }
            endline = true;
            break;
        }
        backchars = pappend(c, ch::ch_tell() - 1);
        if (backchars > 0) {
            /*
             * Got a full printable line, but we haven't
             * reached our curr_pos yet.  Discard the line
             * and start a new one.
             */
            if (chopline || hshift > 0) {
                endline = true;
                chopped = true;
                quit_if_one_screen = false;
                break;
            }
        shift:
            pshift_all();
            while (backchars-- > 0) {
                (void)ch::ch_back_get();
                new_pos--;
            }
            goto loop;
        }
    } while (new_pos < curr_pos);

    pdone(endline, chopped, 0);

#if HILITE_SEARCH
    if (is_filtered(base_pos)) {
        /*
         * We don't want to display this line.
         * Get the previous line.
         */
        curr_pos = begin_new_pos;
        goto get_back_line;
    }

    if (status_col && curr_pos > 0 && is_hilited(base_pos, curr_pos - 1, 1, NULL)) {
        debug("line 427 status_col val = ", static_cast<int>(status_col));
        set_status_col('*');
    }
#endif

    return (begin_new_pos);
}

/*
 * Set attnpos.
 */
void set_attnpos(position_t pos)
{
    int c;

    if (pos != NULL_POSITION) {
        if (ch::ch_seek(pos))
            return;
        for (;;) {
            c = ch::ch_forw_get();
            if (c == EOI)
                break;
            if (c == '\n' || c == '\r') {
                (void)ch::ch_back_get();
                break;
            }
            pos++;
        }
        end_attnpos = pos;
        for (;;) {
            c = ch::ch_back_get();
            if (c == EOI || c == '\n' || c == '\r')
                break;
            pos--;
        }
    }
    start_attnpos = pos;
}
