#ifndef LESS_H
#define LESS_H
/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Standard include file for "less".
 */

/*
 * Include the file of compile-time options.
 * The <> make cc search for it in -I., not srcdir.
 */
#include "defines.hpp"

/*
 * Wrapping your function call with ignore_result makes it more clear to
 * readers, compilers and linters that you are, in fact, ignoring the
 * function's return value on purpose.
 */
static inline void ignore_result(long long int unused_result)
{
    (void)unused_result;
}

/*
 * Language details.
 */

/* Library function declarations */

#include <cstdio>
#include <sys/types.h>

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_WCTYPE_H
#include <cwctype>
#endif

#if HAVE_LIMITS_H
#include <climits>
#endif

#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#if DEBUG
#include "debug.hpp"
#endif

// New C++ includes
#include <cstring>

#include "funcs.hpp"
#include "pattern.hpp"

enum option_t {
    OPT_OFF = 0,
    OPT_ON = 1,
    OPT_ONPLUS = 2
};

/* Bad Seek return value */
constexpr const off_t BAD_LSEEK = -1;

/*
 * Special types and consts.
 */
typedef unsigned long lwchar_t;
typedef off_t position_t;
typedef off_t linenum_t;

/* Min printing width of a line number */
const int MIN_LINENUM_WIDTH = 7;

/* Max bytes in one UTF-8 char */
const int MAX_UTF_CHAR_LEN = 6;

const position_t NULL_POSITION = -1;

/*
 * Flags for open()
 */
const int OPEN_READ = O_RDONLY;

const int OPEN_APPEND = O_APPEND | O_WRONLY;

/*
 * Misc settings
 */

const int SPACES_IN_FILENAMES = 1;

/*
 * An IFILE represents an input file.
 */
// TODO: This should be converted to a Class with private fields
#define IFILE void*
#define NULL_IFILE ((IFILE)NULL)

/*
 * The structure used to represent a "screen position".
 * This consists of a file position, and a screen line number.
 * The meaning is that the line starting at the given file
 * position is displayed on the ln-th line of the screen.
 * (Screen lines before ln are empty.)
 */
struct scrpos {
    position_t pos;
    int ln;
};

union parg_t {
    char* p_string;
    int p_int;
    linenum_t p_linenum;
};

constexpr const parg_t NULL_PARG = { 0 };

struct textlist {
    char* string;
    char* endstring;
};

struct wchar_range {
    lwchar_t first, last;
};

struct wchar_range_table {
    struct wchar_range* table;
    int count;
};

const int EOI = -1;
const int READ_INTR = -2;

/* A fraction is represented by an int n; the fraction is n/NUM_FRAC_DENOM */
const long NUM_FRAC_DENOM = 1000000;
const int NUM_LOG_FRAC_DENOM = 6;

/* How quiet should we be?
   NOT_QUIET     : Ring bell at eof and for errors
   LITTLE_QUIET  : Ring bell only for errors
   VERY_QUIET    : Never ring bell
*/
enum quiet_t {
    NOT_QUIET = 0,
    LITTLE_QUIET = 1,
    VERY_QUIET = 2
};

/* How should we prompt?
    PR_SHORT  : Prompt with colon
    PR_MEDIUM : Prompt with message
    PR_LONG   :  Prompt with longer message
*/
enum prompt_t {
    PR_SHORT = 0,
    PR_MEDIUM = 1,
    PR_LONG = 2
};

/* How should we handle backspaces?
    BS_SPECIAL  : Do special things for underlining and bold
    BS_NORMAL   : \b treated as normal char; actually output
    BS_CONTROL  : \b treated as control char; prints as ^H
*/
enum handle_backspace_t {
    BS_SPECIAL = 0,
    BS_NORMAL = 1,
    BS_CONTROL = 2
};

/* How should we search?
    SRCH_FORW        : Search forward from current position
    SRCH_BACK        : Search backward from current position
    SRCH_NO_MOVE     : Highlight, but don't move
    SRCH_FIND_ALL    : Find and highlight all matches
    SRCH_NO_MATCH    : Search for non-matching lines
    SRCH_PAST_EOF    : Search past end-of-file, into next file
    SRCH_FIRST_FILE  : Search starting at the first file
    SRCH_NO_REGEX    : Don't use regular expressions
    SRCH_FILTER      : Search is for '&' (filter) command
    SRCH_AFTER_TARGET: Start search after the target line
*/
enum search_t {
    SRCH_FORW = (1 << 0),
    SRCH_BACK = (1 << 1),
    SRCH_NO_MOVE = (1 << 2),
    SRCH_FIND_ALL = (1 << 4),
    SRCH_NO_MATCH = (1 << 8),
    SRCH_PAST_EOF = (1 << 9),
    SRCH_FIRST_FILE = (1 << 10),
    SRCH_NO_REGEX = (1 << 12),
    SRCH_FILTER = (1 << 13),
    SRCH_AFTER_TARGET = (1 << 14)
};

//#define SRCH_REVERSE(t) (((t)&SRCH_FORW) ? (((t) & ~SRCH_FORW) | SRCH_BACK) : (((t) & ~SRCH_BACK) | SRCH_FORW))
inline int SRCH_REVERSE(int search)
{
    return (search & SRCH_FORW ? ((search & ~SRCH_FORW) | SRCH_BACK) : ((search & ~SRCH_BACK) | SRCH_FORW));
};

/* */
enum mca_t {
    NO_MCA = 0,
    MCA_DONE = 1,
    MCA_MORE = 2
};

/* cc_type:
    CC_OK    Char was accepted & processed
    CC_QUIT  Char was a request to abort current cmd
    CC_ERROR Char could not be accepted due to error
    CC_PASS  Char was rejected (internal)
*/

enum cc_type_t {
    CC_OK = 0,
    CC_QUIT = 1,
    CC_ERROR = 2,
    CC_PASS = 3
};

const int CF_QUIT_ON_ERASE = 0001; /* Abort cmd if its entirely erased */

/* Special char bit-flags used to tell put_line() to do something special
    AT_NORMAL
    AT_UNDERLINE
    AT_BOLD
    AT_BLINK
    AT_STANDOUT
    AT_ANSI  Content-supplied "ANSI" escape sequence
    AT_BINARY  LESS*BINFMT representation
    AT_HILITE  Internal highlights (e.g., for search)
*/
enum put_line_flags_t {
    AT_NORMAL = (0),
    AT_UNDERLINE = (1 << 0),
    AT_BOLD = (1 << 1),
    AT_BLINK = (1 << 2),
    AT_STANDOUT = (1 << 3),
    AT_ANSI = (1 << 4),
    AT_BINARY = (1 << 5),
    AT_HILITE = (1 << 6)
};

// JPH Replaced the following macro with template
//#define CONTROL(c) ((c)&037)
constexpr unsigned char unitSepChar = 31;
template <typename T>
constexpr inline unsigned char control(T inputChar)
{
    return (inputChar & unitSepChar);
}

/* Define ESC char */
constexpr const unsigned char esc = control<int>('[');

/* Define CSI character */
constexpr const unsigned char csi_char = '\233';

/* is_csi_start: helper functon to detect if chat is ESC or CSI char */
inline bool is_csi_start(lwchar_t c)
{
    return (c == esc || c == csi_char);
}

const lwchar_t CHAR_END_COMMAND = 0x40000000;

/*
 * Signals
 */

const int S_INTERRUPT = 01;
const int S_STOP = 02;
const int S_WINCH = 04;

inline bool is_abort_signal(int sigVal)
{
    return sigVal & (S_INTERRUPT | S_STOP);
}

enum quit_t {
    QUIT_OK = 0,
    QUIT_ERROR = 1,
    QUIT_INTERRUPT = 2,
    QUIT_SAVED_STATUS = (-1),
};

enum follow_t {
    FOLLOW_DESC = 0,
    FOLLOW_NAME = 1
};

/* filestate flags */
enum filestate_t {
    CH_CANSEEK = 001,
    CH_KEEPOPEN = 002,
    CH_POPENED = 004,
    CH_HELPFILE = 010,
    CH_NODATA = 020 /* Special case for zero length files */
};

const position_t ch_zero = 0;

const char * const FAKE_HELPFILE = "@/\\less/\\help/\\file/\\@";
const char * const FAKE_EMPTYFILE = "@/\\less/\\empty/\\file/\\@";

/* Flags for cvt_text */
enum cvt_t {
    CVT_TO_LC = 01, /* Convert upper-case to lower-case */
    CVT_BS = 02, /* Do backspace processing */
    CVT_CRLF = 04, /* Remove CR after LF */
    CVT_ANSI = 010 /* Remove ANSI escape sequences */
};

/* X11 mouse reporting definitions */
enum x11_mouse_t {
    X11MOUSE_BUTTON1 = 0, /* Left button press */
    X11MOUSE_BUTTON2 = 1, /* Middle button press */
    X11MOUSE_BUTTON3 = 2, /* Right button press */
    X11MOUSE_BUTTON_REL = 3, /* Button release */
    X11MOUSE_WHEEL_UP = 0x40, /* Wheel scroll up */
    X11MOUSE_WHEEL_DOWN = 0x41, /* Wheel scroll down */
    X11MOUSE_OFFSET = 0x20 /* Added to button & pos bytes to create a char */
};

struct mlist;
struct loption;
struct hilite_tree;


/* Functions not included in funcs.h */

// ---------------------------------------------------------
// Template functions
// ---------------------------------------------------------
const int TEN = 10;
const char CH_0 = '0';
const char CH_9 = '9';

// ---------------------------------------------------------
// strToType : Convert a string with a number in to its numeric value
// Caller must ensure that type is appropriate for the expected
// number in the string
// ebuf is set to the next character after the number that was
// processed
// TODO: Move to utils. Follow C++ guidance - see clang-tidy
template <typename T>
T strToType(char* buf, char** ebuf)
{
    T val = 0;
    for (;;) {
        char c = *buf++;
        if (c < CH_0 || c > CH_9)
            break;
        val = TEN * val + c - CH_0;
    }
    if (ebuf != NULL)
        *ebuf = buf;
    return val;
}

// ---------------------------------------------------------
// TypetoStr : Convert a numeric type T to its string representation
// Caller must ensure that buf is sized to support max possible
// string length based on the type
// TODO: Move to utils. Follow C++ guidance - see clang-tidy
template <typename T>
void typeToStr(const T num, char* buf)
{
    T tempNum = num;
    int neg = (tempNum < 0);
    char tbuf[((sizeof(tempNum) * 8 - 1) * 302 / 1000 + 1 + 1) + 2];
    char* s = tbuf + sizeof(tbuf);
    if (neg)
        tempNum = -tempNum;
    *--s = '\0';
    do {
        *--s = (tempNum % 10) + '0';
    } while ((tempNum /= 10) != 0);
    if (neg)
        *--s = '-';
    strcpy(buf, s);
}

// ----------------------------------------------------------------
/*
 * Upper bound on the string length of an integer converted to string.
 * 302 / 1000 is ceil (log10 (2.0)).  Subtract 1 for the sign bit;
 * add 1 for integer division truncation; add 1 more for a minus sign.
 */
// TODO: Move to utils. Follow C++ guidance - see clang-tidy
template <typename T>
int strlen_bound(void)
{
    return ((sizeof(T) * CHAR_BIT - 1) * 302 / 1000 + 1 + 1);
}

#endif