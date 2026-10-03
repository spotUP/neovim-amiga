/* libuv platform functions for AmigaOS 3.x with ixemul.library.
 *
 * The event loop itself is libuv's UNIX core with posix-poll.c (poll() is
 * libixcompat's, over ixemul's select()); threads are nothreads.c. What is
 * here is what each platform file supplies: a monotonic clock, the
 * program's path, memory and CPU facts.
 *
 * The host test build (UV_NO_THREADS + UV_POSIX_POLL on a POSIX host)
 * compiles the #else branches: POSIX stand-ins with the same contracts, so
 * the rest of the backend can be tested off the Amiga. */

#include "uv.h"
#include "internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

/* libuv's uv_exepath contract (test-get-currentexe.c): a buffer too small
 * truncates; *size becomes the length written, without the NUL. */
static int uv__exepath_copy(const char* path, char* buffer, size_t* size) {
  size_t n;

  if (buffer == NULL || size == NULL || *size == 0)
    return UV_EINVAL;
  n = strlen(path);
  if (n > *size - 1)
    n = *size - 1;
  memcpy(buffer, path, n);
  buffer[n] = '\0';
  *size = n;
  return 0;
}

#if defined(__amigaos__)

#include "amiga-os.h"
#include <time.h>

/* Monotonic nanoseconds: libamigacompat's clock_gettime(CLOCK_MONOTONIC),
 * timer.device's E-clock (1.4 us). timer.device is in ROM; without it
 * libuv's timers could not run at all, so that aborts. */
uint64_t uv__hrtime(uv_clocktype_t type) {
  struct timespec ts;

  (void) type;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    abort();
  return (uint64_t) ts.tv_sec * 1000000000u + (uint64_t) ts.tv_nsec;
}


/* GetProgramDir + GetProgramName, as an ixemul path: "Work:bin/nvim"
 * becomes "/Work/bin/nvim", the form getcwd() and the rest of an ixemul
 * program use. */
int uv_exepath(char* buffer, size_t* size) {
  char path[1024];
  char unix_path[1026];
  char* colon;

  if (buffer == NULL || size == NULL || *size == 0)
    return UV_EINVAL;
  if (uv__amiga_program_path(path, sizeof(path)) != 0)
    return UV_ENOENT;

  colon = strchr(path, ':');
  if (colon != NULL) {
    /* "Vol:rest" -> "/Vol/rest" ("Vol:" -> "/Vol") */
    *colon = '\0';
    unix_path[0] = '/';
    strcpy(unix_path + 1, path);
    if (colon[1] != '\0') {
      strcat(unix_path, "/");
      strcat(unix_path, colon + 1);
    }
    return uv__exepath_copy(unix_path, buffer, size);
  }
  return uv__exepath_copy(path, buffer, size);
}


uint64_t uv_get_free_memory(void) {
  return (uint64_t) uv__amiga_avail_mem(0);
}


uint64_t uv_get_total_memory(void) {
  return (uint64_t) uv__amiga_avail_mem(1);
}


/* Seconds since the E-clock started, which is at boot. */
int uv_uptime(double* uptime) {
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    return UV__ERR(errno);
  *uptime = (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
  return 0;
}


static const char* uv__cpu_model(void) {
  return uv__amiga_cpu_model();
}

#else  /* the host test build */

#include <time.h>
#include <unistd.h>
#if defined(__APPLE__)
# include <mach-o/dyld.h>
# include <mach/mach.h>
#endif

uint64_t uv__hrtime(uv_clocktype_t type) {
  struct timespec ts;

  (void) type;
  if (clock_gettime(CLOCK_MONOTONIC, &ts))
    abort();
  return (uint64_t) ts.tv_sec * 1000000000u + (uint64_t) ts.tv_nsec;
}


int uv_exepath(char* buffer, size_t* size) {
  char path[4096];
#if defined(__APPLE__)
  uint32_t n = sizeof(path);

  if (_NSGetExecutablePath(path, &n) != 0)
    return UV_ENOBUFS;
#else
  ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);

  if (n < 0)
    return UV__ERR(errno);
  path[n] = '\0';
#endif
  return uv__exepath_copy(path, buffer, size);
}


uint64_t uv_get_free_memory(void) {
#if defined(__APPLE__)
  vm_statistics_data_t info;
  mach_msg_type_number_t count = sizeof(info) / sizeof(integer_t);

  if (host_statistics(mach_host_self(), HOST_VM_INFO,
                      (host_info_t) &info, &count) != KERN_SUCCESS)
    return 0;
  return (uint64_t) info.free_count * sysconf(_SC_PAGESIZE);
#else
  return (uint64_t) sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);
#endif
}


uint64_t uv_get_total_memory(void) {
  return (uint64_t) sysconf(_SC_PHYS_PAGES) * (uint64_t) sysconf(_SC_PAGESIZE);
}


int uv_uptime(double* uptime) {
  *uptime = (double) uv__hrtime(UV_CLOCK_PRECISE) / 1e9;
  return 0;
}


static const char* uv__cpu_model(void) {
  return "host";
}

#endif  /* __amigaos__ */


/* AmigaOS keeps no memory limit per process. */
uint64_t uv_get_constrained_memory(void) {
  return 0;
}


/* What a process may still allocate: all of it (no per-process limit). */
uint64_t uv_get_available_memory(void) {
  return uv_get_free_memory();
}


/* AmigaOS keeps no load average. */
void uv_loadavg(double avg[3]) {
  avg[0] = 0;
  avg[1] = 0;
  avg[2] = 0;
}


/* exec.library does not account a task's memory. */
int uv_resident_set_memory(size_t* rss) {
  (void) rss;
  return UV_ENOSYS;
}


/* One CPU (the model from AttnFlags); AmigaOS keeps neither its clock
 * rate nor per-mode CPU times, so speed and times are 0. */
int uv_cpu_info(uv_cpu_info_t** cpu_infos, int* count) {
  uv_cpu_info_t* ci;

  ci = uv__calloc(1, sizeof(*ci));
  if (ci == NULL)
    return UV_ENOMEM;
  ci->model = uv__strdup(uv__cpu_model());
  if (ci->model == NULL) {
    uv__free(ci);
    return UV_ENOMEM;
  }
  ci->speed = 0;
  *cpu_infos = ci;
  *count = 1;
  return 0;
}




/* bsdsocket.library's interface list is not wired up (Neovim does not ask
 * for it): ENOSYS, not an empty list that would read as "no network". */
int uv_interface_addresses(uv_interface_address_t** addresses, int* count) {
  *addresses = NULL;
  *count = 0;
  return UV_ENOSYS;
}

/* uv_free_cpu_info and uv_free_interface_addresses are uv-common.c's. */
