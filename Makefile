# Neovim 0.4.4 for AmigaOS 3.x / 68020 / ixemul 48.2 (UP-Term plan Q1).
# Cross build with bebbo's gcc; the ledger is
# thoughts/shared/plans/2026-10-03-neovim-port.md.
#
#   make            every m68k library (build/m68k/lib*.a)
#   make host-test  libuv's backend built for this Mac (no threads,
#                   posix-poll, the synchronous work queue) + its tests
AMIGA   ?= $(HOME)/opt/amiga
AGCC    ?= $(AMIGA)/bin/m68k-amigaos-gcc
AAR     ?= $(AMIGA)/bin/m68k-amigaos-ar
ACFLAGS ?= -mcrt=ixemul -m68020 -O2 -fno-strict-aliasing -Wall -Wno-unused \
           -Wno-pointer-sign
# link without -m68020: the ixemul SDK has no libm020 multilib, and with
# the flag the driver picks libnix's crt0 (68020 objects + 68000 libc is fine)
ALDFLAGS ?= -mcrt=ixemul
HOSTCC  ?= cc
B       = build/m68k
H       = build/host

.PHONY: all libs host-test clean
all: libs
libs: $(B)/libamigacompat.a $(B)/libuv.a $(B)/liblua.a $(B)/lua51 $(B)/libmsgpackc.a \
      $(B)/libunibilium.a $(B)/libtermkey.a $(B)/libvterm.a $(B)/libluv.a

# ---- what ixemul 48.2 lacks: IPv6 types, getaddrinfo (ledger R1) ----------
COMPAT_INC = -Iamiga/compat/include
COMPAT_HDRS = $(wildcard amiga/compat/include/*.h amiga/compat/include/*/*.h)
COMPAT_OBJS = $(B)/compat/netdb.o $(B)/compat/posix.o $(B)/compat/eprintf.o \
              $(B)/compat/math.o

$(B)/compat/%.o: amiga/compat/%.c $(COMPAT_HDRS)
	@mkdir -p $(dir $@)
	$(AGCC) $(ACFLAGS) $(COMPAT_INC) -c -o $@ $<

$(B)/libamigacompat.a: $(COMPAT_OBJS)
	rm -f $@
	$(AAR) rcs $@ $(COMPAT_OBJS)

# ---- libuv 1.30.0: the UNIX core, posix-poll, no threads -----------------
UV      = vendor/libuv
UV_COMMON = fs-poll.c idna.c inet.c strscpy.c threadpool.c timer.c \
            uv-common.c uv-data-getter-setters.c version.c
UV_UNIX = async.c core.c dl.c fs.c getaddrinfo.c getnameinfo.c \
          loop-watcher.c loop.c pipe.c poll.c process.c signal.c stream.c \
          tcp.c tty.c udp.c posix-poll.c no-fsevents.c no-proctitle.c \
          nothreads.c amiga.c amiga-os.c
UV_SRCS = $(addprefix src/,$(UV_COMMON)) $(addprefix src/unix/,$(UV_UNIX))
UV_INC  = $(COMPAT_INC) -I$(UV)/include -I$(UV)/src
UV_OBJS = $(addprefix $(B)/libuv/,$(UV_SRCS:.c=.o))
UV_HDRS = $(wildcard $(UV)/include/*.h $(UV)/include/uv/*.h $(UV)/src/*.h \
            $(UV)/src/unix/*.h)

$(B)/libuv/%.o: $(UV)/%.c $(UV_HDRS) $(COMPAT_HDRS)
	@mkdir -p $(dir $@)
	$(AGCC) $(ACFLAGS) $(UV_INC) -c -o $@ $<

$(B)/libuv.a: $(UV_OBJS)
	rm -f $@
	$(AAR) rcs $@ $(UV_OBJS)

clean:
	rm -rf build/m68k build/host/libuv

# ---- uvsmoke: the libuv features Neovim uses, self-checking ---------------
$(B)/uvsmoke: tests/uvsmoke.c $(B)/libuv.a $(B)/libamigacompat.a
	$(AGCC) $(ACFLAGS) $(COMPAT_INC) -I$(UV)/include -c -o $(B)/uvsmoke.o tests/uvsmoke.c
	$(AGCC) $(ALDFLAGS) -o $@ $(B)/uvsmoke.o $(B)/libuv.a $(B)/libamigacompat.a -lixcompat

# ---- host test build: the same backend on this machine ---------------------
# UV_NO_THREADS + UV_POSIX_POLL select nothreads.c, the synchronous work
# queue and posix-poll.c; poll() is libixcompat's own select()-based one
# (compiled from ixemul-vtcon, read-only), renamed so the system's stays
# out of the way. uv_spawn stays on fork() here: macOS's vfork() does not
# share memory with the child (probed), so the vfork path (UV__SPAWN_VFORK)
# is only testable on the Amiga (uvsmoke's spawn checks).
IXCOMPAT ?= $(HOME)/Code/ixemul-vtcon/compat
HCFLAGS = -O1 -g -Wall -Wno-unused -Wno-deprecated-declarations \
          -DUV_NO_THREADS -DUV_POSIX_POLL -Dpoll=uvhost_select_poll \
          -D_DARWIN_UNLIMITED_SELECT=0 -I$(UV)/include -I$(UV)/src
H_UV_OBJS = $(addprefix $(H)/libuv/,$(UV_SRCS:.c=.o)) $(H)/libuv/ixpoll.o
H_UV_OBJS := $(filter-out $(H)/libuv/src/unix/amiga-os.o,$(H_UV_OBJS))

$(H)/libuv/%.o: $(UV)/%.c $(UV_HDRS)
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HCFLAGS) -c -o $@ $<

$(H)/libuv/ixpoll.o: $(IXCOMPAT)/poll.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HCFLAGS) -c -o $@ $<

$(H)/libuv.a: $(H_UV_OBJS)
	rm -f $@
	ar rcs $@ $(H_UV_OBJS)

$(H)/uvsmoke: tests/uvsmoke.c $(H)/libuv.a
	$(HOSTCC) $(HCFLAGS) -o $@ tests/uvsmoke.c $(H)/libuv.a

# libuv's own test runner over the host build (tests by name: see
# tools/uv-host-tests.sh for the list that matters to Neovim)
UV_TEST_SRCS = $(filter-out $(UV)/test/benchmark-% $(UV)/test/runner-win.c \
                 $(UV)/test/run-benchmarks.c $(UV)/test/echo-server.c \
                 $(UV)/test/blackhole-server.c,$(wildcard $(UV)/test/*.c))
$(H)/uv-run-tests: $(UV_TEST_SRCS) $(H)/libuv.a
	$(HOSTCC) $(HCFLAGS) -w -include pthread.h -I$(UV)/test -o $@ $(UV_TEST_SRCS) \
	  $(UV)/test/echo-server.c $(UV)/test/blackhole-server.c $(H)/libuv.a -lpthread

# ---- Lua 5.1.5 (PUC; doubles, so soft float without an FPU) ---------------
LUA     = vendor/lua/src
LUA_SRCS = lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c \
           lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c \
           lundump.c lvm.c lzio.c lauxlib.c lbaselib.c ldblib.c liolib.c \
           lmathlib.c loslib.c ltablib.c lstrlib.c loadlib.c linit.c
LUA_OBJS = $(addprefix $(B)/lua/,$(LUA_SRCS:.c=.o))
LUA_DEFS = -DLUA_USE_POSIX

$(B)/lua/%.o: $(LUA)/%.c $(wildcard $(LUA)/*.h) $(COMPAT_HDRS)
	@mkdir -p $(dir $@)
	$(AGCC) $(ACFLAGS) $(COMPAT_INC) $(LUA_DEFS) -c -o $@ $<

$(B)/liblua.a: $(LUA_OBJS)
	rm -f $@
	$(AAR) rcs $@ $(LUA_OBJS)

# the stand-alone interpreter, to time and test Lua on the Amiga
$(B)/lua51: $(B)/lua/lua.o $(B)/liblua.a $(B)/libamigacompat.a
	$(AGCC) $(ALDFLAGS) -o $@ $(B)/lua/lua.o $(B)/liblua.a $(B)/libamigacompat.a -lixcompat

# ---- msgpack-c 3.0.0, the C part -------------------------------------------
MP      = vendor/msgpack-c
MP_SRCS = objectc.c unpack.c version.c vrefbuffer.c zone.c
MP_OBJS = $(addprefix $(B)/msgpack/,$(MP_SRCS:.c=.o))

$(B)/msgpack/%.o: $(MP)/src/%.c $(COMPAT_HDRS)
	@mkdir -p $(dir $@)
	$(AGCC) $(ACFLAGS) $(COMPAT_INC) -I$(MP)/include -c -o $@ $<

$(B)/libmsgpackc.a: $(MP_OBJS)
	rm -f $@
	$(AAR) rcs $@ $(MP_OBJS)

# ---- unibilium 92d929f ------------------------------------------------------
# terminfo search path on the Amiga: $TERMINFO, then UP-Term's kit
# (ENV:TERMINFO is set by its Install), then the GG tree
UB      = vendor/unibilium
UB_SRCS = unibilium.c uninames.c uniutil.c
UB_OBJS = $(addprefix $(B)/unibilium/,$(UB_SRCS:.c=.o))
UB_DEFS = -DTERMINFO_DIRS='"/usr/share/terminfo:/usr/lib/terminfo:/gg/share/terminfo"'

$(B)/unibilium/%.o: $(UB)/%.c $(UB)/unibilium.h $(COMPAT_HDRS)
	@mkdir -p $(dir $@)
	$(AGCC) $(ACFLAGS) $(COMPAT_INC) $(UB_DEFS) -I$(UB) -c -o $@ $<

$(B)/libunibilium.a: $(UB_OBJS)
	rm -f $@
	$(AAR) rcs $@ $(UB_OBJS)

# ---- libtermkey 0.21.1 (on unibilium, as Neovim builds it) ----------------
TK      = vendor/libtermkey
TK_SRCS = termkey.c driver-csi.c driver-ti.c
TK_OBJS = $(addprefix $(B)/termkey/,$(TK_SRCS:.c=.o))

$(B)/termkey/%.o: $(TK)/%.c $(TK)/termkey.h $(TK)/termkey-internal.h $(COMPAT_HDRS)
	@mkdir -p $(dir $@)
	$(AGCC) $(ACFLAGS) $(COMPAT_INC) -DHAVE_UNIBILIUM -I$(UB) -I$(TK) -c -o $@ $<

$(B)/libtermkey.a: $(TK_OBJS)
	rm -f $@
	$(AAR) rcs $@ $(TK_OBJS)

# ---- libvterm 7c72294 (Neovim's fork) --------------------------------------
VT      = vendor/libvterm
VT_SRCS = encoding.c keyboard.c mouse.c parser.c pen.c screen.c state.c \
          unicode.c vterm.c
VT_OBJS = $(addprefix $(B)/vterm/,$(VT_SRCS:.c=.o))
VT_INCS = $(B)/vterm-gen/encoding/DECdrawing.inc $(B)/vterm-gen/encoding/uk.inc

$(B)/vterm-gen/encoding/%.inc: $(VT)/src/encoding/%.tbl
	@mkdir -p $(dir $@)
	perl -C $(VT)/tbl2inc_c.pl $< > $@

$(B)/vterm/%.o: $(VT)/src/%.c $(VT_INCS) $(COMPAT_HDRS)
	@mkdir -p $(dir $@)
	$(AGCC) $(ACFLAGS) $(COMPAT_INC) -std=gnu99 -I$(VT)/include -I$(VT)/src \
	  -I$(B)/vterm-gen -c -o $@ $<

$(B)/libvterm.a: $(VT_OBJS)
	rm -f $@
	$(AAR) rcs $@ $(VT_OBJS)

# ---- luv 1.30.0-0 (libuv for Lua; vim.loop) --------------------------------
LUV     = vendor/luv
$(B)/luv/luv.o: $(wildcard $(LUV)/src/*.c $(LUV)/src/*.h) $(B)/libuv.a $(COMPAT_HDRS)
	@mkdir -p $(dir $@)
	$(AGCC) $(ACFLAGS) $(COMPAT_INC) -I$(UV)/include -I$(LUA) \
	  -Ivendor/lua-compat-5.3 -Ivendor/lua-compat-5.3/c-api \
	  -DLUA_COMPAT_APIINTCASTS -c -o $@ $(LUV)/src/luv.c

$(B)/libluv.a: $(B)/luv/luv.o
	rm -f $@
	$(AAR) rcs $@ $(B)/luv/luv.o

# ---- sysroot: the dependencies as Neovim's CMake finds them ---------------
SYSROOT_DEPS ?= libs
SR = $(B)/sysroot
sysroot: $(SYSROOT_DEPS)
	rm -rf $(SR) && mkdir -p $(SR)/include/luv $(SR)/include/uv $(SR)/lib
	cp $(UV)/include/*.h $(SR)/include/ && cp $(UV)/include/uv/*.h $(SR)/include/uv/
	cp $(LUA)/lua.h $(LUA)/luaconf.h $(LUA)/lualib.h $(LUA)/lauxlib.h $(SR)/include/
	printf 'extern "C" {\n#include "lua.h"\n#include "lualib.h"\n#include "lauxlib.h"\n}\n' > $(SR)/include/lua.hpp
	cp -R $(MP)/include/msgpack $(MP)/include/msgpack.h $(SR)/include/
	cp $(UB)/unibilium.h $(TK)/termkey.h $(VT)/include/*.h $(SR)/include/
	cp $(LUV)/src/luv.h $(LUV)/src/util.h $(LUV)/src/lhandle.h $(LUV)/src/lreq.h $(SR)/include/luv/
	cp $(B)/libuv.a $(B)/liblua.a $(B)/libmsgpackc.a $(B)/libunibilium.a \
	   $(B)/libtermkey.a $(B)/libvterm.a $(B)/libluv.a $(wildcard $(B)/libamigacompat.a) $(SR)/lib/

# ---- host Neovim: the same no-threads libuv and inline TUI on this Mac ----
# (tools/configure-nvim.sh --host). Proves the UV_NO_THREADS paths in
# Neovim itself (TUI on the main loop, synchronous work queue) without the
# rig; the __amigaos__ paths (vfork, PTY:) only run on the Amiga.
HD = build/hostdeps
HD_MAKE = $(MAKE) -o $(HD)/libuv.a B=$(HD) AGCC=$(HOSTCC) AAR=ar \
          ACFLAGS='-O1 -g -w -DUV_NO_THREADS -DUV_POSIX_POLL' COMPAT_INC= COMPAT_HDRS=
host-deps: $(H)/libuv.a
	mkdir -p $(HD) && cp $(H)/libuv.a $(HD)/libuv.a
	$(HD_MAKE) $(HD)/liblua.a $(HD)/libmsgpackc.a $(HD)/libunibilium.a \
	  $(HD)/libtermkey.a $(HD)/libvterm.a $(HD)/libluv.a
	$(HD_MAKE) SYSROOT_DEPS= sysroot

# ---- dist: what goes onto the Amiga ----------------------------------------
# build/m68k/dist/nvim/bin/nvim (stripped) + share/nvim/runtime, the layout
# Neovim finds its runtime by (../share/nvim/runtime from the binary). The
# runtime comes from the host build's install (same 0.4.4 tree): its
# generated syntax/vim/generated.vim and doc/tags cannot be made by
# running the m68k binary here.
DIST = $(B)/dist/nvim
dist:
	ninja -C $(B)/nvim nvim
	ninja -C $(H)/nvim nvim
	rm -rf $(H)/stage && DESTDIR=$(CURDIR)/$(H)/stage ninja -C $(H)/nvim install > $(H)/install.log
	rm -rf $(B)/dist && mkdir -p $(DIST)/bin $(DIST)/share/nvim
	$(AMIGA)/bin/m68k-amigaos-strip -o $(DIST)/bin/nvim $(B)/nvim/bin/nvim
	chmod +x $(DIST)/bin/nvim
	cp -R $(H)/stage/usr/local/share/nvim/runtime $(DIST)/share/nvim/
	cp $(B)/uvsmoke $(B)/lua51 $(B)/dist/ 2>/dev/null || true
	ls -l $(DIST)/bin/nvim
