/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */


/*
 * Routines dealing with getting input from the keyboard (i.e. from the user).
 */

#include "less.h"
public int tty;
extern int sigs;
extern int utf_mode;
extern int wheel_lines;

/*
 * Open keyboard for input.
 */
public void
open_getchr(VOID_PARAM)
{

    /*
     * Try /dev/tty.
     * If that doesn't work, use file descriptor 2,
     * which in Unix is usually attached to the screen,
     * but also usually lets you read from the keyboard.
     */
    tty = open("/dev/tty", OPEN_READ);
    if (tty < 0)
        tty = 2;

}

/*
 * Close the keyboard.
 */
public void
close_getchr(VOID_PARAM)
{
}


/*
 * Get the number of lines to scroll when mouse wheel is moved.
 */
public int
default_wheel_lines(VOID_PARAM)
{
    int lines = 1;
    return lines;
}

/*
 * Get a character from the keyboard.
 */
public int
getchr(VOID_PARAM)
{
    char c;
    int result;

    do
    {
        {
            unsigned char uc;
            result = iread(tty, &uc, sizeof(char));
            c = (char) uc;
        }
        if (result == READ_INTR)
            return (READ_INTR);
        if (result < 0)
        {
            /*
             * Don't call error() here,
             * because error calls getchr!
             */
            quit(QUIT_ERROR);
        }


        /*
         * Various parts of the program cannot handle
         * an input character of '\0'.
         * If a '\0' was actually typed, convert it to '\340' here.
         */
        if (c == '\0')
            c = '\340';
    } while (result != 1);

    return (c & 0xFF);
}
