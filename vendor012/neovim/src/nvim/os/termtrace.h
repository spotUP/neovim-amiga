#pragma once

#include <stddef.h>

/// Timestamped trace of the terminal round trips at startup (the 'background'
/// OSC 11 + DSR query and its answer), the UI flushes they ride on, vim.wait,
/// and the exit paths -- the steps between nvim_ui_send and TermResponse.
/// Off unless $NVIM_TERMTRACE names a file (appended to; one line per step,
/// milliseconds since the first line). Added by neovim-amiga to measure
/// where E1568's 100 ms go on a 68020.
void termtrace(const char *fmt, ...)
  __attribute__((format(printf, 1, 2)));

/// termtrace() with `len` bytes of `data` appended, control bytes escaped.
void termtrace_bytes(const char *what, const char *data, size_t len);

/// Whether tracing is on (to skip building a message).
int termtrace_on(void);
