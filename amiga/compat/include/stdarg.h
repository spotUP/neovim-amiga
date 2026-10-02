/* ixemul 48.2's <stdarg.h> (va_list is the char * its library takes) with
 * va_start on gcc's __builtin_va_start. The SDK's va_start is
 * __builtin_next_arg, which gcc does not count as variadic use: it then
 * clones a variadic inline (IPA constant propagation, "event_create.constprop"
 * in Neovim) and fails with "va_start used in function with fixed args".
 * On m68k both give the address of the first unnamed argument, so the
 * SDK's va_arg/va_copy keep working on the result. (Request R2.) */
#ifndef AMIGA_COMPAT_STDARG_H
#define AMIGA_COMPAT_STDARG_H
#pragma GCC system_header
#include_next <stdarg.h>
#undef va_start
#define va_start(ap, last) __builtin_va_start(*(__builtin_va_list *)&(ap), last)
#endif
