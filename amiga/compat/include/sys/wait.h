/* ixemul 48.2's <sys/wait.h>. Its wait4 reports stopped children
 * (WUNTRACED) but never continued ones: WCONTINUED asks for nothing and
 * WIFCONTINUED is never true, so a caller's "child resumed" path does not
 * run (Neovim: a :terminal job's resume state). (Request R1.) */
#ifndef AMIGA_COMPAT_SYS_WAIT_H
#define AMIGA_COMPAT_SYS_WAIT_H
#pragma GCC system_header
#include_next <sys/wait.h>
#ifndef WCONTINUED
#define WCONTINUED	0
#endif
#ifndef WIFCONTINUED
#define WIFCONTINUED(status)	0
#endif
#endif
