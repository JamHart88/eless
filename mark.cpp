/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

#include "mark.hpp"
#include "ch.hpp"
#include "edit.hpp"
#include "filename.hpp"
#include "ifile.hpp"
#include "jump.hpp"
#include "less.hpp"
#include "output.hpp"
#include "position.hpp"
#include "screen.hpp"
#include "utils.hpp"

// TODO: Move to namespaces
extern int sc_height;
extern int jump_sline;
extern int perma_marks;

int marks_modified = 0;

namespace mark {

/*
 * A mark is an ifile (input file) plus a position within the file.
 */
struct mark {
  /*
   * Normally m_ifile != IFILE_NULL and m_filename == NULL.
   * For restored marks we set m_filename instead of m_ifile
   * because we don't want to create an ifile until the
   * user explicitly requests the file (by name or mark).
   */
  char          m_letter;   /* Associated character */
  ifile::Ifile* m_ifile;    /* Input file being marked */
  char*         m_filename; /* Name of the input file */
  struct scrpos m_scrpos;   /* Position of the mark */
};

/*
 * The table of marks.
 * Each mark is identified by a lowercase or uppercase letter.
 * The final one is lmark, for the "last mark"; addressed by the apostrophe.
 */
#define NMARKS ((2 * 26) + 2)  /* a-z, A-Z, mousemark, lastmark */
#define NUMARKS ((2 * 26) + 1) /* user marks (not lastmark) */
#define MOUSEMARK (NMARKS - 2)
#define LASTMARK (NMARKS - 1)
static struct mark marks[NMARKS];


/*
 * Initialize a mark struct.
 */
static void cmark(struct mark* m, ifile::Ifile* ifile, position_t pos, int ln)
{
  m->m_ifile      = ifile;
  m->m_scrpos.pos = pos;
  m->m_scrpos.ln  = ln;
  m->m_filename   = NULL;
}

/*
 * Initialize the mark table to show no marks are set.
 */

void init_mark(void)
{
  int i;

  for (i = 0; i < NMARKS; i++) {
    char letter;
    switch (i) {
    case MOUSEMARK:
      letter = '#';
      break;
    case LASTMARK:
      letter = '\'';
      break;
    default:
      letter = (i < 26) ? 'a' + i : 'A' + i - 26;
      break;
    }
    marks[i].m_letter = letter;
    cmark(&marks[i], nullptr, NULL_POSITION, -1);
  }
}

/*
 * Set m_ifile and screen::clear m_filename.
 */
static void mark_set_ifile(struct mark* m, ifile::Ifile* ifile)
{
  m->m_ifile = ifile;
  /* With m_ifile set, m_filename is no longer needed. */
  free(m->m_filename);
  m->m_filename = NULL;
}

/*
 * Populate the m_ifile member of a mark struct from m_filename.
 */
static void mark_get_ifile(struct mark* m)
{
  if (m->m_ifile != nullptr)
    return; /* m_ifile is already set */
  mark_set_ifile(m, ifile::getIfile(m->m_filename));
}

/*
 * Return the user mark struct identified by a character.
 */
static struct mark* getumark(int c)
{
  char mc = static_cast<char>(c);
  debug::debug("getumark : ", mc);

  if (c >= 'a' && c <= 'z')
    return (&marks[c - 'a']);
  if (c >= 'A' && c <= 'Z')
    return (&marks[c - 'A' + 26]);
  if (c == '#')
    return (&marks[MOUSEMARK]);
  output::error((char*)"Invalid mark letter", NULL_PARG);
  return (NULL);
}

/*
 * Get the mark structure identified by a character.
 * The mark struct may either be in the mark table (user mark)
 * or may be constructed on the fly for certain characters like ^, $.
 */
static struct mark* getmark(int c)
{
  struct mark*       m;
  static struct mark sm;

  switch (c) {
  case '^':
    /*
     * Beginning of the current file.
     */
    m = &sm;
    cmark(m, ifile::getCurrentIfile(), ch_zero, 0);
    break;
  case '$':
    /*
     * End of the current file.
     */
    if (ch::end_seek()) {
      output::error((char*)"Cannot seek to end of file", NULL_PARG);
      return (NULL);
    }
    m = &sm;
    cmark(m, ifile::getCurrentIfile(), ch::tell(), sc_height);
    break;
  case '.':
    /*
     * Current position in the current file.
     */
    m = &sm;
    position::get_scrpos(&m->m_scrpos, TOP);
    cmark(m, ifile::getCurrentIfile(), m->m_scrpos.pos, m->m_scrpos.ln);
    break;
  case '\'':
    /*
     * The "last mark".
     */
    m = &marks[LASTMARK];
    break;
  default:
    /*
     * Must be a user-defined mark.
     */
    m = getumark(c);
    if (m == NULL)
      break;
    if (m->m_scrpos.pos == NULL_POSITION) {
      output::error((char*)"Mark not set", NULL_PARG);
      return (NULL);
    }
    break;
  }
  return (m);
}

/*
 * Is a mark letter invalid?
 */

int badmark(int c)
{
  return (getmark(c) == NULL);
}

/*
 * Set a user-defined mark.
 */

void setmark(int c, int where)
{
  struct mark*  m;
  struct scrpos scrpos;

  m = getumark(c);
  if (m == NULL)
    return;
  position::get_scrpos(&scrpos, where);
  if (scrpos.pos == NULL_POSITION) {
    screen::bell();
    return;
  }
  cmark(m, ifile::getCurrentIfile(), scrpos.pos, scrpos.ln);
  marks_modified = 1;
}

/*
 * Clear a user-defined mark.
 */

void clrmark(int c)
{
  struct mark* m;

  m = getumark(c);
  if (m == NULL)
    return;
  if (m->m_scrpos.pos == NULL_POSITION) {
    screen::bell();
    return;
  }
  m->m_scrpos.pos = NULL_POSITION;
  marks_modified  = 1;
}

/*
 * Set lmark (the mark named by the apostrophe).
 */

void lastmark(void)
{
  struct scrpos scrpos;

  if (ch::getflags() & CH_HELPFILE)
    return;
  position::get_scrpos(&scrpos, TOP);
  if (scrpos.pos == NULL_POSITION)
    return;
  cmark(&marks[LASTMARK], ifile::getCurrentIfile(), scrpos.pos, scrpos.ln);
}

/*
 * Go to a mark.
 */

void gomark(int c)
{
  struct mark*  m;
  struct scrpos scrpos;

  m = getmark(c);
  if (m == NULL)
    return;

  /*
   * If we're trying to go to the lastmark and
   * it has not been set to anything yet,
   * set it to the beginning of the current file.
   * {{ Couldn't we instead set marks[LASTMARK] in edit()? }}
   */
  if (m == &marks[LASTMARK] && m->m_scrpos.pos == NULL_POSITION)
    cmark(m, ifile::getCurrentIfile(), ch_zero, jump_sline);

  mark_get_ifile(m);

  /* Save scrpos; if it's LASTMARK it could change in edit_ifile. */
  scrpos = m->m_scrpos;
  if (m->m_ifile != ifile::getCurrentIfile()) {
    /*
     * Not in the current file; edit the correct file.
     */
    if (edit::edit_ifile(m->m_ifile))
      return;
  }

  jump::jump_loc(scrpos.pos, scrpos.ln);
}

/*
 * Return the position associated with a given mark letter.
 *
 * We don't return which screen line the position
 * is associated with, but this doesn't matter much,
 * because it's always the first non-blank line on the screen.
 */

position_t markpos(int c)
{
  struct mark* m;

  m = getmark(c);
  if (m == NULL)
    return (NULL_POSITION);

  if (m->m_ifile != ifile::getCurrentIfile()) {
    output::error((char*)"Mark not in current file", NULL_PARG);
    return (NULL_POSITION);
  }
  return (m->m_scrpos.pos);
}

/*
 * Return the mark associated with a given position, if any.
 */

char posmark(position_t pos)
{
  int i;

  /* Only user marks */
  for (i = 0; i < NUMARKS; i++) {
    if (marks[i].m_ifile == ifile::getCurrentIfile() && marks[i].m_scrpos.pos == pos) {
      if (i < 26)
        return 'a' + i;
      if (i < 26 * 2)
        return 'A' + (i - 26);
      return '#';
    }
  }
  return 0;
}

/*
 * Clear the marks associated with a specified ifile.
 */

void unmark(ifile::Ifile* ifilePtr)
{
  int i;

  for (i = 0; i < NMARKS; i++)
    if (marks[i].m_ifile == ifilePtr)
      marks[i].m_scrpos.pos = NULL_POSITION;
}

/*
 * Check if any marks refer to a specified ifile vi m_filename
 * rather than m_ifile.
 */

void mark_check_ifile(ifile::Ifile* ifilePtr)
{
  int   i;
  char* filename = filename::lrealpath(ifilePtr->getFilename());

  for (i = 0; i < NMARKS; i++) {
    struct mark* m             = &marks[i];
    char*        mark_filename = m->m_filename;
    if (mark_filename != NULL) {
      mark_filename = filename::lrealpath(mark_filename);
      if (strcmp(filename, mark_filename) == 0)
        mark_set_ifile(m, ifilePtr);
      free(mark_filename);
    }
  }
  free(filename);
}

#if CMD_HISTORY

/*
 * Save marks to history file.
 */

void save_marks(FILE* fout, char* hdr)
{
  int i;

  debug::debug("Save marks 1");

  if (!perma_marks)
    return;

  debug::debug("save marks 2");
  ignore_result(fprintf(fout, "%s\n", hdr));

  for (i = 0; i < NUMARKS; i++) {
    debug::debug("Mark:", i);
    char*        filename;
    struct mark* m = &marks[i];

    debug::debug("mark m_letter", m->m_letter);
    debug::debug("mark filename", m->m_filename);

    if (m->m_scrpos.pos == NULL_POSITION)
      continue;

    int   posStrLen = utils::strlen_bound<position_t>();
    char* posStrPtr = new char[posStrLen];

    utils::typeToStr<position_t>(m->m_scrpos.pos, posStrPtr, posStrLen);

    debug::debug("mark pos: ", posStrPtr);

    filename = m->m_filename;
    if (filename == NULL)
      filename = m->m_ifile->getFilename();
    filename = filename::lrealpath(filename);

    debug::debug("filename:", filename);
    debug::debug("ln: ", m->m_scrpos.ln);

    if (strcmp(filename, "-") != 0)
      ignore_result(fprintf(fout, "m %c %d %s %s\n",
          m->m_letter, m->m_scrpos.ln, posStrPtr, filename));
    free(filename);
    delete[] posStrPtr;
  }
}

void skip_whitespace(char*& line)
{
  while (*line == ' ')
    line++;
}
/*
 * Restore one mark from the history file.
 */

void restore_mark(char* line)
{
  // Format is "m <markchar> <screenpos> <position> <file>"

  char* token = strtok(line, " ");

  if (token == NULL)
    return;
  if (token[0] != 'm')
    return;

  // Get mark type from <markchar>
  char*        markChar = strtok(NULL, " ");
  struct mark* markPtr  = getumark(markChar[0]);
  if (markPtr == NULL)
    return;

  // get the <screenpos> parameter
  int screenpos = utils::strToType<int>(strtok(NULL, " "));

  if (screenpos < 1)
    screenpos = 1;
  if (screenpos > sc_height)
    screenpos = sc_height;

  // get the <position> value
  position_t filePos = utils::strToType<position_t>(strtok(NULL, " "));

  cmark(markPtr, nullptr, filePos, screenpos);

  // get the <filename> parameter
  markPtr->m_filename = utils::save(strtok(NULL, " "));
}

#endif /* CMD_HISTORY */

} // namespace mark