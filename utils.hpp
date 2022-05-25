

#ifndef UTILS_H
#define UTILS_H

/*
 * Utility Functions
 */

#include "less.hpp"
#include <cstring>

namespace utils {

char* save(const char* s);
void* ecalloc(int count, unsigned int size);
char* skipsp(char* s);
int sprefix(char* ps, char* s, int uppercase);
void quit(int status);

// ---------------------------------------------------------
// Template functions
// ---------------------------------------------------------
const int TEN = 10;
const char CH_0 = '0';
const char CH_9 = '9';
const char CH_DASH = '-';
const char CH_NULL = '\0';
const char CH_SPACE = ' ';

// ---------------------------------------------------------
// strToType : Convert a string with a number in to its numeric value
// Caller must ensure that type is appropriate for the expected
// number in the string
// ebuf is set to the next character after the number that was
// processed
// TODO: Move to utils. Follow C++ guidance - see clang-tidy
template <typename T>
T strToType(char* buf)
{
    T val = 0;
    for (int i = 0;; i++) {
        char c = buf[i];
        if (c < CH_0 || c > CH_9)
            break;
        val = TEN * val + c - CH_0;
    }

    return val;
}

// ---------------------------------------------------------
// TypetoStr : Convert a numeric type T to its string representation
// Caller must ensure that buf is sized to support max possible
// string length based on the type
// TODO: Move to utils. Follow C++ guidance - see clang-tidy
template <typename T>
void typeToStr(const T num, char* buf, int bufLength)
{
    T tempNum = num;
    int neg = (tempNum < 0);

    // Upper bound on the string length of an integer converted to string.
    // 302 / 1000 is ceil (log10 (2.0)).  Subtract 1 for the sign bit;
    // add 1 for integer division truncation; add 1 more for a minus sign.

    char tbuf[((sizeof(tempNum) * 8 - 1) * 302 / 1000 + 1 + 1) + 2];
    char* currentCharPtr = tbuf + sizeof(tbuf);
    if (neg)
        tempNum = -tempNum;

    *--currentCharPtr = CH_NULL;
    int digits = 1;

    do {
        *--currentCharPtr = (tempNum % 10) + CH_0;
        digits++;
    } while ((tempNum /= 10) != 0);

    if (neg)
        *--currentCharPtr = CH_DASH;

    strncpy(buf, currentCharPtr, bufLength - 1);
}

// ----------------------------------------------------------------
/*
 * Upper bound on the string length of an integer converted to string.
 * 302 / 1000 is ceil (log10 (2.0)).  Subtract 1 for the sign bit;
 * add 1 for integer division truncation; add 2 more for a minus sign
 * and null terminator.
 */
// TODO: Move to utils. Follow C++ guidance - see clang-tidy
template <typename T>
const int strlen_bound(void)
{
    return ((sizeof(T) * CHAR_BIT - 1) * 302 / 1000 + 1 + 2);
}

}

#endif