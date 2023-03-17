#ifndef FILENAME_H
#define FILENAME_H

//
// Routines to mess around with filenames (and files).
// Much of this is very OS dependent.
//

#include "less.hpp"
namespace filename {

char*      shell_unquote(char* str);
char*      get_meta_escape(void);
char*      shell_quote(char* s);
char*      homefile(char* filename);
char*      fexpand(char* s);
char*      fcomplete(char* s);
int        bin_file(int f);
char*      lglob(char* filename);
char*      lrealpath(const char* path);
char*      open_altfile(char* filename, int* pf, void** pfd);
void       close_altfile(char* altfilename, char* filename);
int        is_dir(char* filename);
char*      bad_file(char* filename);
position_t filesize(int f);
char*      shell_coption(void);
char*      last_component(char* name);

} // namespace filename
#endif