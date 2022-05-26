#ifndef IFILE_H
#define IFILE_H

//
// An IFILE represents an input file.
//
// It is actually a pointer to an ifile structure,
// but is opaque outside this module.
// Ifile structures are kept in a linked list in the order they
// appear on the command line.
// Any new file which does not already appear in the list is
// inserted after the current file.
//
// test
#include "less.hpp"

/*
 * An IFILE represents an input file.
 */

namespace ifile {

class Ifile {

public:

    Ifile(const char* filename);
    Ifile(const Ifile& src);
    Ifile& operator=(const Ifile& src);
    ~Ifile();
    friend bool operator==(const Ifile& lhs, const Ifile& rhs);

    //const char* filename();
  

    /* 
     * Accessor functions
     */

    // Name of the file
    char* getFilename() { return this->h_filename; };
    void setFilename(const char* newFilename);

    // Saved Position within the file
    scrpos getPos() { return this->h_scrpos; };
    void setPos(scrpos pos);

    // Has the file been opened?
    bool getOpened() { return this->h_opened; };
    void setOpened(bool opened);

    // Hold Count
    int getHoldCount() { return this->h_hold; };
    void setHold(int incr);

    // File state - used in ch.cpp
    void* getFilestate() { return this->h_filestate; };
    void setFilestate(void* filestate);

    // Alt Pipe
    void* getAltpipe() { return this->h_altpipe; };
    void setAltpipe(void* altpipePtr);

    // Alt filename
    char* getAltfilename() { return this->h_altfilename; };
    void setAltfilename(char* altfilename);

private:
    char* h_filename; /* Name of the file */
    void* h_filestate; /* File state (used in ch.c) */ // TODO: Change this
    int h_hold; /* Hold count */
    bool h_opened; /* Has this ifile been opened? */
    scrpos h_scrpos; /* Saved position within the file */
    void* h_altpipe; /* Alt pipe */
    char* h_altfilename; /* Alt filename */
};

//
Ifile* getCurrentIfile();
void setCurrentIfile(Ifile* newCurrentIfile);
Ifile* getOldIfile();
void setOldIfile(Ifile* newOldIfile);

// Get Ifile from a filename
Ifile* getIfile(const char* filename);

// Create an Ifile from a filename
void createIfile(const char* filename);

// Delete the ifile
void deleteIfile(Ifile* ifilePtr);

Ifile* nextIfile(Ifile* current);
Ifile* prevIfile(Ifile* current);
Ifile* getOffIfile(Ifile* thisIfile);

// -----------------------------
// Ifile list specific operations
// -----------------------------

// Get what the index is of the current ifile in the currently held list of files
int getIndex(const Ifile* ifile);

// Get next Ifile in the list
Ifile* nextIfile();

// Get number of Ifiles in the list
int numIfiles();

} // namespace ifile

#endif
