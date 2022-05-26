/*
 * Copyright (C) 1984-2020  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information, see the README file.
 */

/*
 * An IFILE represents an input file.
 *
 * It is actually a pointer to an ifile structure,
 * but is opaque outside this module.
 * Ifile structures are kept in a linked list in the order they
 * appear on the command line.
 * Any new file which does not already appear in the list is
 * inserted after the current file.
 */

#include "ifile.hpp"
#include "filename.hpp"
#include "less.hpp"
#include "mark.hpp"
#include "utils.hpp"

#include <iostream>

#include <vector>

/*
 * Convert an IFILE (external representation)
 * to a struct file (internal representation), and vice versa.
 */
#define int_ifile(h) ((struct ifile*)(h))
#define ext_ifile(h) ((IFILE)(h))

namespace ifile {

// ifile list
std::vector<Ifile*> fileList;

namespace {
    Ifile* currentIfile = nullptr; // TODO: need to ensure this is updated in all cases
    Ifile* previousIfile = nullptr; // TODO: need to ensure this is updated in all cases.
}

/*
 * Get pointer to the current ifile
 */
Ifile* getCurrentIfile()
{
    return currentIfile;
}

void setCurrentIfile(Ifile* newCurrentIfile)
{
    currentIfile = newCurrentIfile;
}

/*
 * Get pointer to the old ifile
 */
Ifile* getOldIfile()
{
    return previousIfile;
}

void setOldIfile(Ifile* newOldIfile)
{
    previousIfile = newOldIfile;
}

/*
 * Return the number of ifiles.
 */
int numIfiles(void)
{
    return (fileList.size());
}

/*
 * Find Ifile and return its index. Index starts at 1
 */
int find(Ifile* current)
{
    int idx = 0;
    for (Ifile* f : fileList) {
        idx++;
        if (f == current) {
            return idx;
        }
    }
    return -1;
}

/*
 * Allocate a new ifile structure and stick a filename in it.
 * It should go after "prev" in the list
 * (or at the beginning of the list if "prev" is NULL).
 * Return a pointer to the new ifile structure.
 */
Ifile* newIfile(const char* filename)
{
    Ifile* ret = new Ifile(filename);

    fileList.push_back(ret);
    currentIfile = fileList.back();

    /* {{ It's dodgy to call mark.c functions from here;
     *    there is potentially dangerous recursion.
     *    Probably need to revisit this design. }}
     */
    mark_check_ifile(currentIfile);

    return currentIfile;
}

/*
 * Delete an existing ifile structure.
 */
void deleteIfile(Ifile* ifilePtr)
{

    // std::cout << "deleteIfile 1\n";
    if (ifilePtr != nullptr) {

        std::vector<Ifile*>::iterator it = fileList.begin();

        // find pos in list first
        for (Ifile const* lclIfile : fileList) {
            if (lclIfile == ifilePtr) {
                // std::cout << "Found entry in list index \n";
                break;
            }
            it++; // increment at the end - list starts at 1
        }

        /*
         * If the ifile we're deleting is the currently open ifile,
         * move off it.
         */
        unmark(ifilePtr);

        if (ifilePtr == getCurrentIfile()) {

            setCurrentIfile(getOffIfile(ifilePtr));
        }
        fileList.erase(it);
        delete (ifilePtr);
    }
}

/*
 * Get the ifile after a given one in the list.
 */
Ifile* nextIfile(Ifile* current)
{
    Ifile* ret = nullptr;

    if (fileList.size() == 0)
        ret = nullptr;
    else if (current == nullptr)
        ret = fileList.front();
    else {
        int idx = find(current) - 1; // Turn index to 0 index
        // std::cout << "nextifile idx = " << idx << " size = " << fileList.size() << "\n";

        if (idx > -1 && idx + 1 < static_cast<int>(fileList.size()))
            ret = fileList.at(idx + 1);
    }

    return ret;
}

/*
 * Get the ifile before a given one in the list.
 */
Ifile* prevIfile(Ifile* current)
{
    Ifile* ret = nullptr;

    if (fileList.size() == 0)
        ret = nullptr;
    else if (current == nullptr)
        ret = fileList.back();
    else {
        int idx = find(current) - 1; // -1 to make index start at 0

        // std::cout << "previfile idx = " << idx << " size = " << fileList.size() << "\n";
        if (idx > -1 && idx > 0)
            return fileList.at(idx - 1);
    }
    return ret;
}

/*
 * Return a different ifile from the given one.
 * The previous one (if there is one) or the next one (if there is one),
 * otherwise NULL
 */
Ifile* getOffIfile(Ifile* thisIfile)
{

    Ifile* newIfile;

    if ((newIfile = prevIfile(thisIfile)) != nullptr)
        return newIfile;
    if ((newIfile = nextIfile(thisIfile)) != nullptr)
        return newIfile;
    return nullptr;
}

/*
 * Find an ifile structure, given a filename.
 */

Ifile* findIfile(const char* searchFilename)
{
    Ifile* ret = nullptr;

    char* realSearchFilename = lrealpath(searchFilename);

    for (Ifile* current : fileList) {

        if (strcmp(searchFilename, current->getFilename()) == 0 || strcmp(realSearchFilename, current->getFilename()) == 0) {

            ret = current;
            /*
             * If given name is shorter than the name we were
             * previously using for this file, adopt shorter name.
             */
            if (strlen(searchFilename) < strlen(current->getFilename())) {
                current->setFilename(searchFilename);
            }
            break;
        }
    }

    free(realSearchFilename);

    return ret;
}

/*
 * Get the ifile associated with a filename.
 * If the filename has not been seen before,
 * will create a new Ifile
 */
Ifile* getIfile(const char* filename)
{
    Ifile* ifile = findIfile(filename);
    if (ifile == nullptr) {
        ifile = newIfile(filename);
        // std::cout << "getIfile ifile =" << ifile << "\n";
    }
    return ifile;
}

void createIfile(const char* filename)
{
    (void)getIfile(filename);
}

/*
 * Get the index of the file associated with a ifile.
 * Index start at 1
 */
int getIndex(const Ifile* ifile)
{

    int index = 1;
    bool found = false;

    for (Ifile* current : fileList) {

        if (current == ifile) {
            found = true;
            break;
        }
        index++;
    }
    if (!found)
        index = -1;

    return index;
}

//==========================================================
// Ifile class functions
//==========================================================

/*
 * Ifile constructor
 */
Ifile::Ifile(const char* filename)
{
    h_filename = utils::save(filename);
    h_filestate = NULL;
    h_hold = 0;
    h_opened = false;
    h_scrpos.pos = NULL_POSITION;
    h_scrpos.ln = 0;
    h_altpipe = NULL;
    h_altfilename = NULL;
};

/*
 * Ifile copy constructor
 */
Ifile::Ifile(const Ifile& src)
{
    this->h_filename = utils::save(src.h_filename);
    this->h_filestate = src.h_filestate;
    this->h_hold = src.h_hold;
    this->h_opened = src.h_opened;
    this->h_scrpos = src.h_scrpos;
    this->h_altpipe = NULL;
    this->h_altfilename = src.h_altfilename;
};

/*
 * Operator=
 */
Ifile& Ifile::operator=(const Ifile& src)
{
    if (this != &src) {

        free(this->h_filename);
        this->h_filename = utils::save(src.h_filename);
        this->h_filestate = src.h_filestate;
        this->h_hold = src.h_hold;
        this->h_opened = src.h_opened;
        this->h_scrpos = src.h_scrpos;
        this->h_altpipe = NULL;
        this->h_altfilename = src.h_altfilename;
    }
    return *this;
}
/*
 * Destructor
 */
Ifile::~Ifile()
{

    // free incase it was not done by caller
    if (h_filestate != NULL)
        free(this->h_filestate);

    free(this->h_altfilename);
    free(this->h_filename);
}

/*
 * Set the filename
 */
void Ifile::setFilename(const char* newFilename)
{
    free(this->h_filename);
    h_filename = utils::save(newFilename);
}

/*
 * Set the Pos field
 */
void Ifile::setPos(scrpos pos)
{
    this->h_scrpos.pos = pos.pos;
    this->h_scrpos.ln = pos.ln;
}

/*
 * Set if the file is opened
 */
void Ifile::setOpened(bool opened)
{
    this->h_opened = opened;
}

/*
 * Increment the Hold count by incr amount
 */
void Ifile::setHold(int incr)
{
    this->h_hold += incr;
}

/*
 * Set File State
 * Assumes caller does any deallocation of the filestate
 */
void Ifile::setFilestate(void* filestate)
{
    // No need to free filestate - assume caller does any free
    this->h_filestate = filestate;
}

/*
 * Set the altpipe Ptr
 * Assumes caller does any deallocation
 */

void Ifile::setAltpipe(void* altpipePtr)
{
    this->h_altpipe = altpipePtr;
}

/*
 * Set the AltFilename
 */
void Ifile::setAltfilename(char* altfilename)
{
    if (this->h_altfilename != NULL)
        free(this->h_altfilename);
    this->h_altfilename = altfilename;
}

/*
 * Test equality between 2 Ifiles - done on filename only
 */
bool operator==(const Ifile& lhs, const Ifile& rhs)
{
    return (strcmp(lhs.h_filename, rhs.h_filename) == 0);
}

} // ifile namespace
