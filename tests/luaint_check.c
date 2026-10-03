/* Compile-only check (m68k-amigaos-gcc, Makefile.v012 `checks`): Lua's
 * lua_Integer must hold 64 bits on the Amiga. luv pushes uv_hrtime() (ns)
 * and 64-bit file sizes through lua_pushinteger; with 5.1's default
 * ptrdiff_t (32 bits on m68k) vim.uv.hrtime() wrapped every 4.3 s and was
 * negative half the time (0.12.5 rig, run 3). */
#include <lua.h>

_Static_assert(sizeof(lua_Integer) == 8, "lua_Integer is not 64 bits");

/* what luv_hrtime does: an E-clock time after an hour of uptime survives */
static lua_Integer pushed(unsigned long long ns) { return (lua_Integer) ns; }
int luaint_check(void) { return pushed(3600ULL * 1000000000ULL) > 0; }
