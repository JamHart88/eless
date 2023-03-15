#ifndef DEBUG_H
#define DEBUG_H

// Temporary Debuggng routines

namespace debug {

#define D(a) debug(__FILE__, __LINE__, a);
void debug(const char* str);
void debug(const char* str1, const char* str2);
void debug(const char* str, const int val);
void debug(const char* str1, const int line, const char* str2);

}; // namespace debug

#endif