#ifndef TAGS_H
#define TAGS_H

/*
 * Reading Tags files
*/

#include "less.hpp"

 void cleantags (void);
 int gettagtype (void);
 void findtag (char *tag);
 POSITION tagsearch (void);
 char * nexttag (int n);
 char * prevtag (int n);
 int ntags (void);
 int curr_tag (void);
 int edit_tagfile (void);

#endif