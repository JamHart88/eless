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

#include "less.h"
#include "option.h"

extern int nbufs;
extern int bufspace;
extern int pr_type;
extern int plusoption;
extern int swindow;
extern int sc_width;
extern int sc_height;
extern int dohelp;
extern int any_display;
extern char openquote;
extern char closequote;
extern char *prproto[];
extern char *eqproto;
extern char *hproto;
extern char *wproto;
extern char *every_first_cmd;
extern IFILE curr_ifile;
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
#if LOGFILE
extern char *namelogfile;
extern int force_logfile;
extern int logfile;
#endif
#if TAGS
public char *tagoption = NULL;
extern char *tags;
extern char ztags[];
#endif

#if LOGFILE
/*
 * Handler for -o option.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_o(type, s)
//     int type;
//     char *s;
public void opt_o(int type, char *s)
{
    PARG parg;
    char *filename;

    
    switch (type)
    {
    case INIT:
        namelogfile = save(s);
        break;
    case TOGGLE:
        if (ch_getflags() & CH_CANSEEK)
        {
            error((char *)"Input is not a pipe", NULL_PARG);
            return;
        }
        if (logfile >= 0)
        {
            error((char *)"Log file is already in use", NULL_PARG);
            return;
        }
        s = skipsp(s);
        if (namelogfile != NULL)
            free(namelogfile);
        filename = lglob(s);
        namelogfile = shell_unquote(filename);
        free(filename);
        use_logfile(namelogfile);
        sync_logfile();
        break;
    case QUERY:
        if (logfile < 0)
            error((char *)"No log file", NULL_PARG);
        else
        {
            parg.p_string = namelogfile;
            error((char *)"Log file \"%s\"", &parg);
        }
        break;
    }
}

/*
 * Handler for -O option.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt__O(type, s)
//     int type;
//     char *s;
public void opt__O(int type, char *s)
{
    force_logfile = TRUE;
    opt_o(type, s);
}
#endif

/*
 * Handlers for -j option.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_j(type, s)
//     int type;
//     char *s;
public void opt_j(int type, char *s)
{
    PARG parg;
    char buf[30]; // Make bigger than Long int in chars
    int len;
    int err;

    switch (type)
    {
    case INIT:
    case TOGGLE:
        if (*s == '.')
        {
            s++;
            jump_sline_fraction = getfraction(&s, (char *)"j", &err);
            if (err)
                error((char *)"Invalid line fraction", NULL_PARG);
            else
                calc_jump_sline();
        } else
        {
            int sline = getnum(&s, (char *)"j", &err);
            if (err)
                error((char *)"Invalid line number", NULL_PARG);
            else
            {
                jump_sline = sline;
                jump_sline_fraction = -1;
            }
        }
        break;
    case QUERY:
        if (jump_sline_fraction < 0)
        {
            parg.p_int =  jump_sline;
            error((char *)"Position target at screen line %d", &parg);
        } else
        {

            sprintf(buf, ".%06ld", jump_sline_fraction);
            len = (int) strlen(buf);
            while (len > 2 && buf[len-1] == '0')
                len--;
            buf[len] = '\0';
            parg.p_string = buf;
            error((char *)"Position target at screen position %s", &parg);
        }
        break;
    }
}

// -------------------------------------------
// Converted from C to C++ - C below
// public void
// calc_jump_sline(VOID_PARAM)
public void calc_jump_sline(VOID_PARAM)
{
    if (jump_sline_fraction < 0)
        return;
    jump_sline = sc_height * jump_sline_fraction / NUM_FRAC_DENOM;
}

/*
 * Handlers for -# option.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_shift(type, s)
//     int type;
//     char *s;
public void opt_shift(int type, char *s)
{
    PARG parg;
    char buf[30]; // Make bigger than Long int in chars
    int len;
    int err;

    switch (type)
    {
    case INIT:
    case TOGGLE:
        if (*s == '.')
        {
            s++;
            shift_count_fraction = getfraction(&s, (char *)"#", &err);
            if (err)
                error((char *)"Invalid column fraction", NULL_PARG);
            else
                calc_shift_count();
        } else
        {
            int hs = getnum(&s, (char *)"#", &err);
            if (err)
                error((char *)"Invalid column number", NULL_PARG);
            else
            {
                shift_count = hs;
                shift_count_fraction = -1;
            }
        }
        break;
    case QUERY:
        if (shift_count_fraction < 0)
        {
            parg.p_int = shift_count;
            error((char *)"Horizontal shift %d columns", &parg);
        } else
        {

            sprintf(buf, ".%06ld", shift_count_fraction);
            len = (int) strlen(buf);
            while (len > 2 && buf[len-1] == '0')
                len--;
            buf[len] = '\0';
            parg.p_string = buf;
            error((char *)"Horizontal shift %s of screen width", &parg);
        }
        break;
    }
}
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// calc_shift_count(VOID_PARAM)
public void calc_shift_count(VOID_PARAM)
{
    if (shift_count_fraction < 0)
        return;
    shift_count = sc_width * shift_count_fraction / NUM_FRAC_DENOM;
}

#if USERFILE
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_k(type, s)
//     int type;
//     char *s;
public void opt_k(int type, char *s)
{
    PARG parg;

    switch (type)
    {
    case INIT:
        if (lesskey(s, 0))
        {
            parg.p_string = s;
            error((char *)"Cannot use lesskey file \"%s\"", &parg);
        }
        break;
    }
}
#endif

#if TAGS
/*
 * Handler for -t option.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_t(type, s)
//     int type;
//     char *s;
public void opt_t(int type, char *s)
{
    IFILE save_ifile;
    POSITION pos;

    switch (type)
    {
    case INIT:
        tagoption = save(s);
        /* Do the rest in main() */
        break;
    case TOGGLE:
        
        findtag(skipsp(s));
        save_ifile = save_curr_ifile();
        /*
         * Try to open the file containing the tag
         * and search for the tag in that file.
         */
        if (edit_tagfile() || (pos = tagsearch()) == NULL_POSITION)
        {
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
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt__T(type, s)
//     int type;
//     char *s;
public void opt__T(int type, char *s)
{
    PARG parg;
    char *filename;

    switch (type)
    {
    case INIT:
        tags = save(s);
        break;
    case TOGGLE:
        s = skipsp(s);
        if (tags != NULL && tags != ztags)
            free(tags);
        filename = lglob(s);
        tags = shell_unquote(filename);
        free(filename);
        break;
    case QUERY:
        parg.p_string = tags;
        error((char *)"Tags file \"%s\"", &parg);
        break;
    }
}
#endif

/*
 * Handler for -p option.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_p(type, s)
//     int type;
//     char *s;
public void opt_p(int type, char *s)
{
    switch (type)
    {
    case INIT:
        /*
         * Unget a command for the specified string.
         */
        if (less_is_more)
        {
            /*
             * In "more" mode, the -p argument is a command,
             * not a search string, so we don't need a slash.
             */
            every_first_cmd = save(s);
        } else
        {
            plusoption = TRUE;
            ungetcc(CHAR_END_COMMAND);
            ungetsc(s);
             /*
              * {{ This won't work if the "/" command is
              *    changed or invalidated by a .lesskey file. }}
              */
            ungetsc((char *)"/");
        }
        break;
    }
}

/*
 * Handler for -P option.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt__P(type, s)
//     int type;
//     char *s;
public void opt__P(int type, char *s)
{
    char **proto;
    PARG parg;

    switch (type)
    {
    case INIT:
    case TOGGLE:
        /*
         * Figure out which prototype string should be changed.
         */
        switch (*s)
        {
        case 's':  proto = &prproto[PR_SHORT];    s++;    break;
        case 'm':  proto = &prproto[PR_MEDIUM];    s++;    break;
        case 'M':  proto = &prproto[PR_LONG];    s++;    break;
        case '=':  proto = &eqproto;        s++;    break;
        case 'h':  proto = &hproto;        s++;    break;
        case 'w':  proto = &wproto;        s++;    break;
        default:   proto = &prproto[PR_SHORT];        break;
        }
        free(*proto);
        *proto = save(s);
        break;
    case QUERY:
        parg.p_string = prproto[pr_type];
        error((char *)"%s", &parg);
        break;
    }
}

/*
 * Handler for the -b option.
 */
    /*ARGSUSED*/
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_b(type, s)
//     int type;
//     char *s;
public void opt_b(int type, char *s)
{
    switch (type)
    {
    case INIT:
    case TOGGLE:
        /*
         * Set the new number of buffers.
         */
        ch_setbufspace(bufspace);
        break;
    case QUERY:
        break;
    }
}

/*
 * Handler for the -i option.
 */
    /*ARGSUSED*/
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_i(type, s)
//     int type;
//     char *s;
public void opt_i(int type, char *s)
{
    switch (type)
    {
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
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt__V(type, s)
//     int type;
//     char *s;
public void opt__V(int type, char *s)
{
    switch (type)
    {
    case TOGGLE:
    case QUERY:
        dispversion();
        break;
    case INIT:
        /*
         * Force output to stdout per GNU standard for --version output.
         */
        any_display = 1;
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
        quit(QUIT_OK);
        break;
    }
}


/*
 * Handler for the -x option.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_x(type, s)
//     int type;
//     char *s;
public void opt_x(int type, char *s)
{
    extern int tabstops[];
    extern int ntabstops;
    extern int tabdefault;
    char msg[60+(4*TABSTOP_MAX)];
    int i;
    PARG p;

    switch (type)
    {
    case INIT:
    case TOGGLE:
        /* Start at 1 because tabstops[0] is always zero. */
        for (i = 1;  i < TABSTOP_MAX;  )
        {
            int n = 0;
            s = skipsp(s);
            while (*s >= '0' && *s <= '9')
                n = (10 * n) + (*s++ - '0');
            if (n > tabstops[i-1])
                tabstops[i++] = n;
            s = skipsp(s);
            if (*s++ != ',')
                break;
        }
        if (i < 2)
            return;
        ntabstops = i;
        tabdefault = tabstops[ntabstops-1] - tabstops[ntabstops-2];
        break;
    case QUERY:
        strcpy(msg, "Tab stops ");
        if (ntabstops > 2)
        {
            for (i = 1;  i < ntabstops;  i++)
            {
                if (i > 1)
                    strcat(msg, ",");
                sprintf(msg+strlen(msg), "%d", tabstops[i]);
            }
            sprintf(msg+strlen(msg), " and then ");
        }
        sprintf(msg+strlen(msg), "every %d spaces",
            tabdefault);
        p.p_string = msg;
        error((char *)"%s", &p);
        break;
    }
}


/*
 * Handler for the -" option.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_quote(type, s)
//     int type;
//     char *s;
public void opt_quote(int type, char *s)
{
    char buf[3];
    PARG parg;

    switch (type)
    {
    case INIT:
    case TOGGLE:
        if (s[0] == '\0')
        {
            openquote = closequote = '\0';
            break;
        }
        if (s[1] != '\0' && s[2] != '\0')
        {
            error((char *)"-\" must be followed by 1 or 2 chars", NULL_PARG);
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
        error((char *)"quotes %s", &parg);
        break;
    }
}

/*
 * Handler for the --rscroll option.
 */
    /*ARGSUSED*/
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_rscroll(type, s)
//     int type;
//     char *s;
public void opt_rscroll(int type, char *s)
{
    PARG p;

    switch (type)
    {
    case INIT:
    case TOGGLE: {
        char *fmt;
        int attr = AT_STANDOUT;
        setfmt(s, &fmt, &attr, (char *) "*s>");
        if (strcmp(fmt, "-") == 0)
        {
            rscroll_char = 0;
        } else
        {
            rscroll_char = *fmt ? *fmt : '>';
            rscroll_attr = attr;
        }
        break; }
    case QUERY: {
        p.p_string = rscroll_char ? (char *) prchar(rscroll_char) : (char *) "-";
        error((char *)"rscroll char is %s", &p);
        break; }
    }
}

/*
 * "-?" means display a help message.
 * If from the command line, exit immediately.
 */
    /*ARGSUSED*/
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_query(type, s)
//     int type;
//     char *s;
public void opt_query(int type, char *s)
{
    switch (type)
    {
    case QUERY:
    case TOGGLE:
        error((char *)"Use \"h\" for help", NULL_PARG);
        break;
    case INIT:
        dohelp = 1;
    }
}

/*
 * Handler for the --mouse option.
 */
    /*ARGSUSED*/
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_mousecap(type, s)
//     int type;
//     char *s;
public void opt_mousecap(int type, char *s)
{
    switch (type)
    {
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
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// opt_wheel_lines(type, s)
//     int type;
//     char *s;
public void opt_wheel_lines(int type, char *s)
{
    switch (type)
    {
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
// -------------------------------------------
// Converted from C to C++ - C below
// public int
// get_swindow(VOID_PARAM)
public int get_swindow(VOID_PARAM)
{
    if (swindow > 0)
        return (swindow);
    return (sc_height + swindow);
}
