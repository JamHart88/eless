#ifndef DEFINES_H
#define DEFINES_H

/* Unix definition file for less.  C++
 *
 * This file has 3 sections:
 * User preferences.
 * Globals always true on Unix.
 * Globals automatically determined by configure.
 *
 * * * * * *  WARNING  * * * * * *
 * If you edit defines.h by hand, do "touch stamp-h" before you run make
 * so config.status doesn't overwrite your changes.
 */

/* ----------------------------------------------------
 * Configurational compilation options
 * ----------------------------------------------------*/
#ifdef NOT_CXX11
#define nullptr NULL
#endif

// TBD

/* ----------------------------------------------------
 * Optional user options
 * ----------------------------------------------------*/

/*
 * SHELL_ESCAPE is 1 if you wish to allow shell escapes.
 * (This is possible only if your system supplies the system() function.)
 */
#define SHELL_ESCAPE 1

/*
 * EXAMINE is 1 if you wish to allow examining files by name from within less.
 */
#define EXAMINE 1

/*
 * TAB_COMPLETE_FILENAME is 1 if you wish to allow the TAB key
 * to complete filenames at prompts.
 */
#define TAB_COMPLETE_FILENAME 1

/*
 * CMD_HISTORY is 1 if you wish to allow keys to cycle through
 * previous commands at prompts.
 */
#define CMD_HISTORY 1

/*
 * HILITE_SEARCH is 1 if you wish to have search targets to be
 * displayed in standout mode.
 */
#define HILITE_SEARCH 1

/*
 * EDITOR is 1 if you wish to allow editor invocation (the "v" command).
 * (This is possible only if your system supplies the system() function.)
 * EDIT_PGM is the name of the (default) editor to be invoked.
 */
#define EDITOR 1

/*
 * TAGS is 1 if you wish to support tag files.
 */
#define TAGS 1

/*
 * USERFILE is 1 if you wish to allow a .less file to specify
 * user-defined key bindings.
 */
#define USERFILE 1

/*
 * GLOB is 1 if you wish to have shell metacharacters expanded in filenames.
 * This will generally work if your system provides the "popen" function
 * and the "echo" shell command.
 */
#define GLOB 1

/*
 * PIPEC is 1 if you wish to have the "|" command
 * which allows the user to pipe data into a shell command.
 */
#define PIPEC 1

/*
 * GNU_OPTIONS is 1 if you wish to support the GNU-style command
 * line options --help and --version.
 */
#define GNU_OPTIONS 1

/*
 * ONLY_RETURN is 1 if you want RETURN to be the only input which
 * will continue past an output::error message.
 * Otherwise, any key will continue past an output::error message.
 */
#define ONLY_RETURN 0

/*
 * LESSKEYFILE is the filename of the default decode::lesskey output file
 * (in the HOME directory).
 * LESSKEYFILE_SYS is the filename of the system-wide decode::lesskey output file.
 * DEF_LESSKEYINFILE is the filename of the default decode::lesskey input
 * (in the HOME directory).
 * LESSHISTFILE is the filename of the history file
 * (in the HOME directory).
 */
#define LESSKEYFILE ".less"
#define LESSKEYFILE_SYS SYSDIR "/sysless"
#define DEF_LESSKEYINFILE ".decode::lesskey"
#define LESSHISTFILE ".lesshst"

#define DEBUG 1

/* ----------------------------------------------------
 * Globals always true on Unix.
 * ----------------------------------------------------  */

/*
 * Pathname separator character.
 */
#define PATHNAME_SEP "/"

/*
 * The value returned from tgetent on success.
 * Some HP-UX systems return 0 on success.
 */
#define TGETENT_OK 1

/*
 * Define if you have the <sgstat.h> header file.
 */
/* #undef HAVE_SGSTAT_H */

/*
 * Default shell metacharacters and meta-escape character.
 */
#define DEF_METACHARS "; *?\t\n'\"()<>[]|&^`#\\$%=~{},"
#define DEF_METAESCAPE "\\"

/*
 * Sizes of various buffers.
 */
constexpr const int CMDBUF_SIZE   = 2048; // Buffer for multichar commands
constexpr const int UNGOT_SIZE    = 200;  // Max chars to unget()
constexpr const int LINEBUF_SIZE  = 1024; // Initial max size of line in input file
constexpr const int OUTBUF_SIZE   = 1024; // Output buffer
constexpr const int PROMPT_SIZE   = 2048; // Max size of prompt string
constexpr const int TERMBUF_SIZE  = 2048; // Termcap buffer for tgetent
constexpr const int TERMSBUF_SIZE = 1024; // Buffer to hold termcap strings
constexpr const int TAGLINE_SIZE  = 1024; // Max size of line in tags file
constexpr const int TABSTOP_MAX   = 128;  // Max number of custom tab stops

/* ----------------------------------------------------
 * Globals automatically determined by configure.
 * ----------------------------------------------------*/

/* Define EDIT_PGM to your editor. */
#define EDIT_PGM "vi"

/* Define HAVE_CONST if your compiler supports the "const" modifier. */
#define HAVE_CONST 1

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* Define HAVE_ERRNO if you have the errno variable. */
#define HAVE_ERRNO 1

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the `fchmod' function. */
#define HAVE_FCHMOD 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define HAVE_FILENO if you have the fileno() macro. */
#define HAVE_FILENO 1

/* Define HAVE_FLOAT if your compiler supports the "double" type. */
#define HAVE_FLOAT 1

/* Define to 1 if you have the `fsync' function. */
#define HAVE_FSYNC 1

/* GNU regex library */
/* #undef HAVE_GNU_REGEX */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define HAVE_LOCALE if you have locale.h and setlocale. */
#define HAVE_LOCALE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define HAVE_OSPEED if your termcap library has the ospeed variable. */
#define HAVE_OSPEED 1

/* PCRE (Perl-compatible regular expression) library */
/* #undef HAVE_PCRE */

/* PCRE2 (Perl-compatible regular expression) library */
/* #undef HAVE_PCRE2 */

/* Define to 1 if you have the `popen' function. */
#define HAVE_POPEN 1

/* POSIX regcomp() and regex.h */
#define HAVE_POSIX_REGCOMP 1

/* Define to 1 if you have the `realpath' function. */
#define HAVE_REALPATH 1

/* System V regcmp() */
/* #undef HAVE_REGCMP */

/* */
/* #undef HAVE_REGEXEC2 */

/* BSD re_comp() */
/* #undef HAVE_RE_COMP */

/* Define to 1 if you have the `sigsetmask' function. */
#define HAVE_SIGSETMASK 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_snprintf1

/* Define to 1 if you have the `stat' function. */
#define HAVE_STAT 1

/* Define HAVE_STAT_INO if your struct stat has st_ino and st_dev. */
#define HAVE_STAT_INO 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define HAVE_STRERROR if you have the strerror() function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `system' function. */
#define HAVE_SYSTEM 1

/* Define HAVE_SYS_ERRLIST if you have the sys_errlist[] variable. */
#define HAVE_SYS_ERRLIST 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/stream.h> header file. */
/* #undef HAVE_SYS_STREAM_H */

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <termcap.h> header file. */
#define HAVE_TERMCAP_H 1

/* Define HAVE_TERMIOS_FUNCS if you have tcgetattr/tcsetattr. */
#define HAVE_TERMIOS_FUNCS 1

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if you have the <termio.h> header file. */
#define HAVE_TERMIO_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Henry Spencer V8 regcomp() and regexp.h */
/* #undef HAVE_V8_REGCOMP */

/* Define to 1 if you have the <values.h> header file. */
#define HAVE_VALUES_H 1

/* Define HAVE_WCTYPE if you have iswupper, iswlower, towupper, towlower. */
#define HAVE_WCTYPE 1

/* Define to 1 if you have the <wctype.h> header file. */
#define HAVE_WCTYPE_H 1

/* Define MUST_DEFINE_ERRNO if you have errno but it is not define in errno.h.
 */
/* #undef MUST_DEFINE_ERRNO */

/* Define MUST_DEFINE_OSPEED if you have ospeed but it is not defined in
   termcap.h. */
/* #undef MUST_DEFINE_OSPEED */

/* pattern matching is supported, but without metacharacters. */
/* #undef NO_REGEX */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "less"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "less 1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "less"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1"

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */
#endif
