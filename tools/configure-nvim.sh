#!/bin/sh
# Configure Neovim 0.4.4 for m68k-amigaos-ixemul in build/m68k/nvim, against
# the dependencies staged by `make sysroot`, with the host LuaJIT running
# the generators (tools/lua-host).
#   tools/configure-nvim.sh          m68k, into build/m68k/nvim
#   tools/configure-nvim.sh --host   this Mac, against `make host-deps`
#                                    (build/hostdeps/sysroot), into build/host/nvim
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
if [ "${1:-}" = "--host" ]; then
	shift
	SR=$ROOT/build/hostdeps/sysroot
	OUT=$ROOT/build/host/nvim
	# -include locale.h: 0.4.4's os/lang.c tests HAVE_LOCALE_H before it
	# includes config.h, which only its macOS branch trips over
	TARGET="-DCMAKE_C_FLAGS='-DUV_NO_THREADS -DUV_POSIX_POLL -include locale.h' -DNVIM_NO_THREADS=ON"
else
	SR=$ROOT/build/m68k/sysroot
	OUT=$ROOT/build/m68k/nvim
	TARGET="-DCMAKE_TOOLCHAIN_FILE=$ROOT/amiga/cmake/m68k-amigaos-ixemul.cmake -DCMAKE_FIND_ROOT_PATH=$SR"
fi
"$ROOT/tools/host-lua.sh" > /dev/null
# pkg-config would answer with the HOST's libraries (Homebrew libuv's
# -lpthread -lm): point it at the sysroot, which has no .pc files
export PKG_CONFIG_LIBDIR="$SR/lib/pkgconfig" PKG_CONFIG_PATH=
# shellcheck disable=SC2086
eval cmake -S "$ROOT/vendor/neovim" -B "$OUT" -G Ninja $TARGET \
  -DCMAKE_BUILD_TYPE=Release \
  -DPREFER_LUA=ON \
  -DLUA_PRG="$ROOT/tools/lua-host" \
  -DLUA_INCLUDE_DIR="$SR/include" -DLUA_LIBRARY="$SR/lib/liblua.a" \
  -DLIBUV_INCLUDE_DIR="$SR/include" -DLIBUV_LIBRARY="$SR/lib/libuv.a" \
  -DLIBLUV_INCLUDE_DIR="$SR/include" -DLIBLUV_LIBRARY="$SR/lib/libluv.a" \
  -DMSGPACK_INCLUDE_DIR="$SR/include" -DMSGPACK_LIBRARY="$SR/lib/libmsgpackc.a" \
  -DUNIBILIUM_INCLUDE_DIR="$SR/include" -DUNIBILIUM_LIBRARY="$SR/lib/libunibilium.a" \
  -DLIBTERMKEY_INCLUDE_DIR="$SR/include" -DLIBTERMKEY_LIBRARY="$SR/lib/libtermkey.a" \
  -DLIBVTERM_INCLUDE_DIR="$SR/include" -DLIBVTERM_LIBRARY="$SR/lib/libvterm.a" \
  -DENABLE_LIBINTL=OFF -DENABLE_LIBICONV=OFF -DENABLE_JEMALLOC=OFF \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_INSTALL_PREFIX=/usr/local "$@"
