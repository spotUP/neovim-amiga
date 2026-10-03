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

/* The running program's file, as an AmigaDOS path. The program name
 * comes first when it holds a path (":" or "/"): it is the file that was
 * run, and a launcher that sets the name but not the program directory
 * leaves GetProgramDir() at its own (vsh's runner: RunCommand after
 * SetProgramName(argv[0]), no SetProgramDir; the AmigaOS Shell sets both,
 * and then both answers agree). A bare name is looked up in the program
 * directory, and only a file that exists there counts. */
int uv__amiga_program_path(char* buf, int size) {
  char name[256];
  BPTR dir;
  BPTR lock;
  BPTR old;
  int ok;

  if (!GetProgramName((STRPTR) name, sizeof(name) - 1) || name[0] == '\0')
    return -1;
  if (name[0] == '/' && strchr(name, ':') == NULL) {
    /* an ixemul path as typed in vsh, "/Vol/rest": AmigaDOS "Vol:rest" */
    char* sep = strchr(name + 1, '/');
    memmove(name, name + 1, strlen(name));
    if (sep != NULL)
      sep[-1] = ':';
    else
      strcat(name, ":");
  }
  if (strchr(name, ':') != NULL || strchr(name, '/') != NULL) {
    lock = Lock((STRPTR) name, SHARED_LOCK);
    if (lock == 0)
      return -1;
    ok = NameFromLock(lock, (STRPTR) buf, size);
    UnLock(lock);
    return ok ? 0 : -1;
  }
  dir = GetProgramDir();
  if (dir == 0)
    return -1;
  old = CurrentDir(dir);
  lock = Lock((STRPTR) name, SHARED_LOCK);
  CurrentDir(old);
  if (lock == 0)
    return -1;
  ok = NameFromLock(lock, (STRPTR) buf, size);
  UnLock(lock);
  return ok ? 0 : -1;
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
