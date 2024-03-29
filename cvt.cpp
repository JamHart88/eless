/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * Routines to convert text in various ways.  Used by search.
 */

#include "cvt.hpp"
#include "charset.hpp"
#include "less.hpp"
#include "line.hpp"
#include "utils.hpp"

namespace cvt {

/*
 * Get the length of a buffer needed to convert a string.
 */
int cvt_length(int len, int ops)
{
  if (less::Globals::utf_mode)
    /*
     * Just copying a string in UTF-8 mode can cause it to grow
     * in length.
     * Four output bytes for one input byte is the worst case.
     */
    len *= 4;
  return (len + 1);
}

/*
 * Allocate a chpos array for use by cvt_text.
 */
int* cvt_alloc_chpos(int len)
{
  int  i;
  int* chpos = (int*)utils::ecalloc(sizeof(int), len);
  /* Initialize all entries to an invalid position. */
  for (i = 0; i < len; i++)
    chpos[i] = -1;
  return (chpos);
}

/*
 * Convert text.  Perform the transformations specified by ops.
 * Returns converted text in odst.  The original offset of each
 * odst character (when it was in osrc) is returned in the chpos array.
 */
void cvt_text(char* odst,
    char*           osrc,
    int*            chpos,
    int*            lenp,
    int             ops)
{
  char*    dst;
  char*    edst = odst;
  char*    src;
  char*    src_end;
  lwchar_t ch;

  if (lenp != NULL)
    src_end = osrc + *lenp;
  else
    src_end = osrc + strlen(osrc);

  for (src = osrc, dst = odst; src < src_end;) {
    int src_pos = (int)(src - osrc);
    int dst_pos = (int)(dst - odst);
    ch          = charset::step_char(&src, +1, src_end);
    if ((ops & CVT_BS) && ch == '\b' && dst > odst) {
      /* Delete backspace and preceding char. */
      do {
        dst--;
      } while (dst > odst && less::Globals::utf_mode && !IS_ASCII_OCTET(*dst) && !IS_UTF8_LEAD(*dst));
    } else if ((ops & CVT_ANSI) && is_csi_start(ch)) {
      /* Skip to end of ANSI escape sequence. */
      src++; /* skip the csi_char start char */
      while (src < src_end)
        if (!line::is_ansi_middle(*src++))
          break;
    } else {
      /* Just copy the char to the destination buffer. */
      if ((ops & CVT_TO_LC) && isupper(ch))
        ch = tolower(ch);
      charset::put_wchar(&dst, ch);
      /* Record the original position of the char. */
      if (chpos != NULL)
        chpos[dst_pos] = src_pos;
    }
    if (dst > edst)
      edst = dst;
  }
  if ((ops & CVT_CRLF) && edst > odst && edst[-1] == '\r')
    edst--;
  *edst = '\0';
  if (lenp != NULL)
    *lenp = (int)(edst - odst);
  /* FIXME: why was this here?  if (chpos != NULL) chpos[dst - odst] = src - osrc; */
}

}; // namespace cvt