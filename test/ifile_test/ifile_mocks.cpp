#include "less.hpp"
#include "ifile.hpp"
#include "mark.hpp"
#include "../mocks.hpp"
#include "ifile_mocks.hpp"
#include <iostream>
#include <stdio.h>
#include <string>



//------------------------------------------------
// from filename.cpp
char * filename::lrealpath(const char *path) {

  //std::cout << "--> filename::lrealpath got string " << path << "\n";

  char * res = GetMock<UtilsMock>().filename::lrealpath(path);

  //std::cout << "    filename::lrealpath is returning string " << res << "\n";

	return res;
}

// ------------------------------------------------
namespace utils {

char* save(const char* s){
  //std::cout << "===> Save got string " << s << "\n";

  char * res = GetMock<UtilsMock>().save(s);

  //std::cout << "      Save is returning string " << res << "\n";
	return res;
};

} // utils namespace

 
//-------------------------------------------------
// From mark.cpp
//
//
//

void unmark(ifile::Ifile * ifilePtr) {
  GetMock<UtilsMock>().unmark(ifilePtr);
}


void mark_check_ifile(ifile::Ifile * ifilePtr)
{
  GetMock<UtilsMock>().mark_check_ifile(ifilePtr);
}

//position_t markpos(int c) {
//	position_t t = (position_t) 0;
//	return t;
//};

