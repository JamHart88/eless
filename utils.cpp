
/*
 * Utility Functions
 */

#include "utils.hpp"
#include "cmdbuf.hpp"
#include "edit.hpp"
#include "less.hpp"
#include "output.hpp"
#include "screen.hpp"
#include "ttyin.hpp"

bool any_display = false;
int quitting = 0;

extern int is_tty;

namespace utils {

// ------------------------------------------------
/*
 * Copy a string to a "safe" place
 * (that is, to a buffer allocated by calloc).
 */

char* save(const char* s)
{
    char* p;
    int len = strlen(s) + 1;
    p = (char*)ecalloc(len, sizeof(char));
    memcpy(p, s, len);
    return (p);
}

// ------------------------------------------------
// ecalloc: Allocate memory.
// Like calloc(), but never returns an error (NULL).
//

void* ecalloc(int count, unsigned int size)
{
    void* p;

    p = (void*)calloc(count, size);
    if (p != NULL)
        return (p);
    error((char*)"Cannot allocate memory", NULL_PARG);
    quit(QUIT_ERROR);
    /*NOTREACHED*/
    return (NULL);
}

/*
 * Skip leading spaces in a string.
 */

char* skipsp(char* s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    return (s);
}

/*
 * See how many characters of two strings are identical.
 * If uppercase is true, the first string must begin with an uppercase
 * character; the remainder of the first string may be either case.
 */

int sprefix(char* ps, char* s, int uppercase)
{
    int c;
    int sc;
    int len = 0;

    for (; *s != '\0'; s++, ps++) {
        c = *ps;
        if (uppercase) {
            if (len == 0 && iswlower(c))
                return (-1);
            if (iswupper(c))
                c = tolower(c);
        }
        sc = *s;
        if (len > 0 && iswupper(sc))
            sc = tolower(sc);
        if (c != sc)
            break;
        len++;
    }
    return (len);
}

/*
 * Exit the program.
 */

void quit(int status)
{
    static int save_status;

    /*
     * Put cursor at bottom left corner, clear the line,
     * reset the terminal modes, and exit.
     */
    if (status < 0)
        status = save_status;
    else
        save_status = status;
    quitting = 1;
    edit((char*)NULL);
    save_cmdhist();
    if (any_display && is_tty)
        clear_bot();
    deinit();
    flush();
    raw_mode(0);
    close_getchr();
    exit(status);
}
} // utils namespace
