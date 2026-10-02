/* See amiga-os.h. Only NDK headers here. */
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <string.h>
#include <stdlib.h>

#include "amiga-os.h"

extern struct ExecBase* SysBase;
struct Device* TimerBase;

static struct timerequest uv__amiga_tr;
static int uv__amiga_tr_state;  /* 0 not tried, 1 open, -1 failed */

static void uv__amiga_timer_close(void) {
  if (uv__amiga_tr_state == 1) {
    CloseDevice((struct IORequest*) &uv__amiga_tr);
    uv__amiga_tr_state = 0;
    TimerBase = NULL;
  }
}

/* timer.device is opened once, for ReadEClock; no I/O is ever sent. */
int uv__amiga_eclock(unsigned long long* ticks, unsigned long* hz) {
  struct EClockVal ev;

  if (uv__amiga_tr_state == 0) {
    memset(&uv__amiga_tr, 0, sizeof(uv__amiga_tr));
    if (OpenDevice((CONST_STRPTR) TIMERNAME, UNIT_ECLOCK,
                   (struct IORequest*) &uv__amiga_tr, 0) == 0) {
      TimerBase = uv__amiga_tr.tr_node.io_Device;
      uv__amiga_tr_state = 1;
      atexit(uv__amiga_timer_close);
    } else {
      uv__amiga_tr_state = -1;
    }
  }
  if (uv__amiga_tr_state != 1)
    return -1;
  *hz = ReadEClock(&ev);
  *ticks = ((unsigned long long) ev.ev_hi << 32) | ev.ev_lo;
  return 0;
}

int uv__amiga_program_path(char* buf, int size) {
  char name[256];
  BPTR dir;

  dir = GetProgramDir();
  if (dir == 0 || !NameFromLock(dir, (STRPTR) buf, size))
    return -1;
  if (!GetProgramName((STRPTR) name, sizeof(name)))
    return -1;
  if (!AddPart((STRPTR) buf, FilePart((STRPTR) name), size))
    return -1;
  return 0;
}

unsigned long uv__amiga_avail_mem(int total) {
  return AvailMem(total ? (MEMF_ANY | MEMF_TOTAL) : MEMF_ANY);
}

const char* uv__amiga_cpu_model(void) {
  UWORD f = SysBase->AttnFlags;

  if (f & 0x80)  /* AFF_68060, which older NDKs do not name */
    return "MC68060";
  if (f & AFF_68040)
    return "MC68040";
  if (f & AFF_68030)
    return "MC68030";
  if (f & AFF_68020)
    return "MC68020";
  if (f & AFF_68010)
    return "MC68010";
  return "MC68000";
}

int uv__amiga_cmpxchgi(int* ptr, int oldval, int newval) {
  int out;

  Disable();
  out = *(volatile int*) ptr;
  if (out == oldval)
    *(volatile int*) ptr = newval;
  Enable();
  return out;
}
