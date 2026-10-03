/* <dlfcn.h> for ixemul 48.2, which has no dynamic linker (an AmigaOS
 * library is not an object with a symbol table). Declarations only, and
 * no RTLD_DEFAULT: feature-detection code that probes optional functions
 * with dlsym(RTLD_DEFAULT, ...) (libuv's fs.c) compiles its fallback.
 * Nothing defines these functions; a caller fails at link time. libuv's
 * uv_dlopen on AmigaOS is its own (src/unix/dl.c). */
#ifndef AMIGA_COMPAT_DLFCN_H
#define AMIGA_COMPAT_DLFCN_H
#pragma GCC system_header
#define RTLD_LAZY	1
#define RTLD_NOW	2
#define RTLD_GLOBAL	0x100
#define RTLD_LOCAL	0
void	*dlopen(const char *, int);
void	*dlsym(void *, const char *);
int	 dlclose(void *);
char	*dlerror(void);
#endif
