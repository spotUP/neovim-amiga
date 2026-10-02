#ifndef NVIM_OS_PTY_PROCESS_UNIX_H
#define NVIM_OS_PTY_PROCESS_UNIX_H

#include <sys/ioctl.h>

#include "nvim/event/process.h"

typedef struct pty_process {
  Process process;
  char *term_name;
  uint16_t width, height;
  struct winsize winsize;
  int tty_fd;
#ifdef __amigaos__
  char tty_name[32];  ///< the slave's name (AmigaOS: no ptsname)
#endif
} PtyProcess;

static inline PtyProcess pty_process_init(Loop *loop, void *data)
{
  PtyProcess rv;
  rv.process = process_init(loop, kProcessTypePty, data);
  rv.term_name = NULL;
  rv.width = 80;
  rv.height = 24;
  rv.tty_fd = -1;
#ifdef __amigaos__
  rv.tty_name[0] = '\0';
#endif
  return rv;
}

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "os/pty_process_unix.h.generated.h"
#endif

#endif  // NVIM_OS_PTY_PROCESS_UNIX_H
