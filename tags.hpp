#ifndef TAGS_H
#define TAGS_H

/*
 * Reading Tags files
 */

#include "less.hpp"

namespace tags {

void       cleantags(void);
int        gettagtype(void); // not used
void       findtag(char* tag);
position_t tagsearch(void);
char*      nexttag(int n);
char*      prevtag(int n);
int        ntags(void);
int        curr_tag(void);
int        edit_tagfile(void);

} // namespace tags
#endif