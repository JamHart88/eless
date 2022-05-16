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

int new_file;

int is_tty;

IFILE curr_ifile = NULL_IFILE;

IFILE old_ifile = NULL_IFILE;

struct scrpos initial_scrpos;


position_t start_attnpos = NULL_POSITION;

position_t end_attnpos = NULL_POSITION;

int wscroll;

char* progname;


int dohelp;

#if LOGFILE

int logfile = -1;

int force_logfile = FALSE;

char* namelogfile = NULL;
#endif

#if EDITOR

char* editor;

char* editproto;
#endif

#if TAGS
extern char* tags;
extern char* tagoption;
extern int jump_sline;
#endif


int one_screen;
extern int less_is_more;
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
    IFILE ifile;
    char* s;

    progname = *argv++;
    argc--;

    /*
     * Process command line arguments and LESS environment arguments.
     * Command line arguments override environment arguments.
     */
    is_tty = isatty(1);
    init_mark();
    init_cmds();
    get_term();
    init_charset();
    init_line();
    init_cmdhist();
    init_option();
    init_search();

    /*
     * If the name of the executable program is "more",
     * act like LESS_IS_MORE is set.
     */
    s = last_component(progname);
    if (strcmp(s, "more") == 0)
        less_is_more = 1;

    init_prompt();

    s = lgetenv(less_is_more ? (char*)"MORE" : (char*)"LESS");
    if (s != NULL)
        scan_option(utils::save(s));

#define isoptstring(s) (((s)[0] == '-' || (s)[0] == '+') && (s)[1] != '\0')
    while (argc > 0 && (isoptstring(*argv) || isoptpending())) {
        s = *argv++;
        argc--;
        if (strcmp(s, "--") == 0)
            break;
        scan_option(s);
    }
#undef isoptstring

    if (isoptpending()) {
        /*
         * Last command line option was a flag requiring a
         * following string, but there was no following string.
         */
        nopendopt();
        utils::quit(QUIT_OK);
    }

    expand_cmd_tables();

#if EDITOR
    editor = lgetenv((char*)"VISUAL");
    if (editor == NULL || *editor == '\0') {
        editor = lgetenv((char*)"EDITOR");
        if (isnullenv(editor))
            editor = (char*)EDIT_PGM;
    }
    editproto = lgetenv((char*)"LESSEDIT");
    if (isnullenv(editproto))
        editproto = (char*)"%E ?lm+%lm. %g";
#endif

    /*
     * Call get_ifile with all the command line filenames
     * to "register" them with the ifile system.
     */
    ifile = NULL_IFILE;
    if (dohelp)
        ifile = get_ifile((char*)FAKE_HELPFILE, ifile);
    while (argc-- > 0) {
        (void)get_ifile(*argv++, ifile);
        ifile = prev_ifile(NULL_IFILE);
    }
    /*
     * Set up terminal, etc.
     */
    if (!is_tty) {
        /*
         * Output is not a tty.
         * Just copy the input file(s) to output.
         */
        SET_BINARY(1);
        if (edit_first() == 0) {
            do {
                cat_file();
            } while (edit_next(1) == 0);
        }
        utils::quit(QUIT_OK);
    }

    if (missing_cap && !know_dumb)
        error((char*)"WARNING: terminal is not fully functional", NULL_PARG);
    open_getchr();
    raw_mode(1);
    init_signals(1);

    /*
     * Select the first file to examine.
     */
#if TAGS
    if (tagoption != NULL || strcmp(tags, "-") == 0) {
        /*
         * A -t option was given.
         * Verify that no filenames were also given.
         * Edit the file selected by the "tags" search,
         * and search for the proper line in the file.
         */
        if (nifile() > 0) {
            error((char*)"No filenames allowed with -t option",
                NULL_PARG);
            utils::quit(QUIT_ERROR);
        }
        findtag(tagoption);
        if (edit_tagfile()) /* Edit file which contains the tag */
            utils::quit(QUIT_ERROR);
        /*
         * Search for the line which contains the tag.
         * Set up initial_scrpos so we display that line.
         */
        initial_scrpos.pos = tagsearch();
        if (initial_scrpos.pos == NULL_POSITION)
            utils::quit(QUIT_ERROR);
        initial_scrpos.ln = jump_sline;
    } else
#endif
    {
        if (edit_first())
            utils::quit(QUIT_ERROR);
        /*
         * See if file fits on one screen to decide whether
         * to send terminal init. But don't need this
         * if -X (no_init) overrides this (see init()).
         */
        if (quit_if_one_screen) {
            if (nifile()
                > 1) /* If more than one file, -F cannot be used */
                quit_if_one_screen = FALSE;
            else if (!no_init)
                one_screen = get_one_screen();
        }
    }

    init();
    commands();
    utils::quit(QUIT_OK);
    /*NOTREACHED*/
    return (0);
}
