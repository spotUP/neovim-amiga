#!/bin/sh
# The host Lua that runs Neovim's build-time generators: the host LuaJIT
# (x86_64, Rosetta on Apple silicon; it has `bit` built in) plus lpeg 1.0.2
# and libmpack-lua 1.0.7 built from vendor/ as x86_64 bundles into
# build/host/lua/. Prints the LUA_CPATH to use; exits non-zero if a module
# does not load.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
LUAJIT=${LUAJIT:-/usr/local/bin/luajit}
LJINC=${LJINC:-/usr/local/include/luajit-2.1}
OUT=$ROOT/build/host/lua
mkdir -p "$OUT" "$ROOT/build/host/mpsrc/mpack-src"
if [ ! -f "$OUT/lpeg.so" ]; then
	clang -arch x86_64 -O2 -w -bundle -undefined dynamic_lookup -I"$LJINC" \
	    -o "$OUT/lpeg.so" "$ROOT"/vendor/lpeg/*.c
fi
if [ ! -f "$OUT/mpack.so" ]; then
	rm -rf "$ROOT/build/host/mpsrc/mpack-src/src"
	cp -R "$ROOT/vendor/libmpack/src" "$ROOT/build/host/mpsrc/mpack-src/"
	clang -arch x86_64 -O2 -w -bundle -undefined dynamic_lookup -I"$LJINC" \
	    -I"$ROOT/build/host/mpsrc" -o "$OUT/mpack.so" \
	    "$ROOT/vendor/libmpack-lua/lmpack.c"
fi
LUA_CPATH="$OUT/?.so" "$LUAJIT" -e "assert(require'lpeg'); assert(require'mpack'.Packer); assert(bit.band)"
echo "LUA_CPATH=$OUT/?.so"
