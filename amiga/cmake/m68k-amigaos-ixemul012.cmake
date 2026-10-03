# CMake toolchain for Neovim 0.12.5 (vendor012; the 0.4.4 one is
# m68k-amigaos-ixemul.cmake): bebbo's gcc for AmigaOS 3.x, ixemul 48.2, 68020.
# CMAKE_SYSTEM_NAME is Generic (CMake has no AmigaOS platform), so CMake's
# UNIX is false and Neovim adds neither -lm nor -lutil (newlib's, which an
# ixemul program must not link); the C code gets -DUNIX from the flags.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR m68k)

set(AMIGA_PREFIX "$ENV{HOME}/opt/amiga" CACHE PATH "bebbo's toolchain")
set(CMAKE_C_COMPILER "${AMIGA_PREFIX}/bin/m68k-amigaos-gcc")
set(CMAKE_AR "${AMIGA_PREFIX}/bin/m68k-amigaos-ar" CACHE FILEPATH "")
set(CMAKE_RANLIB "${AMIGA_PREFIX}/bin/m68k-amigaos-ranlib" CACHE FILEPATH "")

get_filename_component(NA_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
# Tree-sitter parsers linked in (src/nvim/os/static_dl.c serves them to
# uv_dlopen): the ones the bundled ftplugins start. c and vim (1.6 MB of
# tables between them) on request: -DAMIGA_TS_PARSERS="vimdoc;...;c;vim".
set(AMIGA_TS_PARSERS "vimdoc;lua;query;markdown;markdown_inline" CACHE STRING
    "tree-sitter parsers linked into nvim")
set(_ts_defs "")
set(_ts_libs "")
foreach(_p ${AMIGA_TS_PARSERS})
  string(TOUPPER "${_p}" _P)
  string(APPEND _ts_defs " -DNVIM_STATIC_TS_${_P}")
  string(APPEND _ts_libs " -ltsparser_${_p}")
endforeach()

set(CMAKE_C_FLAGS_INIT
  "-mcrt=ixemul -m68020 -fno-strict-aliasing -DUNIX -I${NA_ROOT}/amiga/compat/include${_ts_defs}")

# Link without the C flags: -m68020 at link time makes the driver pick
# libnix's crt0 (the ixemul SDK has no libm020 multilib); the objects are
# 68020 code either way. Every link (try_compile checks too) ends with
# libamigacompat (what ixemul lacks, __eprintf for assert) and libixcompat.
set(CMAKE_C_LINK_EXECUTABLE
  "<CMAKE_C_COMPILER> -mcrt=ixemul <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES> -L${NA_ROOT}/build/v012/m68k/sysroot/lib${_ts_libs} -lamigawide -lutf8proc -L${NA_ROOT}/build/m68k -lamigacompat -lixcompat")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


# pkg-config would answer with the HOST's libraries (Homebrew libuv's
# -lpthread -lm, i.e. newlib's libm, which clashes with ixemul's libc).
# Here, not in the configure script: CMake re-reads the toolchain file when
# it regenerates on its own after a CMakeLists.txt change.
set(ENV{PKG_CONFIG_LIBDIR} "${NA_ROOT}/build/v012/m68k/sysroot/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")

# No stack protector: gcc accepts -fstack-protector-strong (so Neovim's flag
# check passes) but m68k-amigaos has no libssp (__stack_chk_guard/_fail),
# and the guard costs every protected call on a 68020. The answers to
# Neovim's checks, preset:
set(HAS_FSTACK_PROTECTOR_STRONG_FLAG OFF CACHE BOOL "no libssp on m68k-amigaos")
set(HAS_FSTACK_PROTECTOR_FLAG OFF CACHE BOOL "no libssp on m68k-amigaos")
