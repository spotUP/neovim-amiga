# CMake toolchain: bebbo's gcc for AmigaOS 3.x, ixemul 48.2, 68020.
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
set(CMAKE_C_FLAGS_INIT
  "-mcrt=ixemul -m68020 -fno-strict-aliasing -DUNIX -I${NA_ROOT}/amiga/compat/include")

# Link without the C flags: -m68020 at link time makes the driver pick
# libnix's crt0 (the ixemul SDK has no libm020 multilib); the objects are
# 68020 code either way. Every link (try_compile checks too) ends with
# libamigacompat (what ixemul lacks, __eprintf for assert) and libixcompat.
set(CMAKE_C_LINK_EXECUTABLE
  "<CMAKE_C_COMPILER> -mcrt=ixemul <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES> -L${NA_ROOT}/build/m68k/sysroot/lib -lamigacompat -lixcompat")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# no thread library: see NVIM_NO_THREADS in vendor/neovim/CMakeLists.txt
set(NVIM_NO_THREADS ON)
