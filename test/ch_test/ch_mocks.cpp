#include "ch_mocks.hpp"
#include "../mocks.hpp"
#include "ch.hpp"
#include "forwback.hpp"
#include "less.hpp"
#include "mark.hpp"
#include <iostream>
#include <stdio.h>
#include <string>

// Maybe need to initialize all these in the fixture
int autobuf;
char helpdata[] = { '\n', 'S', '\b', 'S', 'U', '\b', 'U', 'M' };
int size_helpdata = sizeof(helpdata) - 1;
int logfile = -1;
int follow_mode; /* F cmd Follows file desc or file name? */
ino_t curr_ino;
dev_t curr_dev;
screen_trashed_t screen_trashed = TRASHED;
int sigs;
char* namelogfile = NULL;

// log a string to stdout
void log(std::string s, bool highlight)
{
  std::string hl_str;
  if (highlight) 
    hl_str += "*** ";
  else 
    hl_str += "    ";

  if (ENABLE_LOGGING)
      std::cout << hl_str << " log : " << s << "\n";
}



void error(char* fmt, parg_t parg)
{
    log(std::string("error fmt:" + std::string(fmt) + " "));

    GetMock<ChMock>().error(fmt, parg);
}
void ierror(char* fmt, parg_t parg)
{
    log(std::string("ierror fmt:" + std::string(fmt) + " "));
    GetMock<ChMock>().ierror(fmt, parg);
}

int iread(int fd, unsigned char* buf, unsigned int len)
{
    log(std::string("iread: " + std::string((char*)buf)));
    int res = GetMock<ChMock>().iread(fd, buf, len);
    return res;
}

void clear_eol(void)
{
    log(std::string("clear_eol"));
    GetMock<ChMock>().clear_eol();
}

char* wait_message(void)
{
    char* res = GetMock<ChMock>().wait_message();
    return res;
}

position_t filesize(int f)
{
    log(std::string("filesize:"));
    position_t res = GetMock<ChMock>().filesize(f);
    return res;
}

//----------------------------------------
namespace ifile {

Ifile::Ifile(const char* f) {
    // nothing
};

Ifile* getCurrentIfile()
{
    log("In getCurrentIfile");
    Ifile* res = GetMock<ChMock>().getCurrentIfile();
    return res;
}

} // ifile
//----------------------------------------

// other
__off_t lseek(int fd, __off_t offset, int whence)
{
    log(std::string("lseek"));
    __off_t res = GetMock<ChMock>().lseek(fd, offset, whence);
    return res;
}