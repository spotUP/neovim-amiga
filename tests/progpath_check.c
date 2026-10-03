/* Host check of libuv's AmigaOS program path (vendor012/libuv/src/unix/
 * amiga-os.c, uv__amiga_program_path), with dos.library stubbed over a
 * table of files. The 0.12.5 rig found $VIMRUNTIME wrong when nvim was
 * started from vsh: vsh's runner sets the program name (argv[0]) and
 * leaves the program directory at vsh's, and the old code joined that
 * directory with the name's file part ("VTCX:nvim").
 *   cc -Itests/progpath_stubs -Ivendor012/libuv/src/unix \
 *      tests/progpath_check.c -o build/v012/host/progpath_check */
#include <stdio.h>
#include <string.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/dos.h>

struct ExecBase* SysBase;
ULONG AvailMem(ULONG f) { (void) f; return 0; }

static const char* files[] = {
  "VTCX:", "VTCX:vsh", "VTCX:nvim012", "VTCX:nvim012/nvim/bin", "VTCX:nvim012/nvim/bin/nvim",
};
#define NFILES (sizeof(files) / sizeof(files[0]))
static BPTR cur;               /* current dir lock */
static const char* prog_name;  /* GetProgramName */
static BPTR prog_dir;          /* GetProgramDir */

static BPTR lock_of(const char* full) {
  for (unsigned i = 0; i < NFILES; i++)
    if (strcmp(files[i], full) == 0)
      return (BPTR) i + 1;
  return 0;
}
BPTR Lock(STRPTR name, LONG mode) {
  char full[300];
  (void) mode;
  if (name[0] == '/')
    return 0;  /* AmigaDOS: "/" is the parent; nothing here has one */
  if (strncmp(name, "VTC:", 4) == 0)
    snprintf(full, sizeof full, "VTCX:%s", name + 4);
  else if (strchr(name, ':'))
    snprintf(full, sizeof full, "%s", name);
  else {
    const char* d = cur ? files[cur - 1] : "VTCX:";
    snprintf(full, sizeof full, "%s%s%s", d, d[strlen(d) - 1] == ':' ? "" : "/", name);
  }
  return lock_of(full);
}
void UnLock(BPTR l) { (void) l; }
int NameFromLock(BPTR l, STRPTR buf, LONG n) {
  if (l <= 0 || (unsigned long) l > NFILES || (LONG) strlen(files[l - 1]) >= n)
    return 0;
  strcpy(buf, files[l - 1]);
  return 1;
}
BPTR CurrentDir(BPTR l) { BPTR o = cur; cur = l; return o; }
int GetProgramName(STRPTR buf, LONG n) {
  if ((LONG) strlen(prog_name) >= n)
    return 0;
  strcpy(buf, prog_name);
  return 1;
}
BPTR GetProgramDir(void) { return prog_dir; }

#include "amiga-os.c"

static int failed, total;
static void expect(const char* what, const char* name, const char* dir, const char* want) {
  char buf[256];
  int rc;
  prog_name = name;
  prog_dir = lock_of(dir);
  cur = lock_of("VTCX:");
  rc = uv__amiga_program_path(buf, sizeof buf);
  total++;
  if ((want == NULL && rc == 0) || (want != NULL && (rc != 0 || strcmp(buf, want) != 0))) {
    failed++;
    printf("FAIL %s: name %s, program dir %s: got %s, want %s\n", what, name, dir,
           rc == 0 ? buf : "(failed)", want ? want : "(failed)");
  } else {
    printf("ok   %s\n", what);
  }
}

int main(void) {
  /* vsh: SetProgramName(argv[0]) + RunCommand, program dir left at vsh's */
  expect("vsh, AmigaDOS path typed", "VTC:nvim012/nvim/bin/nvim", "VTCX:",
         "VTCX:nvim012/nvim/bin/nvim");
  expect("vsh, ixemul path typed", "/VTC/nvim012/nvim/bin/nvim", "VTCX:",
         "VTCX:nvim012/nvim/bin/nvim");
  expect("vsh, relative path typed", "nvim012/nvim/bin/nvim", "VTCX:",
         "VTCX:nvim012/nvim/bin/nvim");
  /* the AmigaOS Shell: program dir and a bare name (found on the path) */
  expect("Shell, bare name", "nvim", "VTCX:nvim012/nvim/bin", "VTCX:nvim012/nvim/bin/nvim");
  /* a bare name whose program dir does not hold it: no answer, so nvim
   * falls back to argv[0] and $PATH (path_guess_exepath) */
  expect("bare name, wrong program dir", "nvim", "VTCX:", NULL);
  expect("no program name", "", "VTCX:", NULL);
  printf("PASS: %d failed of %d\n", failed, total);
  return failed != 0;
}
