/* The AmigaOS calls libuv's AmigaOS platform needs, kept in amiga-os.c so
 * the NDK's headers (devices/timer.h has its own struct timeval) never meet
 * ixemul's POSIX headers in one translation unit. */
#ifndef UV_AMIGA_OS_H
#define UV_AMIGA_OS_H

/* E-clock ticks since boot and the E-clock rate in Hz; -1 without
   timer.device */
int uv__amiga_eclock(unsigned long long* ticks, unsigned long* hz);

/* the running program's file, AmigaOS form ("Work:bin/nvim"); 0 or -1 */
int uv__amiga_program_path(char* buf, int size);

/* AvailMem(MEMF_ANY), or MEMF_ANY|MEMF_TOTAL when total */
unsigned long uv__amiga_avail_mem(int total);

/* "MC68020" and so on, from SysBase->AttnFlags */
const char* uv__amiga_cpu_model(void);

/* see atomic-ops.h */
int uv__amiga_cmpxchgi(int* ptr, int oldval, int newval);

#endif
