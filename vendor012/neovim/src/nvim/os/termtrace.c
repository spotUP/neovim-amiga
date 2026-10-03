// See termtrace.h.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "nvim/os/termtrace.h"

static int state;  // 0 not looked, 1 on, -1 off
static FILE *out;
static uint64_t t0;

int termtrace_on(void)
{
  if (state == 0) {
    const char *path = getenv("NVIM_TERMTRACE");
    state = -1;
    if (path != NULL && *path != '\0' && (out = fopen(path, "a")) != NULL) {
      setvbuf(out, NULL, _IOLBF, 0);
      t0 = uv_hrtime();
      state = 1;
    }
  }
  return state == 1;
}

static void stamp(void)
{
  uint64_t us = (uv_hrtime() - t0) / 1000;
  fprintf(out, "%5lu.%03lu ", (unsigned long)(us / 1000), (unsigned long)(us % 1000));
}

void termtrace(const char *fmt, ...)
{
  if (!termtrace_on()) {
    return;
  }
  va_list ap;
  va_start(ap, fmt);
  stamp();
  vfprintf(out, fmt, ap);
  va_end(ap);
  fputc('\n', out);
}

void termtrace_bytes(const char *what, const char *data, size_t len)
{
  if (!termtrace_on()) {
    return;
  }
  stamp();
  fprintf(out, "%s (%lu bytes) ", what, (unsigned long)len);
  for (size_t i = 0; i < len && i < 48; i++) {
    unsigned char c = (unsigned char)data[i];
    if (c == 0x1b) {
      fputs("\\e", out);
    } else if (c < 0x20 || c >= 0x7f) {
      fprintf(out, "\\x%02x", c);
    } else {
      fputc(c, out);
    }
  }
  fputc('\n', out);
}
