
#include <execinfo.h>
#include <link.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace debug {
// File for debugging.

#ifdef __cplusplus
extern "C" {
#endif
/* Static functions. */

/* Note that these are linked internally by the compiler.
 * Don't call them directly! */
void __cyg_profile_func_enter(void* this_fn, void* call_site) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void* this_fn, void* call_site) __attribute__((no_instrument_function));

#ifdef __cplusplus
};
#endif

static FILE* fp_trace;

void __attribute__((constructor)) __attribute__((__no_instrument_function__)) trace_begin(void)
{
  fp_trace = fopen("trace.out", "w");
}

void __attribute__((destructor)) __attribute__((__no_instrument_function__)) trace_end(void)
{
  if (fp_trace != NULL) {
    fclose(fp_trace);
  }
}

// converts a function's address in memory to its VMA address in the executable file. VMA is what addr2line expects
size_t __attribute__((__no_instrument_function__)) ConvertToVMA(void* addr)
{
  Dl_info          info;
  struct link_map* link_map;
  dladdr1(addr, &info, (void**)&link_map, RTLD_DL_LINKMAP);
  return link_map->l_addr;
  // return (size_t) addr;
}

void __attribute__((__no_instrument_function__)) __cyg_profile_func_enter(void* func, void* caller)
{
  if (fp_trace != NULL) {
    // Need to convert the func address to the VMA offset - so can use nm to find the func name from the log file
    size_t funcVMA = ConvertToVMA(func);
    size_t diff    = (size_t)func - funcVMA;
    // This should match the value found in nm
    fprintf(fp_trace, "e func: %p caller: %p time: %lu nm_addr: %p\n", func, caller, time(NULL), (void*)diff);

    // e.g. gives
    //   e 0x562eaa7239e0 0x7fdba18f7d90 1651848632 0x562eaa71d000 0x69e0
    //                                                             ^^^^^^
    // jamie@old-work-laptop:~/Nextcloud/Progs/myless$ nm -o  less  | grep main
    // less:                 U __libc_start_main@GLIBC_2.34
    // less:00000000000069e0 T main
    //                 ^^^^^
    // less:0000000000004a7f t main.cold

    // PrintCallStack();
  }
}

void __attribute__((__no_instrument_function__)) __cyg_profile_func_exit(void* func, void* caller)
{
  if (fp_trace != NULL) {

    // Need to convert the func address to the VMA offset - so can use nm to find the func name from the log file
    size_t funcVMA = ConvertToVMA(func);
    size_t diff    = (size_t)func - funcVMA;
    // This should match the value found in nm
    fprintf(fp_trace, "x func: %p caller: %p time: %lu nm_addr: %p\n", func, caller, time(NULL), (void*)diff);
  }
}

void __attribute__((__no_instrument_function__)) debug(const char* str)
{
  if (fp_trace != NULL) {
    fprintf(fp_trace, "DBG: %s\n", str);
  }
}

void __attribute__((__no_instrument_function__)) debug(const char* str1, const char* str2)
{
  if (fp_trace != NULL) {
    fprintf(fp_trace, "DBG: %s%s\n", str1, str2);
  }
}

void __attribute__((__no_instrument_function__)) debug(const char* str, const int val)
{
  if (fp_trace != NULL) {
    fprintf(fp_trace, "DBG: %s%d\n", str, val);
  }
}

void __attribute__((__no_instrument_function__)) debug(const char* str1, const int linenum, const char* str2)
{
  if (fp_trace != NULL) {
    fprintf(fp_trace, "DBG: FILE: %s LINE: %d : ", str1, linenum);
    fprintf(fp_trace, "'%s'\n", str2);
    fflush(fp_trace);
  }
}

} // namespace debug