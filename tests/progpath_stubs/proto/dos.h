#ifndef STUB_PROTO_DOS_H
#define STUB_PROTO_DOS_H
#include <exec/types.h>
int GetProgramName(STRPTR, LONG);
BPTR GetProgramDir(void);
BPTR CurrentDir(BPTR);
BPTR Lock(STRPTR, LONG);
void UnLock(BPTR);
int NameFromLock(BPTR, STRPTR, LONG);
#endif
