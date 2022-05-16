#ifndef TAGS_H
#define TAGS_H

/*
 * Reading Tags files
*/

#include "less.hpp"

public void cleantags (void);
public int gettagtype (void);
public void findtag (char *tag);
public POSITION tagsearch (void);
public char * nexttag (int n);
public char * prevtag (int n);
public int ntags (void);
public int curr_tag (void);
public int edit_tagfile (void);

#endif