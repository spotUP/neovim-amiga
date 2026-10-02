#ifndef NVIM_TUI_INPUT_H
#define NVIM_TUI_INPUT_H

#include <stdbool.h>

#include <termkey.h>
#include "nvim/event/stream.h"
#include "nvim/event/time.h"

typedef struct term_input {
  int in_fd;
  // Phases: -1=all 0=disabled 1=first-chunk 2=continue 3=last-chunk
  int8_t paste;
  bool waiting;
  TermKey *tk;
#if TERMKEY_VERSION_MAJOR > 0 || TERMKEY_VERSION_MINOR > 18
  TermKey_Terminfo_Getstr_Hook *tk_ti_hook_fn;  ///< libtermkey terminfo hook
#endif
  TimeWatcher timer_handle;
  Loop *loop;
  Stream read_stream;
  RBuffer *key_buffer;
  uv_mutex_t key_buffer_mutex;
  uv_cond_t key_buffer_cond;
#ifdef UV_NO_THREADS
  bool retry_pending;  ///< tinput_retry_event is scheduled
  bool read_stopped;   ///< reading paused until the key buffer drains
#endif
} TermInput;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "tui/input.h.generated.h"
#endif

#endif  // NVIM_TUI_INPUT_H
