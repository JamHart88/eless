#ifndef IFILE_MOCKS_H
#define IFILE_MOCKS_H

#include "../mocks.hpp"
#include "ch.hpp"

#include <gtest/gtest.h>
#include "gmock/gmock.h"


const bool ENABLE_LOGGING = false;
void log(std::string s, bool highlight = true);

namespace ifile {

// Need to replace the ifile Class. Could not get it to work via mocking
// just implemented the minimum that ch.cpp needs
class Ifile {
    public:
        virtual ~Ifile() {};
        Ifile(const char * f);
        MOCK_METHOD(void *, getFilestate, ());
        MOCK_METHOD(void, setFilestate, (void * filestate)); 
        MOCK_METHOD(char *, getFilename, ());
    };

Ifile * getCurrentIfile();

} // ifile namespace


class ChMock : public ModuleMock
{
public:
    // output.hpp
    MOCK_METHOD2(error, void(char*, parg_t));
    MOCK_METHOD2(ierror, void (char*, parg_t));

    // os.cpp
    MOCK_METHOD3(iread, int(int, unsigned char*, unsigned int));

    // screen
    MOCK_METHOD0(clear_eol, void (void));

    // prompt
    MOCK_METHOD0(wait_message, char* (void));
    
    // filename
    MOCK_METHOD1(filesize, position_t (int));

    // ifile
    MOCK_METHOD0(getCurrentIfile, ifile::Ifile* (void));

    // 
    MOCK_METHOD3(lseek, __off_t(int, __off_t, int));
};

#endif

