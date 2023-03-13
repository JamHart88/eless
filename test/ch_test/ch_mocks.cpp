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
const int TRUNCATE_LIMIT = 100;
const int STR_TAIL_LEN = 20;

void log(std::string s, bool highlight)
{
  std::string hl_str;
  if (highlight) 
    hl_str += "*** ";
  else 
    hl_str += "    ";

  if (ENABLE_LOGGING) {
    std::string s_trunc = s;
    int maxLen = s.length();
    int strTailLen = maxLen - STR_TAIL_LEN;
    if (maxLen > TRUNCATE_LIMIT) 
        s_trunc = 
            s.substr(0, TRUNCATE_LIMIT) + 
            "...[" + 
            std::to_string(maxLen-TRUNCATE_LIMIT-STR_TAIL_LEN) + 
            "]..." +
            s.substr(strTailLen, STR_TAIL_LEN);

    std::cout << hl_str << " log : '" << s_trunc << "'\n";
  }
}

//************************************************************
// output
//************************************************************

void error(char* fmt, parg_t parg)
{
    //log(std::string("error fmt:" + std::string(fmt) + " "));

    GetMock<ChMock>().error(fmt, parg);
}

void ierror(char* fmt, parg_t parg)
{
    //log(std::string("ierror fmt:" + std::string(fmt) + " "));
    GetMock<ChMock>().ierror(fmt, parg);
}

//************************************************************
// os
//************************************************************

int iread(int fd, unsigned char* buf, unsigned int len)
{
    log(std::string("iread: " + std::string((char*)buf)));
    log(
        std::string("iread (fd ") + 
        std::to_string(fd) +
        ", buf .... " + 
        ", len " + 
        std::to_string(len) +
        ")"  );

    int res = GetMock<ChMock>().iread(fd, buf, len);
    return res;
}

//************************************************************
// screen
//************************************************************

void clear_eol(void)
{
    log(std::string("clear_eol"));
    GetMock<ChMock>().clear_eol();
}

//************************************************************
// prompt
//************************************************************

char* wait_message(void)
{
    char* res = GetMock<ChMock>().wait_message();
    return res;
}

//************************************************************
// filename
//************************************************************

position_t filesize(int f)
{
    //log(std::string("filesize:"));
    position_t res = GetMock<ChMock>().filesize(f);
    return res;
}

//************************************************************
// ifile
//************************************************************

namespace ifile {

Ifile::Ifile(const char* f) {
    // nothing
};

Ifile* getCurrentIfile()
{
    //log("In getCurrentIfile");
    Ifile* res = GetMock<ChMock>().getCurrentIfile();
    return res;
}


} // ifile


//************************************************************
// system calls
//************************************************************

__off_t lseek(int fd, __off_t offset, int whence)
{
    log(
        std::string("lseek (fd ") + 
        std::to_string(fd) +
        ", offset " + 
        std::to_string(offset) +
        ", whence " + 
        std::to_string(whence) +
        ")"  );
    __off_t res = GetMock<ChMock>().lseek(fd, offset, whence);
    return res;
}

int close(int fd)
{
    //log(std::string("close"));
    int res = GetMock<ChMock>().close(fd);
    return res;
}

ssize_t write (int fd, const void * buf, size_t n) {
    
    char* lclbuf = (char*)calloc(n + 1, sizeof(char));
    memcpy(lclbuf, buf, n);
    //lclbuf[n+1]='\0';

    log(std::string("write - fd = ") + std::to_string(fd) + 
        " buf= '" + std::string(lclbuf) +
        "' len= " + std::to_string(n) );

    ssize_t res = GetMock<ChMock>().write(fd, buf, n);
    
    free(lclbuf);

    return res;
}

int stat (const char * file, struct stat * buf) {

    int res = GetMock<ChMock>().stat(file, buf);
    return res;
}


//************************************************************
// debug
//************************************************************

static FILE* fp_trace;

void trace_begin(void)
{
    fp_trace = fopen("debug.out", "w");
}


void debug(const char* str)
{
    if (fp_trace == nullptr) 
        trace_begin();
    if (fp_trace != nullptr) 
        fprintf(fp_trace, "DBG: %s\n", str);
}

void trace_end(void) {
    fclose(fp_trace);
}