#!/bin/sh
# Configure Neovim 0.12.5 (vendor012/neovim).
#   tools/configure-nvim012.sh --host   this Mac, on `make -f Makefile.v012 host-deps`
#                                       + B=build/v012/hostdeps sysroot -> build/v012/host/nvim
#   tools/configure-nvim012.sh          m68k (needs the host build's nlua0) -> build/v012/m68k/nvim
# PUC Lua 5.1 (PREFER_LUA); generators run by build/v012/host/lua51 (arm64,
# PUC 5.1, loads the host-built nlua0 module).
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
LUA51=$ROOT/build/v012/host/lua51
if [ "${1:-}" = "--host" ]; then
	shift
	SR=$ROOT/build/v012/hostdeps/sysroot
	OUT=$ROOT/build/v012/host/nvim
	# the Amiga's static parsers and uv_dlopen (UV_STATIC_DL), same list
	TS="vimdoc lua query markdown markdown_inline"
	DEFS=""; LIBS=""
	for p in $TS; do
		DEFS="$DEFS -DNVIM_STATIC_TS_$(echo $p | tr a-z A-Z)"
		LIBS="$LIBS -ltsparser_$p"
	done
	set -- -DCMAKE_C_FLAGS="-DUV_NO_THREADS -DUV_POSIX_POLL -DUV_STATIC_DL$DEFS" \
	  -DCMAKE_EXE_LINKER_FLAGS="-L$SR/lib$LIBS" "$@"
else
	SR=$ROOT/build/v012/m68k/sysroot
	OUT=$ROOT/build/v012/m68k/nvim
	NLUA0=$(ls "$ROOT"/build/v012/host/nvim/lib/libnlua0* 2>/dev/null | head -1)
	[ -n "$NLUA0" ] || { echo "build the host nvim first (its nlua0)"; exit 1; }
	set -- -DCMAKE_TOOLCHAIN_FILE="$ROOT/amiga/cmake/m68k-amigaos-ixemul012.cmake" \
	  -DCMAKE_FIND_ROOT_PATH="$SR" -DNLUA0_HOST_PRG="$NLUA0" -DCOMPILE_LUA=OFF \
	  -DICONV_INCLUDE_DIR="$ROOT/amiga/compat/include" "$@"
fi
export PKG_CONFIG_LIBDIR="$SR/lib/pkgconfig" PKG_CONFIG_PATH=
cmake -S "$ROOT/vendor012/neovim" -B "$OUT" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DPREFER_LUA=ON -DLUA_PRG="$LUA51" -DLUA_GEN_PRG="$LUA51" \
  -DLUA_INCLUDE_DIR="$SR/include" -DLUA_LIBRARY="$SR/lib/liblua.a" \
  -DLIBUV_INCLUDE_DIR="$SR/include" -DLIBUV_LIBRARY="$SR/lib/libuv.a" \
  -DLUV_INCLUDE_DIR="$SR/include" -DLUV_LIBRARY="$SR/lib/libluv.a" \
  -DLPEG_LIBRARY="$SR/lib/liblpeg.a" \
  -DUNIBILIUM_INCLUDE_DIR="$SR/include" -DUNIBILIUM_LIBRARY="$SR/lib/libunibilium.a" \
  -DUTF8PROC_INCLUDE_DIR="$SR/include" -DUTF8PROC_LIBRARY="$SR/lib/libutf8proc.a" \
  -DTREESITTER_INCLUDE_DIR="$SR/include" -DTREESITTER_LIBRARY="$SR/lib/libtree-sitter.a" \
  -DENABLE_LIBINTL=OFF -DENABLE_WASMTIME=OFF \
  -DCMAKE_INSTALL_PREFIX=/usr/local "$@"
