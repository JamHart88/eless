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
 *    INIT      The option is being initialized from the command line.
 *    TOGGLE    The option is being changed from within the program.
 *    QUERY     The setting of the option is merely being queried.
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

extern int nbufs;
extern int bufspace;
extern int pr_type;
extern bool plusoption;
extern int swindow;
extern int sc_width;
extern int sc_height;
extern int dohelp;
extern bool any_display;
extern char openquote;
extern char closequote;
extern char* prproto[];
extern char* eqproto;
extern char* hproto;
extern char* wproto;
extern char* every_first_cmd;
extern char version[];
extern int jump_sline;
extern long jump_sline_fraction;
extern int shift_count;
extern long shift_count_fraction;
extern char rscroll_char;
extern int rscroll_attr;
extern int mousecap;
extern int wheel_lines;
extern int less_is_more;

#if TAGS
char* tagoption = NULL;
extern char* tags;
extern char ztags[];
#endif


/*
 * Handler for -o option.
 */
void opt_o(int type, char* s)
{
    parg_t parg;
    char* filename;

    switch (type) {
    case INIT:
        less::Settings::namelogfile = utils::save(s);
        break;
    case TOGGLE:
        if (ch::ch_getflags() & CH_CANSEEK) {
            error((char*)"Input is not a pipe", NULL_PARG);
            return;
        }
        if (less::Settings::logfile >= 0) {
            error((char*)"Log file is already in use", NULL_PARG);
            return;
        }
        s = utils::skipsp(s);
        if (less::Settings::namelogfile != NULL)
            free(less::Settings::namelogfile);
        filename = lglob(s);
        less::Settings::namelogfile = shell_unquote(filename);
        free(filename);
        use_logfile(less::Settings::namelogfile);
        ch::sync_logfile();
        break;
    case QUERY:
        if (less::Settings::logfile < 0)
            error((char*)"No log file", NULL_PARG);
        else {
            parg.p_string = less::Settings::namelogfile;
            error((char*)"Log file \"%s\"", parg);
        }
        break;
    }
}

/*
 * Handler for -O option.
 */
void opt__O(int type, char* s)
{
    less::Settings::force_logfile = true;
    opt_o(type, s);
}

/*
 * Handlers for -j option.
 */
void opt_j(int type, char* s)
{
    parg_t parg;
    char buf[30]; // Make bigger than Long int in chars
    int len;
    int err;

    switch (type) {
    case INIT:
    case TOGGLE:
        if (*s == '.') {
            s++;
            jump_sline_fraction = getfraction(&s, (char*)"j", &err);
            if (err)
                error((char*)"Invalid line fraction", NULL_PARG);
            else
                calc_jump_sline();
        } else {
            int sline = getnum(&s, (char*)"j", &err);
            if (err)
                error((char*)"Invalid line number", NULL_PARG);
            else {
                jump_sline = sline;
                jump_sline_fraction = -1;
            }
        }
        break;
    case QUERY:
        if (jump_sline_fraction < 0) {
            parg.p_int = jump_sline;
            error((char*)"Position target at screen line %d", parg);
        } else {

            sprintf(buf, ".%06ld", jump_sline_fraction);
            len = (int)strlen(buf);
            while (len > 2 && buf[len - 1] == '0')
                len--;
            buf[len] = '\0';
            parg.p_string = buf;
            error((char*)"Position target at screen position %s", parg);
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
    char buf[30]; // Make bigger than Long int in chars
    int len;
    int err;

    switch (type) {
    case INIT:
    case TOGGLE:
        if (*s == '.') {
            s++;
            shift_count_fraction = getfraction(&s, (char*)"#", &err);
            if (err)
                error((char*)"Invalid column fraction", NULL_PARG);
            else
                calc_shift_count();
        } else {
            int hs = getnum(&s, (char*)"#", &err);
            if (err)
                error((char*)"Invalid column number", NULL_PARG);
            else {
                shift_count = hs;
                shift_count_fraction = -1;
            }
        }
        break;
    case QUERY:
        if (shift_count_fraction < 0) {
            parg.p_int = shift_count;
            error((char*)"Horizontal shift %d columns", parg);
        } else {

            sprintf(buf, ".%06ld", shift_count_fraction);
            len = (int)strlen(buf);
            while (len > 2 && buf[len - 1] == '0')
                len--;
            buf[len] = '\0';
            parg.p_string = buf;
            error((char*)"Horizontal shift %s of screen width", parg);
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
    case INIT:
        if (lesskey(s, 0)) {
            parg.p_string = s;
            error((char*)"Cannot use lesskey file \"%s\"", parg);
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
    position_t pos;

    switch (type) {
    case INIT:
        tagoption = utils::save(s);
        /* Do the rest in main() */
        break;
    case TOGGLE:

        findtag(utils::skipsp(s));
        save_ifile = save_curr_ifile();
        /*
         * Try to open the file containing the tag
         * and search for the tag in that file.
         */
        if (edit_tagfile() || (pos = tagsearch()) == NULL_POSITION) {
            /* Failed: reopen the old file. */
            reedit_ifile(save_ifile);
            break;
        }
        unsave_ifile(save_ifile);
        jump_loc(pos, jump_sline);
        break;
    }
}

/*
 * Handler for -T option.
 */
void opt__T(int type, char* s)
{
    parg_t parg;
    char* filename;

    switch (type) {
    case INIT:
        tags = utils::save(s);
        break;
    case TOGGLE:
        s = utils::skipsp(s);
        if (tags != NULL && tags != ztags)
            free(tags);
        filename = lglob(s);
        tags = shell_unquote(filename);
        free(filename);
        break;
    case QUERY:
        parg.p_string = tags;
        error((char*)"Tags file \"%s\"", parg);
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
    case INIT:
        /*
         * Unget a command for the specified string.
         */
        if (less_is_more) {
            /*
             * In "more" mode, the -p argument is a command,
             * not a search string, so we don't need a slash.
             */
            every_first_cmd = utils::save(s);
        } else {
            plusoption = true;
            ungetcc(CHAR_END_COMMAND);
            ungetsc(s);
            /*
             * {{ This won't work if the "/" command is
             *    changed or invalidated by a .lesskey file. }}
             */
            ungetsc((char*)"/");
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
    case INIT:
    case TOGGLE:
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
    case QUERY:
        parg.p_string = prproto[pr_type];
        error((char*)"%s", parg);
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
    case INIT:
    case TOGGLE:
        /*
         * Set the new number of buffers.
         */
        ch::ch_setbufspace(bufspace);
        break;
    case QUERY:
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
    case TOGGLE:
        chg_caseless();
        break;
    case QUERY:
    case INIT:
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
    case TOGGLE:
    case QUERY:
        dispversion();
        break;
    case INIT:
        /*
         * Force output to stdout per GNU standard for --version output.
         */
        any_display = true;
        putstr("less ");
        putstr(version);
        putstr(" (");
        putstr(pattern_lib_name());
        putstr(" regular expressions)\n");
        putstr("Copyright (C) 1984-2020  Mark Nudelman\n\n");
        putstr("less comes with NO WARRANTY, to the extent permitted by law.\n");
        putstr("For information about the terms of redistribution,\n");
        putstr("see the file named README in the less distribution.\n");
        putstr("Home page: http://www.greenwoodsoftware.com/less\n");
        utils::quit(QUIT_OK);
        break;
    }
}

/*
 * Handler for the -x option.
 */
void opt_x(int type, char* s)
{
    extern int tabstops[];
    extern int ntabstops;
    extern int tabdefault;
    constexpr const int MSG_SIZE = 60 + (4 * TABSTOP_MAX);
    char msg[MSG_SIZE];
    int i;
    parg_t p;

    switch (type) {
    case INIT:
    case TOGGLE:
        /* Start at 1 because tabstops[0] is always zero. */
        for (i = 1; i < TABSTOP_MAX;) {
            int n = 0;
            s = utils::skipsp(s);
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
        ntabstops = i;
        tabdefault = tabstops[ntabstops - 1] - tabstops[ntabstops - 2];
        break;
    case QUERY:
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
        error((char*)"%s", p);
        break;
    }
}

/*
 * Handler for the -" option.
 */
void opt_quote(int type, char* s)
{
    char buf[3];
    parg_t parg;

    switch (type) {
    case INIT:
    case TOGGLE:
        if (s[0] == '\0') {
            openquote = closequote = '\0';
            break;
        }
        if (s[1] != '\0' && s[2] != '\0') {
            error((char*)"-\" must be followed by 1 or 2 chars", NULL_PARG);
            return;
        }
        openquote = s[0];
        if (s[1] == '\0')
            closequote = openquote;
        else
            closequote = s[1];
        break;
    case QUERY:
        buf[0] = openquote;
        buf[1] = closequote;
        buf[2] = '\0';
        parg.p_string = buf;
        error((char*)"quotes %s", parg);
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
    case INIT:
    case TOGGLE: {
        char* fmt;
        int attr = AT_STANDOUT;
        setfmt(s, &fmt, &attr, (char*)"*s>");
        if (strcmp(fmt, "-") == 0) {
            rscroll_char = 0;
        } else {
            rscroll_char = *fmt ? *fmt : '>';
            rscroll_attr = attr;
        }
        break;
    }
    case QUERY: {
        p.p_string = rscroll_char ? (char*)prchar(rscroll_char) : (char*)"-";
        error((char*)"rscroll char is %s", p);
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
    case QUERY:
    case TOGGLE:
        error((char*)"Use \"h\" for help", NULL_PARG);
        break;
    case INIT:
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
    case TOGGLE:
        if (mousecap == OPT_OFF)
            deinit_mouse();
        else
            init_mouse();
        break;
    case INIT:
    case QUERY:
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
    case INIT:
    case TOGGLE:
        if (wheel_lines <= 0)
            wheel_lines = default_wheel_lines();
        break;
    case QUERY:
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
