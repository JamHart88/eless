#ifndef PROMPT_H
#define PROMPT_H

/*
 * Prompting and other messages.
 * There are three flavors of prompts, SHORT, MEDIUM and LONG,
 * selected by the -m/-M options.
 * There is also the "equals message", printed by the = command.
 * A prompt is a message composed of various pieces, such as the
 * name of the file being viewed, the percentage into the file, etc.
 */

#include "less.hpp"

void  init_prompt(void);
char* pr_expand(const char* proto, int maxwidth);
char* eq_message(void);
char* pr_string(void);
char* wait_message(void);

#endif