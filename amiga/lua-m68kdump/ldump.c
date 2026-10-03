/*
** ldump.c for a HOST Lua that writes bytecode for the Amiga's Lua 5.1
** (neovim-amiga, ledger Q-D3): big-endian, int 4 bytes, size_t 4 bytes,
** Instruction 4 bytes, lua_Number an 8-byte IEEE double -- what PUC Lua
** 5.1.5 built by m68k-amigaos-gcc expects (SIZEOF_INT 4, SIZEOF_SIZE_T 4
** in its config). Values are written byte by byte, so the host's own
** endianness and sizes do not matter. Derived from vendor/lua/src/ldump.c.
*/
/*
** $Id: ldump.c,v 2.8.1.1 2007/12/27 13:02:25 roberto Exp $
** save precompiled Lua chunks
** See Copyright Notice in lua.h
*/

#include <stddef.h>
#include <string.h>

#define ldump_c
#define LUA_CORE

#include "lua.h"

#include "lobject.h"
#include "lstate.h"
#include "lundump.h"

typedef struct {
 lua_State* L;
 lua_Writer writer;
 void* data;
 int strip;
 int status;
} DumpState;

#define DumpMem(b,n,size,D)	DumpBlock(b,(n)*(size),D)
#define DumpVar(x,D)	 	DumpMem(&x,1,sizeof(x),D)

static void DumpBlock(const void* b, size_t size, DumpState* D)
{
 if (D->status==0)
 {
  lua_unlock(D->L);
  D->status=(*D->writer)(D->L,b,size,D->data);
  lua_lock(D->L);
 }
}

static void DumpBE(unsigned long long v, int n, DumpState* D)
{
 unsigned char b[8];
 int i;
 for (i=n-1; i>=0; i--) { b[i]=(unsigned char)(v&0xff); v>>=8; }
 DumpBlock(b,(size_t)n,D);
}

static void DumpChar(int y, DumpState* D)
{
 char x=(char)y;
 DumpVar(x,D);
}

static void DumpInt(int x, DumpState* D)
{
 DumpBE((unsigned long long)(unsigned int)x,4,D);
}

static void DumpSize(size_t x, DumpState* D)
{
 DumpBE((unsigned long long)x,4,D);
}

static void DumpNumber(lua_Number x, DumpState* D)
{
 double d=(double)x;
 unsigned long long u;
 memcpy(&u,&d,8);			/* IEEE 754 bits, host order */
 DumpBE(u,8,D);
}

/* vectors of 4-byte items (Instruction, int) */
static void DumpVector(const void* b, int n, size_t size, DumpState* D)
{
 int i;
 DumpInt(n,D);
 for (i=0; i<n; i++)
  DumpBE((unsigned long long)((const unsigned int*)b)[i],4,D);
 (void)size;
}

static void DumpString(const TString* s, DumpState* D)
{
 if (s==NULL || getstr(s)==NULL)
 {
  DumpSize(0,D);
 }
 else
 {
  size_t size=s->tsv.len+1;		/* include trailing '\0' */
  DumpSize(size,D);
  DumpBlock(getstr(s),size,D);
 }
}

#define DumpCode(f,D)	 DumpVector(f->code,f->sizecode,sizeof(Instruction),D)

static void DumpFunction(const Proto* f, const TString* p, DumpState* D);

static void DumpConstants(const Proto* f, DumpState* D)
{
 int i,n=f->sizek;
 DumpInt(n,D);
 for (i=0; i<n; i++)
 {
  const TValue* o=&f->k[i];
  DumpChar(ttype(o),D);
  switch (ttype(o))
  {
   case LUA_TNIL:
	break;
   case LUA_TBOOLEAN:
	DumpChar(bvalue(o),D);
	break;
   case LUA_TNUMBER:
	DumpNumber(nvalue(o),D);
	break;
   case LUA_TSTRING:
	DumpString(rawtsvalue(o),D);
	break;
   default:
	lua_assert(0);			/* cannot happen */
	break;
  }
 }
 n=f->sizep;
 DumpInt(n,D);
 for (i=0; i<n; i++) DumpFunction(f->p[i],f->source,D);
}

static void DumpDebug(const Proto* f, DumpState* D)
{
 int i,n;
 n= (D->strip) ? 0 : f->sizelineinfo;
 DumpVector(f->lineinfo,n,sizeof(int),D);
 n= (D->strip) ? 0 : f->sizelocvars;
 DumpInt(n,D);
 for (i=0; i<n; i++)
 {
  DumpString(f->locvars[i].varname,D);
  DumpInt(f->locvars[i].startpc,D);
  DumpInt(f->locvars[i].endpc,D);
 }
 n= (D->strip) ? 0 : f->sizeupvalues;
 DumpInt(n,D);
 for (i=0; i<n; i++) DumpString(f->upvalues[i],D);
}

static void DumpFunction(const Proto* f, const TString* p, DumpState* D)
{
 DumpString((f->source==p || D->strip) ? NULL : f->source,D);
 DumpInt(f->linedefined,D);
 DumpInt(f->lastlinedefined,D);
 DumpChar(f->nups,D);
 DumpChar(f->numparams,D);
 DumpChar(f->is_vararg,D);
 DumpChar(f->maxstacksize,D);
 DumpCode(f,D);
 DumpConstants(f,D);
 DumpDebug(f,D);
}

static void DumpHeader(DumpState* D)
{
 char h[LUAC_HEADERSIZE];
 luaU_header(h);
 DumpBlock(h,LUAC_HEADERSIZE,D);
}

/*
** dump Lua function as precompiled chunk
*/
int luaU_dump (lua_State* L, const Proto* f, lua_Writer w, void* data, int strip)
{
 DumpState D;
 D.L=L;
 D.writer=w;
 D.data=data;
 D.strip=strip;
 D.status=0;
 DumpHeader(&D);
 DumpFunction(f,NULL,&D);
 return D.status;
}
