/* ixemul 48.2's <fcntl.h>. It has no O_NOFOLLOW; 0 here, so a program
 * that asks not to follow a symbolic link opens through it (AmigaOS soft
 * links are rare, and ixemul's open() cannot refuse them). Listed in the
 * ledger's "not implemented". (Request R1: a real O_NOFOLLOW.) */
#ifndef AMIGA_COMPAT_FCNTL_H
#define AMIGA_COMPAT_FCNTL_H
#pragma GCC system_header
#include_next <fcntl.h>
#ifndef O_NOFOLLOW
#define O_NOFOLLOW	0
#endif
#endif
