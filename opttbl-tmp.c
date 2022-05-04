/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */


/*
 * The option table.
 */

#include "less.h"
#include "option.h"

/*
 * Variables controlled by command line options.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// static struct loption option[] =
static struct loption option[] = {
    { 'a', &a_optname,
    	TRIPLE, OPT_ONPLUS, &how_search, NULL,
    	{
    		(char *)"Search includes displayed screen",
    		(char *)"Search skips displayed screen",
    		(char *)"Search includes all of displayed screen"
    	}
    },

    { 'b', &b_optname,
    	NUMBER|INIT_HANDLER, 64, &bufspace, opt_b, 
    	{
    		(char *)"Max buffer space per file (K): ",
    		(char *)"Max buffer space per file: %dK",
    		NULL
    	}
    },
    { 'B', &B__optname,
    	BOOL, OPT_ON, &autobuf, NULL,
    	{
    		(char *)"Don't automatically allocate buffers",
    		(char *)"Automatically allocate buffers when needed",
    		NULL
    	}
    },
    { 'c', &c_optname,
    	TRIPLE, OPT_OFF, &top_scroll, NULL,
    	{
    		(char *)"Repaint by scrolling from bottom of screen",
    		(char *)"Repaint by painting from top of screen",
    		(char *)"Repaint by painting from top of screen"
    	}
    },
    { 'd', &d_optname,
    	BOOL|NO_TOGGLE, OPT_OFF, &know_dumb, NULL,
    	{
    		(char *)"Assume intelligent terminal",
    		(char *)"Assume dumb terminal",
    		NULL
    	}
    },
    { 'e', &e_optname,
    	TRIPLE, OPT_OFF, &quit_at_eof, NULL,
    	{
    		(char *)"Don't quit at end-of-file",
    		(char *)"Quit at end-of-file",
    		(char *)"Quit immediately at end-of-file"
    	}
    },
    { 'f', &f_optname,
    	BOOL, OPT_OFF, &force_open, NULL,
    	{
    		(char *)"Open only regular files",
    		(char *)"Open even non-regular files",
    		NULL
    	}
    },
    { 'F', &F__optname,
    	BOOL, OPT_OFF, &quit_if_one_screen, NULL,
    	{
    		(char *)"Don't quit if end-of-file on first screen",
    		(char *)"Quit if end-of-file on first screen",
    		NULL
    	}
    },
#if HILITE_SEARCH
    { 'g', &g_optname,
    	TRIPLE|HL_REPAINT, OPT_ONPLUS, &hilite_search, NULL,
    	{
    		(char *)"Don't highlight search matches",
    		(char *)"Highlight matches for previous search only",
    		(char *)"Highlight all matches for previous search pattern",
    	}
    },
#endif
    { 'h', &h_optname,
    	NUMBER, -1, &back_scroll, NULL,
    	{
    		(char *)"Backwards scroll limit: ",
    		(char *)"Backwards scroll limit is %d lines",
    		NULL
    	}
    },
    { 'i', &i_optname,
    	TRIPLE|HL_REPAINT, OPT_OFF, &caseless, opt_i,
    	{
    		(char *)"Case is significant in searches",
    		(char *)"Ignore case in searches",
    		(char *)"Ignore case in searches and in patterns"
    	}
    },
    { 'j', &j_optname,
    	STRING, 0, NULL, opt_j,
    	{
    		(char *)"Target line: ",
    		(char *)"0123456789.-",
    		NULL
    	}
    },
    { 'J', &J__optname,
    	BOOL|REPAINT, OPT_OFF, &status_col, NULL,
    	{
    		(char *)"Don't display a status column",
    		(char *)"Display a status column",
    		NULL
    	}
    },
#if USERFILE
    { 'k', &k_optname,
    	STRING|NO_TOGGLE|NO_QUERY, 0, NULL, opt_k,
    	{ NULL, NULL, NULL }
    },
#endif
    { 'K', &K__optname,
    	BOOL, OPT_OFF, &quit_on_intr, NULL,
    	{
    		(char *)"Interrupt (ctrl-C) returns to prompt",
    		(char *)"Interrupt (ctrl-C) exits less",
    		NULL
    	}
    },
    { 'L', &L__optname,
    	BOOL, OPT_ON, &use_lessopen, NULL,
    	{
    		(char *)"Don't use the LESSOPEN filter",
    		(char *)"Use the LESSOPEN filter",
    		NULL
    	}
    },
    { 'm', &m_optname,
    	TRIPLE, OPT_OFF, &pr_type, NULL,
    	{
    		(char *)"Short prompt",
    		(char *)"Medium prompt",
    		(char *)"Long prompt"
    	}
    },
    { 'n', &n_optname,
    	TRIPLE|REPAINT, OPT_ON, &linenums, NULL,
    	{
    		(char *)"Don't use line numbers",
    		(char *)"Use line numbers",
    		(char *)"constly display line numbers"
    	}
    },
#if LOGFILE
    { 'o', &o_optname,
    	STRING, 0, NULL, opt_o,
    	{ "log file: ", NULL, NULL }
    },
    { 'O', &O__optname,
    	STRING, 0, NULL, opt__O,
    	{ "Log file: ", NULL, NULL }
    },
#endif
    { 'p', &p_optname,
    	STRING|NO_TOGGLE|NO_QUERY, 0, NULL, opt_p,
    	{ NULL, NULL, NULL }
    },
    { 'P', &P__optname,
    	STRING, 0, NULL, opt__P,
    	{ "prompt: ", NULL, NULL }
    },
    { 'q', &q_optname,
    	TRIPLE, OPT_OFF, &quiet, NULL,
    	{
    		(char *)"Ring the bell for errors AND at eof/bof",
    		(char *)"Ring the bell for errors but not at eof/bof",
    		(char *)"Never ring the bell"
    	}
    },
    { 'r', &r_optname,
    	TRIPLE|REPAINT, OPT_OFF, &ctldisp, NULL,
    	{
    		(char *)"Display control characters as ^X",
    		(char *)"Display control characters directly",
    		(char *)"Display control characters directly, processing ANSI sequences"
    	}
    },
    { 's', &s_optname,
    	BOOL|REPAINT, OPT_OFF, &squeeze, NULL,
    	{
    		(char *)"Display all blank lines",
    		(char *)"Squeeze multiple blank lines",
    		NULL
    	}
    },
    { 'S', &S__optname,
    	BOOL|REPAINT, OPT_OFF, &chopline, NULL,
    	{
    		(char *)"Fold long lines",
    		(char *)"Chop long lines",
    		NULL
    	}
    },
#if TAGS
    { 't', &t_optname,
    	STRING|NO_QUERY, 0, NULL, opt_t,
    	{ "tag: ", NULL, NULL }
    },
    { 'T', &T__optname,
    	STRING, 0, NULL, opt__T,
    	{ "tags file: ", NULL, NULL }
    },
#endif
    { 'u', &u_optname,
    	TRIPLE|REPAINT, OPT_OFF, &bs_mode, NULL,
    	{
    		(char *)"Display underlined text in underline mode",
    		(char *)"Backspaces cause overstrike",
    		(char *)"Print backspace as ^H"
    	}
    },
    { 'V', &V__optname,
    	NOVAR, 0, NULL, opt__V,
    	{ NULL, NULL, NULL }
    },
    { 'w', &w_optname,
    	TRIPLE|REPAINT, OPT_OFF, &show_attn, NULL,
    	{
    		(char *)"Don't highlight first unread line",
    		(char *)"Highlight first unread line after forward-screen",
    		(char *)"Highlight first unread line after any forward movement",
    	}
    },
    { 'x', &x_optname,
    	STRING|REPAINT, 0, NULL, opt_x,
    	{
    		(char *)"Tab stops: ",
    		(char *)"0123456789,",
    		NULL
    	}
    },
    { 'X', &X__optname,
    	BOOL|NO_TOGGLE, OPT_OFF, &no_init, NULL,
    	{
    		(char *)"Send init/deinit strings to terminal",
    		(char *)"Don't use init/deinit strings",
    		NULL
    	}
    },
    { 'y', &y_optname,
    	NUMBER, -1, &forw_scroll, NULL,
    	{
    		(char *)"Forward scroll limit: ",
    		(char *)"Forward scroll limit is %d lines",
    		NULL
    	}
    },
    { 'z', &z_optname,
    	NUMBER, -1, &swindow, NULL,
    	{
    		(char *)"Scroll window size: ",
    		(char *)"Scroll window size is %d lines",
    		NULL
    	}
    },
    { '"', &quote_optname,
    	STRING, 0, NULL, opt_quote,
    	{ "quotes: ", NULL, NULL }
    },
    { '~', &tilde_optname,
    	BOOL|REPAINT, OPT_ON, &twiddle, NULL,
    	{
    		(char *)"Don't show tildes after end of file",
    		(char *)"Show tildes after end of file",
    		NULL
    	}
    },
    { '?', &query_optname,
    	NOVAR, 0, NULL, opt_query,
    	{ NULL, NULL, NULL }
    },
    { '#', &pound_optname,
    	STRING, 0, NULL, opt_shift,
    	{
    		(char *)"Horizontal shift: ",
    		(char *)"0123456789.",
    		NULL
    	}
    },
    { OLETTER_NONE, &keypad_optname,
    	BOOL|NO_TOGGLE, OPT_OFF, &no_keypad, NULL,
    	{
    		(char *)"Use keypad mode",
    		(char *)"Don't use keypad mode",
    		NULL
    	}
    },
    { OLETTER_NONE, &oldbot_optname,
    	BOOL, OPT_OFF, &oldbot, NULL,
    	{
    		(char *)"Use new bottom of screen behavior",
    		(char *)"Use old bottom of screen behavior",
    		NULL
    	}
    },
    { OLETTER_NONE, &follow_optname,
    	BOOL, FOLLOW_DESC, &follow_mode, NULL,
    	{
    		(char *)"F command follows file descriptor",
    		(char *)"F command follows file name",
    		NULL
    	}
    },
    { OLETTER_NONE, &use_backslash_optname,
    	BOOL, OPT_OFF, &opt_use_backslash, NULL,
    	{
    		(char *)"Use backslash escaping in command line parameters",
    		(char *)"Don't use backslash escaping in command line parameters",
    		NULL
    	}
    },
    { OLETTER_NONE, &rscroll_optname,
    	STRING|REPAINT|INIT_HANDLER, 0, NULL, opt_rscroll,
    	{ "right scroll character: ", NULL, NULL }
    },
    { OLETTER_NONE, &nohistdups_optname,
    	BOOL, OPT_OFF, &no_hist_dups, NULL,
    	{
    		(char *)"Allow duplicates in history list",
    		(char *)"Remove duplicates from history list",
    		NULL
    	}
    },
    { OLETTER_NONE, &mousecap_optname,
    	TRIPLE, OPT_OFF, &mousecap, opt_mousecap,
    	{
    		(char *)"Ignore mouse input",
    		(char *)"Use the mouse for scrolling",
    		(char *)"Use the mouse for scrolling (reverse)"
    	}
    },
    { OLETTER_NONE, &wheel_lines_optname,
    	NUMBER|INIT_HANDLER, 0, &wheel_lines, opt_wheel_lines,
    	{
    		(char *)"Lines to scroll on mouse wheel: ",
    		(char *)"Scroll %d line(s) on mouse wheel",
    		NULL
    	}
    },
    { OLETTER_NONE, &perma_marks_optname,
    	BOOL, OPT_OFF, &perma_marks, NULL,
    	{
    		(char *)"Don't save marks in history file",
    		(char *)"Save marks in history file",
    		NULL
    	}
    },
    { '\0', NULL, NOVAR, 0, NULL, NULL, { NULL, NULL, NULL } }
};


/*
 * Initialize each option to its default value.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public void
// init_option(VOID_PARAM)
public void init_option(VOID_PARAM)
{
    struct loption *o;
    char *p;

    p = lgetenv("LESS_IS_MORE");
    if (!isnullenv(p))
    	less_is_more = 1;

    for (o = option;  o->oletter != '\0';  o++)
    {
    	/*
    	 * Set each variable to its default.
    	 */
    	if (o->ovar != NULL)
    		*(o->ovar) = o->odefault;
    	if (o->otype & INIT_HANDLER)
    		(*(o->ofunc))(INIT, (char *) NULL);
    }
}

/*
 * Find an option in the option table, given its option letter.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public struct loption *
// findopt(c)
// 	int c;
public struct loption * findopt(int c)
{
    struct loption *o;

    for (o = option;  o->oletter != '\0';  o++)
    {
    	if (o->oletter == c)
    		return (o);
    	if ((o->otype & TRIPLE) && ASCII_TO_UPPER(o->oletter) == c)
    		return (o);
    }
    return (NULL);
}

/*
 *
 */
// -------------------------------------------
// Converted from C to C++ - C below
// static int
// is_optchar(c)
// 	char c;
static int is_optchar(char c)
{
    if (ASCII_IS_UPPER(c))
    	return 1;
    if (ASCII_IS_LOWER(c))
    	return 1;
    if (c == '-')
    	return 1;
    return 0;
}

/*
 * Find an option in the option table, given its option name.
 * p_optname is the (possibly partial) name to look for, and
 * is updated to point after the matched name.
 * p_oname if non-NULL is set to point to the full option name.
 */
// -------------------------------------------
// Converted from C to C++ - C below
// public struct loption *
// findopt_name(p_optname, p_oname, p_err)
// 	char **p_optname;
// 	char **p_oname;
// 	int *p_err;
public struct loption * findopt_name(char **p_optname, char **p_oname, int *p_err)
{
    char *optname = *p_optname;
    struct loption *o;
    struct optname *oname;
    int len;
    int uppercase;
    struct loption *maxo = NULL;
    struct optname *maxoname = NULL;
    int maxlen = 0;
    int ambig = 0;
    int exact = 0;

    /*
     * Check all options.
     */
    for (o = option;  o->oletter != '\0';  o++)
    {
    	/*
    	 * Check all names for this option.
    	 */
    	for (oname = o->onames;  oname != NULL;  oname = oname->onext)
    	{
    		/* 
    		 * Try normal match first (uppercase == 0),
    		 * then, then if it's a TRIPLE option,
    		 * try uppercase match (uppercase == 1).
    		 */
    		for (uppercase = 0;  uppercase <= 1;  uppercase++)
    		{
    			len = sprefix(optname, oname->oname, uppercase);
    			if (len <= 0 || is_optchar(optname[len]))
    			{
    				/*
    				 * We didn't use all of the option name.
    				 */
    				continue;
    			}
    			if (!exact && len == maxlen)
    				/*
    				 * Already had a partial match,
    				 * and now there's another one that
    				 * matches the same length.
    				 */
    				ambig = 1;
    			else if (len > maxlen)
    			{
    				/*
    				 * Found a better match than
    				 * the one we had.
    				 */
    				maxo = o;
    				maxoname = oname;
    				maxlen = len;
    				ambig = 0;
    				exact = (len == (int)strlen(oname->oname));
    			}
    			if (!(o->otype & TRIPLE))
    				break;
    		}
    	}
    }
    if (ambig)
    {
    	/*
    	 * Name matched more than one option.
    	 */
    	if (p_err != NULL)
    		*p_err = OPT_AMBIG;
    	return (NULL);
    }
    *p_optname = optname + maxlen;
    if (p_oname != NULL)
    	*p_oname = maxoname == NULL ? NULL : maxoname->oname;
    return (maxo);
}
