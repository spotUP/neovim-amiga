/* See amiga-os.h. Only NDK headers here. */
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdlib.h>

#include "amiga-os.h"

extern struct ExecBase* SysBase;

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
