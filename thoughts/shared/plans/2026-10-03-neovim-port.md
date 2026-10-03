---
date: 2026-10-03
topic: Neovim on AmigaOS 3.x / 68020 / ixemul 48.2 (UP-Term plan item Q1)
tags: [neovim, libuv, ixemul, amigaos, up-term, vtcon]
status: draft
---

# Neovim port ledger

Source of the task: `~/Code/vtcon/thoughts/shared/plans/2026-09-28-vtcon.md`, Phase Q (Q1, Q2).
Prior art: `~/Code/tmux-amiga` (tmux 3.6a + libevent, vfork spawn, BSD ptys on PTY:).
Runtime: `~/Code/ixemul-vtcon` (patched ixemul 48.2; read-only from here: fixes are REQUESTS
in section R below). Commands: `RULES.md`.

Finish line for this run: every dependency cross-built, libuv backend built and host-tested,
Neovim linked for m68k-amigaos-ixemul, and the exact rig steps written down for the main
session (this agent does not touch the rig). **Status: 33 of 35 done; first rig run
(vtcon main session, 2026-10-03) passed 7 of 7. Open: E1 (:terminal on the rig), E2 (speed).
Baseline tagged `amiga-0.4.4-1`; the 0.12.5 port is planned in section Q below.**

## Decisions (do not re-litigate)

- **D-1 Neovim 0.4.4** (2020-08). Why: (a) PUC Lua 5.1 is an official build (`PREFER_LUA`),
  and the only Lua that runs at startup is `src/nvim/lua/vim.lua` (a few hundred lines);
  0.5+ grows the startup Lua every release, and 0.10 runs hundreds of KB of Lua at startup,
  which PUC Lua on a 68020 would compile on every start. (b) No tree-sitter (0.5+), whose
  parsers load with dlopen, which ixemul does not have. (c) The TUI and the editor are one
  process; 0.9+ always splits them into two processes (`nvim --embed` child), which would load
  the binary twice. Cost of 0.4.4: its TUI runs in a second THREAD (ui_bridge.c) -- see D-4.
  Upgrade path: the libuv backend carries over unchanged; 0.10 would add the process split,
  static tree-sitter parsers, and precompiled bytecode.
- **D-2 Dependency versions = what 0.4.4 pins** in `third-party/CMakeLists.txt` (libuv 1.30.0,
  msgpack-c 3.0.0, Lua 5.1.5, unibilium 92d929f, libtermkey 0.21.1, libvterm 7c72294,
  luv 1.30.0-0, lua-compat-5.3 0.7). Every tarball's SHA-256 matched Neovim's.
- **D-3 libuv: port its UNIX backend, not a new one.** The poller is libuv's own
  `src/unix/posix-poll.c` (Cygwin and Hurd use it) over libixcompat's `poll()` (select-based).
  Platform functions in `src/unix/amiga.c` + `amiga-os.c` (the NDK calls, kept apart from the
  POSIX headers). Threads: `nothreads.c` replaces thread.c; the work queue runs requests on
  the loop (work then done, never inside the submit). Both switches (`UV_NO_THREADS`,
  `UV_POSIX_POLL`) are settable on a host to test the same code.
- **D-4 The 0.4.4 TUI runs on the main loop** under `UV_NO_THREADS`: `tui_attach_inline()`
  attaches it directly, like a remote UI; no ui_bridge thread.
- **D-5 Host generator Lua = host LuaJIT** (`/usr/local/bin/luajit`, x86_64) with lpeg 1.0.2
  and libmpack-lua 1.0.7 (libmpack 1.0.5) built as x86_64 bundles into `build/host/lua/`.
- **D-6 Build Neovim with its own CMake** and a toolchain file (CMAKE_SYSTEM_NAME Generic, so
  CMake's UNIX is false: no `-lm -lutil`, which are newlib's; `-DUNIX` in the C flags).
  pkg-config is pointed at the empty sysroot from the toolchain file (else Homebrew's libuv
  adds `-lpthread -lm`, and CMake's self-regeneration re-reads only the toolchain file).
- **D-7 Link with `-mcrt=ixemul` alone** (objects are `-m68020`): with `-m68020` at link the
  driver picks libnix's crt0, the ixemul SDK has no libm020 multilib. Never `-lm`.
- **D-8 Stack: a `$STACK: 1048576` cookie** in main.c (vsh and the 3.2 shell honour it).
  1 MB because eval, regexp and Lua recurse through C; unmeasured on the Amiga (E3).
- **D-9 Runtime layout `<dir>/bin/nvim` + `<dir>/share/nvim/runtime`**: Neovim finds its
  runtime relative to `v:progpath` (uv_exepath: "/Vol/dir/bin/nvim"); no $VIM needed. The
  runtime is taken from the host build's install (same tree; generated `syntax/vim/generated.vim`
  and `doc/tags` need a running nvim). No runtime file name exceeds 30 characters (FFS), no
  two differ only by case.
- **D-10 Host proof build**: `build/host/nvim` is Neovim 0.4.4 on the no-threads,
  posix-poll libuv, so the UV_NO_THREADS paths (TUI on the main loop, the work queue) are
  tested off the Amiga; only the `__amigaos__` paths (vfork, PTY:, E-clock, exepath) need the rig.

## Checklist (35 items; 31 done, 4 open)

### A. Repository and host tools
- [x] A1 repo, pristine sources in `vendor/` -- 829909a
- [x] A2 host generator Lua (`tools/host-lua.sh`, `tools/lua-host`) -- c68eca4; proof: the script asserts lpeg, mpack, bit load
- [x] A3 `RULES.md` (Commands), `CLAUDE.md` pointer, `Makefile` -- c68eca4

### B. libuv for ixemul
- [x] B1 `include/uv/unix.h` + `uv/nothreads.h` -- 802f46c
- [x] B2 `src/unix/nothreads.c` (a wait only another thread could end aborts with a message) -- 802f46c
- [x] B3 work queue without threads -- 802f46c; proof: uvsmoke work-not-inside-submit, work-after
- [x] B4 platform: hrtime (timer.device E-clock), exepath, memory, uptime, cpu model; rss/interfaces UV_ENOSYS -- 802f46c
- [x] B5 posix-poll over libixcompat poll() -- 802f46c
- [x] B6 uv_spawn with vfork: exec error via a shared static, environ restored in the parent -- 802f46c
- [x] B7 signals: libuv's signal pipe, no pthread_atfork -- 802f46c; proof (host): uvsmoke signal
- [x] B8 libuv.a for m68k, uvsmoke links -- 802f46c
- [x] B9 host build + libuv's own suite: 285 of 361 pass, 21 skip, 55 fail, each accounted for
      below (`tools/uv-host-tests.sh`, record in build/host/uv-tests.tsv) -- 802f46c
- [x] B11 blocking ttys read once per readiness; all ttys stay blocking (shared console) -- 0c7e217
- [x] B10 rig: uvsmoke PASS, 0 failed (main session, 2026-10-03)

### C. Other dependencies (m68k)
- [x] C1 Lua 5.1.5 `liblua.a` + `lua51` interpreter -- 019618e
- [x] C2 msgpack-c 3.0.0 C part (plain refcounts on AmigaOS, e83eb7d) -- 019618e
- [x] C3 unibilium -- 019618e
- [x] C4 libtermkey (unibilium driver) -- 019618e
- [x] C5 libvterm (no symbol visibility in hunk objects) -- 019618e
- [x] C6 luv + lua-compat-5.3 -- 019618e
- [x] C7 libamigacompat: IPv6 types, getaddrinfo family, pread/pwrite, nanosleep, mkdtemp, strnlen,
      getpw*_r, ttyname_r, if_indextoname, lchown, __eprintf, atan2 -- 054b772, 019618e
- [x] C8 libamigacompat: inttypes.h, C99 math, wctype.h (declarations), O_NOFOLLOW, va_start -- 6388267

### D. Neovim 0.4.4
- [x] D1 CMake toolchain + configure (`tools/configure-nvim.sh`) -- fa5935f
- [x] D2 generators on the host LuaJIT -- fa5935f
- [x] D3 compile: no errors, no implicit declarations under -Wall -Wextra -- fa5935f
- [x] D4 TUI on the main loop -- 6b77d5c; proof: tests/tui_drive.py on build/host/nvim, PASS 8 of 8
      (the commit message says "9 of 9": wrong count, the run has 8 checks)
- [x] D5 tui/input key buffer without a condition wait (6b77d5c); os_delay's uv_cond_timedwait sleeps (nothreads.c)
- [x] D6 nvim links: **3,132,944 bytes unstripped, 3,009,056 stripped** (.text 2.5 MB, .data 66 KB, .bss 85 KB)
- [x] D7 dist layout (`make dist` -> build/m68k/dist/nvim, 20 MB with runtime) + stack cookie (D-8)
- [x] D9 :terminal ptys: PTY: master + vfork child (`amiga_pty_spawn`) -- ed6821a (not run)
- [x] D10 host build of the same tree (`make host-deps`, `configure-nvim.sh --host`) -- fa5935f
- [x] D11 rig script `tools/nvim_rig.py` (steps 1-7), its Neovim commands checked on the host build
- [x] D8 rig (2026-10-03, nvim_rig.py, 7 of 7): --version; headless writefile (runtime found at
      /VTCX/nvim-test/nvim/share/nvim/runtime, vim.loop v:true); system() through vsh = spawned-ok;
      TUI in an XCON: window with TERM=xterm-256color: intro screen, typed text saved, :q; splits
      (-O, :split), syntax, number, cursorline drawn right. Screenshots in build/rig/.

### E. After the rig answers (in order)
- [ ] E1 fix what D8/B10 show; :terminal in Neovim on PTY: (D9) on the rig
- [ ] E2 startup time and memory on the 68020. Rig (FS-UAE, NOT cycle-exact): headless with
      defaults 53.9 s. The A1200 number is unknown. Decide whether the defaults need trimming.

Next run, not counted above: E3 stack depth measured (a `:call` chain to 'maxfuncdepth', a
big regexp); E4 Q2 (vtcon plan): Neovim 0.4.4's test/functional tui_spec through the engine.

## Not implemented (and what it blocks)

| What | Where | Blocks |
|---|---|---|
| Threads (uv_thread_create = UV_ENOSYS; cond wait / held mutex / sem at 0 abort) | libuv nothreads.c | luv's `vim.loop.new_thread`; nothing in Neovim core |
| Work queue runs on the loop: a slow fs request or name lookup blocks the editor for its duration | libuv threadpool.c | nothing; responsiveness during slow async fs |
| uv_fs_event (file watching) | libuv no-fsevents.c (ENOSYS) | luv `fs_event`; Neovim core does not use it |
| uv_dlopen fails with a message | libuv dl.c | Lua C modules; `libcall()` |
| Process title (uv_set_process_title stores nothing) | no-proctitle.c | nothing |
| uv_resident_set_memory, uv_interface_addresses UV_ENOSYS; uv_loadavg 0; cpu speed 0 | amiga.c | the matching `vim.loop` calls |
| IPv6 (types only; socket(AF_INET6) fails at run time) | compat netinet/in.h | IPv6 server/channel addresses |
| getaddrinfo is IPv4 over gethostbyname | compat netdb.c | nothing |
| O_NOFOLLOW is 0 (opens through a link) | compat fcntl.h | the symlink-attack guard of swap/undo/backup files |
| lchown ENOSYS; if_indextoname ENXIO | compat posix.c | nothing |
| pread/pwrite not atomic against another process sharing the offset | compat posix.c | nothing in one process |
| wctype functions: declarations only | compat wctype.h | nothing (Neovim uses its own tables) |
| No gettext/iconv (ENABLE_LIBINTL/ICONV OFF) | configure | translated messages; encodings other than Neovim's built-in UTF-8/Latin-1/UCS |
| TUI data kept at exit (handles' close callbacks run in loop_close) | tui.c tui_stop_inline | nothing (exit only) |
| Typeahead back-pressure re-polls while the editor's input buffer is full | tui/input.c | CPU while >4 KB of typed keys wait |

libuv suite (host build) failures, by cause: threads (async, async_null_cb, barrier_*,
condvar_*, eintr_handling, embed, fs_partial_read/write, ipc_send_recv_*_inprocess,
pipe_set_non_blocking, process_title_threadsafe, semaphore_*, signal_multiple_loops,
thread_*, threadpool_cancel_*, threadpool_multiple_event_loops: they start threads or block
one on a semaphore); fs_event_* (not implemented); process_title; platform_output
(rss ENOSYS); ip6_addr_link_local (interface list ENOSYS); poll_bad_fdtype (poll() accepts a
regular file, epoll does not); poll_duplex, poll_unidirectional (time out on macOS with the
native poll too: host-side, unconfirmed on the Amiga; Neovim does not use uv_poll);
tcp_try_write_error (macOS answers ECONNRESET where the test wants EPIPE: host OS).

## R. Requests for ixemul-vtcon (not done here)

- **R1 libixcompat / SDK**: take over `amiga/compat/` -- `<inttypes.h>`, C99 `<math.h>` parts
  (fpclassify family, INFINITY/NAN, HUGE_VAL without the overflowing `1e500`, trunc, fmin,
  fmax, remainder, copysign, atan2), `<wctype.h>`, IPv6 types in `<netinet/in.h>`,
  getaddrinfo/freeaddrinfo/getnameinfo/gai_strerror, pread/pwrite, nanosleep, mkdtemp, strnlen,
  getpwuid_r/getpwnam_r, ttyname_r, if_indextoname, imaxabs/strtoimax, `_SC_GETPW_R_SIZE_MAX`,
  `__eprintf` for `<assert.h>` (libgcc's writes through newlib's `_impure_ptr`). Then this
  repo drops its copies. A real `O_NOFOLLOW` in `open()` would need the library itself.
- **R2 `machine/stdarg.h` va_start**: `__builtin_next_arg(last)` is not seen by gcc as variadic
  use, so IPA constant propagation clones a variadic `static inline` and then fails with
  "va_start used in function with fixed args" (Neovim's `event_create`). Use
  `__builtin_va_start(*(__builtin_va_list *)&(ap), last)` -- on m68k it yields the same char *
  (this repo's `amiga/compat/include/stdarg.h` does exactly that).
- **R3 `unsetenv`** returns void (4.3BSD); POSIX says int. libuv now copes, others may not.
- **R4 (question, answer from the rig)**: does `read()` on an O_NONBLOCK XCON:/PTY: console
  return EAGAIN when nothing is typed? Neovim opens stdin with uv_pipe_open, which sets
  O_NONBLOCK (as on every Unix), and restores blocking at exit.

## Gotchas

- The compiler defines `AMIGA`, `amiga`, `amigaos`, `MCH_AMIGA` as 1: guard with `__amigaos__`.
- `size_t` is `unsigned int` under gcc 6 here; `off_t` 32 bits; `long` 32 bits; int64_t is long long.
- macOS's vfork does not share memory with the child (probed), so the vfork spawn path is
  testable only on the Amiga; the host build keeps libuv's fork path.
- Do not name a build output `build/m68k/lua`: the object directory is `build/m68k/lua/`.
- Every file that includes uv.h on the host needs `-DUV_NO_THREADS -DUV_POSIX_POLL` (struct layout).
- unibilium's TERMINFO_DIRS is ':'-separated, so an AmigaOS path ("Vol:dir") cannot be in it;
  $TERMINFO (one directory) can.
- ixemul's argv parsing honours a quote only at the start of a word: `+"call f(a, b)"` splits
  at the spaces, write `"+call f(a, b)"`, `"--cmd" "set shell=/VTC/vsh"` (rig, 2026-10-03).
  `+"cq 7"` hung nvim on the rig (not investigated; the rig had to restart).
- vsh does not find a program by a Unix path (/VTC/nvim-test/...); Amiga paths (VTC:...) work.
  vsh's issue (vtcon), not this repo's.
- AmigaDOS shell: `*` is its escape character and `$NAME` is expanded, also inside quotes --
  the rig script avoids both in command lines.
- Neovim 0.4.4's own host quirks: os/lang.c needs `-include locale.h` on macOS, and its LuaJIT
  link flags (`-pagezero_size`) make a malformed arm64 binary (now only with LuaJIT).

## Rig steps (for the main session; this agent never drives the rig)

1. `cd ~/Code/neovim-amiga && make dist` (already built: build/m68k/dist).
2. Rig up (`cd ~/Code/vtcon && python3 tools/rig/rig.py start`), kit not installed.
3. `python3 ~/Code/neovim-amiga/tools/nvim_rig.py --tui` -- copies build/m68k/dist to
   `~/Code/vtcon/build/rig/vtc/nvim-test/`, puts the patched ixemul first in LIBS:, mounts PTY:,
   then: (1) VTC:nvim-test/uvsmoke, (2) lua51, (3) nvim --version, (4) headless writefile with
   $VIMRUNTIME and vim.loop, (5) system() through /VTC/vsh, (6) headless start with defaults
   (time), (7) TUI in an XCON: window: type, save to RAM:nvtui.txt, quit; screenshot
   build/rig/nvim_tui.png in this repo.
4. Send back the whole output (every ok/FAIL line, uvsmoke's lines, the times) and the
   screenshot. If a step hangs: `Status` output and RAM: listing.

---

# Q. Neovim 0.12.5 (owner, 2026-10-03: "the ported neovim is an old version")

0.4.4 stays the working baseline: tag `amiga-0.4.4-1`, its tree `vendor/`, its build `build/m68k`,
`build/host`. 0.12.5 lives beside it: `vendor012/` (pristine a4503cf), `Makefile.v012`, builds in
`build/v012/`. Nothing of 0.4.4 is changed by the 0.12 work.

## Q facts (read from the 0.12.5 sources, `vendor012/`)

- **Dependencies** (cmake.deps/deps.txt, all SHA-256 checked): libuv **1.52.1**, luv 1.52.1-0,
  Lua 5.1.5 (or LuaJIT), unibilium 2.1.2, lpeg 1.1.0 (now linked into nvim: `vim.lpeg`),
  lua-compat-5.3 0.13, utf8proc 2.11.3, tree-sitter 0.26.13 + 6 parsers. Gone since 0.4.4:
  msgpack-c (own encoder), libvterm and libtermkey (vendored into src/nvim). CMake asks
  only libuv >= 1.28, but luv 1.52 calls 1.52's API, so libuv 1.52.1 it is.
- **PUC Lua 5.1: still supported.** `PREFER_LUA` (src/nvim/CMakeLists.txt:62) does
  `find_package(Lua 5.1 EXACT REQUIRED)`; `src/bit.c` supplies `bit`. Only the unit-test
  harness needs LuaJIT.
- **Threads: Neovim's core starts none** (the only uv_thread_create is os/pty_proc_win.c).
  It uses two mutexes (log.c recursive, runtime.c plain), which nothreads.c keeps as real
  state. Threads would come only from Lua: `vim.uv.new_thread` (UV_ENOSYS) and
  `new_work`/async fs, which the no-threads work queue runs on the loop. No Neovim runtime
  Lua calls new_thread or new_work (grep of runtime/lua). **The no-threads layer covers 0.12.**
  libuv 1.52 adds thread API (affinity, names, priority, detach) that nothreads.c must answer
  with UV_ENOSYS.
- **The TUI is a separate process since 0.9.** `nvim` with a terminal runs as a UI client:
  main.c:356 `ui_client_start_server` spawns `nvim --embed` (channel_job_start, stdio
  pipes), and both processes run `nlua_init`, i.e. `vim._init_packages` -> `vim._core.shared`,
  `editor`, `system`, `options` (130 KB of Lua source each, plus `vim._core.defaults` 39 KB in
  the server). On AmigaOS, which has no copy-on-write and does not share the text of a
  non-resident program, that is two loaded binaries, two Lua heaps, and the Lua start-up paid
  twice, plus a second LoadSeg of a multi-MB file at every start.
- **One process is possible without touching the UI code**: the server already talks to every
  UI, the built-in TUI included, as a remote UI over a msgpack channel, and the client decodes
  `redraw` straight into the TUI (msgpack_rpc/channel.c:502). A loopback channel in one
  process: a socketpair, the server's end as a UI channel (what `--embed` makes of stdio),
  the client's end as `ui_client_channel_id`, both on main_loop, `tui_start` + `ui_client_attach`
  without `ui_client_run`'s private loop. ~20 places test `ui_client_channel_id` to mean "this
  process is only a client" (log.c, memory.c, ui.c x3, channel.c, event/proc.c, ...): they get an
  "in-process" flag. Cost: the msgpack encode + decode of each redraw, which two processes pay too.
- **Tree-sitter needs dlopen**: lua/treesitter.c:138 loads parsers with uv_dlopen/uv_dlsym of
  `parser/<lang>.so` found on 'runtimepath'. ftplugin/help.lua, lua.lua, markdown.lua and
  query.lua call `vim.treesitter.start()`, so opening :help needs the vimdoc parser.
- **Lua bytecode**: `COMPILE_LUA` (default ON) has the HOST Lua `string.dump` the embedded
  modules; PUC Lua 5.1 bytecode carries the dumping machine's endianness and sizes, so a host
  dump will not load on m68k ("bad header").
- **Generators run in `nlua0`**, a Lua module built from Neovim's sources; for a cross build
  CMake takes a host-built one (`NLUA0_HOST_PRG`, src/nvim/CMakeLists.txt:533) and a host Lua
  that can load it (`LUA_GEN_PRG`): the host LuaJIT is x86_64, the host nlua0 arm64, so the
  generator Lua is a host PUC Lua 5.1 built arm64 from vendor/lua.
- Parser sources: c 3.7 MB, vim 4.5 MB, markdown 2.0 + inline 2.2 MB, vimdoc 0.5 MB,
  lua 0.35 MB, query 0.1 MB (parser.c). Their compiled tables dominate any static link.

## Q decisions

- **Q-D1 Process model: one process on AmigaOS** (loopback channel, above), as the default
  for `nvim` in a terminal; `--embed` and `--remote-ui` keep working as upstream. Order: first
  the upstream two-process start (no UI patches; uv_spawn+socketpair stdio is proven on the
  rig by system()), so the port has a working baseline; then the loopback, measured against
  it (memory: AvailMem before/after; start time). Two processes stay selectable
  (`NVIM_AMIGA_TWO_PROCESS=1`) until the loopback has passed the TUI tests.
- **Q-D2 libuv 1.52.1 with the same backend**: re-apply the 1.30 port (nothreads.c, the
  synchronous work queue, posix-poll, vfork spawn, amiga.c/amiga-os.c, tty once-per-readiness,
  dl.c), plus 1.52's new platform calls; `uv_random` from timer.device E-clock jitter is NOT a
  CSPRNG -> UV_ENOSYS unless ixemul grows a /dev/urandom (request R5).
- **Q-D3 PUC Lua 5.1.5** (same as 0.4.4). COMPILE_LUA OFF first (sources embedded, compiled
  at start); then a host "cross-dump" Lua 5.1 (ldump.c writing big-endian, 4-byte int/size_t,
  8-byte double) so the embedded modules ship as m68k bytecode -- the start-up item.
- **Q-D4 Tree-sitter parsers linked statically; dlopen served from a table**: libuv's AmigaOS
  dl.c gets a static library registry (`uv_amiga_static_lib(name, symbols)`); uv_dlopen of a
  path whose file name is registered (`vimdoc.so`) succeeds, uv_dlsym returns the linked
  `tree_sitter_vimdoc`. The runtime ships empty `parser/<lang>.so` markers so Neovim's own
  search finds them. No Lua or treesitter.c change. Linked: vimdoc, lua, query, markdown,
  markdown_inline (what the bundled ftplugins start); c and vim behind a build option until
  the size is measured. tree-sitter's atomic refcounts get a no-atomics path (one thread).
- **Q-D5 Separate tree and Makefile** (`vendor012/`, `Makefile.v012`, `build/v012/`), shared
  `amiga/compat/`, shared tools. 0.4.4 keeps building.
- **Q-D6 Host first**: Neovim 0.12.5 for this Mac on the no-threads libuv 1.52 (as D-10),
  tui_drive.py against it, then the m68k cross build; the host build also gives nlua0 and the
  runtime install.

## Q checklist (0 of 17)

- [ ] Q1 host PUC Lua 5.1 (arm64) as generator Lua (`build/v012/host/lua51`)
- [ ] Q2 libuv 1.52.1 backend ported (patches of 802f46c + 0c7e217), uvsmoke 1.52 on the host
- [ ] Q3 libuv 1.52 own suite on the host, failures accounted for
- [ ] Q4 host deps: luv 1.52, lpeg 1.1.0, unibilium 2.1.2, utf8proc, tree-sitter + parsers
- [ ] Q5 host nvim 0.12.5 (PREFER_LUA, no-threads libuv) builds; --version, headless
- [ ] Q6 host: tui_drive.py passes against 0.12.5 (two processes, upstream)
- [ ] Q7 static parser registry in libuv dl.c + markers; `:help` highlights through vimdoc (host proof with a static build)
- [ ] Q8 m68k deps cross-built (libuv 1.52, luv, lpeg, unibilium, utf8proc, tree-sitter, parsers)
- [ ] Q9 m68k nvim 0.12.5 links (NLUA0_HOST_PRG, COMPILE_LUA OFF); size recorded, with and without c/vim parsers
- [ ] Q10 dist tree for 0.12.5 (`make -f Makefile.v012 dist`)
- [ ] Q11 rig: --version, headless, system(), TUI two-process (main session)
- [ ] Q12 loopback single process (Q-D1), host tui_drive.py, then rig; memory and start time vs Q11
- [ ] Q13 cross-dump Lua: embedded modules as m68k bytecode; start time vs Q12
- [ ] Q14 user config packaging (section U) on the host
- [ ] Q15 rig: the friend's config starts, lualine + neo-tree + bufexplorer work
- [ ] Q16 :terminal on PTY: in 0.12 (pty_proc_unix.c, same vfork spawn as D9)
- [ ] Q17 Q2 of the vtcon plan: 0.12.5's tui_spec through the engine

## U. A real user config: github.com/tomviljo/dotfiles (c36d479, 2025-11-28)

What it is: `nvim/init.vim` (Vimscript options, autocmds, mappings, vim-plug, a `lua << END`
block configuring lualine and neo-tree), `nvim/colors/vanilla-toms.vim` (cterm colours:
`set notermguicolors`, the 256-colour palette UP-Term draws exactly). Plugins: nvim-web-devicons,
plenary.nvim, nui.nvim, lualine.nvim, neo-tree.nvim, bufexplorer, coq_nvim + coq.artifacts +
coq.thirdparty. Ends with `source ~/.config/nvim/init-local.vim` (machine-local, not in the repo).

Plan:
- **Plugins are installed on the host, not on the Amiga.** vim-plug's `:PlugInstall` runs
  `git clone` (and its bootstrap `curl`); neither exists for ixemul here. `tools/user-config.sh
  <dotfiles>` runs the host nvim 0.12.5 headless with private XDG dirs: copies init.vim and
  colors/, fetches plug.vim, `+PlugInstall +qa`, removes `.git` dirs, writes
  `build/v012/userconf/{config,data}` laid out as `$HOME/.config/nvim` and
  `$HOME/.local/share/nvim` (vim-plug's `plugged/`), plus an empty `init-local.vim`. That tree is
  copied to the Amiga's $HOME. At start vim-plug only adds the plugged directories to
  'runtimepath': no git needed. `:PlugInstall`/`:PlugUpdate` on the Amiga fail with vim-plug's
  own "git not found" message; updating = rerun the script and copy again.
- **coq_nvim cannot run**: it is a Python 3.8+ program (venv, SQLite) driven over RPC, and
  there is no Python 3 for AmigaOS 3.x/68k here. It does nothing until `:COQnow`
  (auto_start is off in this config), so loading it should be harmless -- to be checked on the
  host with python3 hidden from PATH. **Owner/friend decision**: keep the config unchanged (coq
  inert) or replace coq with 0.12's built-in insert completion (`'autocomplete'`).
- **Speed risk**: lualine redraws the status line and tabline from timers in Lua; on a 68020
  under PUC Lua that may cost visibly. Measure on the rig (Q15) before changing anything.
- **Nerd Font glyphs**: nvim-web-devicons, lualine (`icons_enabled = true`) and neo-tree draw
  file-type and UI icons from the Unicode Private Use Area (U+E000-U+F8FF; nf-md icons at
  U+F0001-U+F1AF0). UP-Term's renderer maps code points its font lacks to a replacement glyph,
  so these show as replacements. `listchars` uses U+2500 (box drawing), which UP-Term draws.
  **Request V1 for vtcon (not done here)**: a bitmap font plane with the Nerd Font glyphs
  nvim-web-devicons, lualine and neo-tree use (their icon tables name the code points), in
  UP-Term's cell size, single cell wide as Neovim measures them (utf8proc: width 1 for PUA).
- Other things in the config that need tools outside Neovim: `gr` mapping runs `grep -rn`
  (not in UP-Term's coreutils 5.2.1 set), `gb` needs vim-fugitive and `gd`/`gc` vim-go (not in
  the plugin list: the mappings fail only when pressed).

## R (additions for 0.12)

- **R5** `/dev/urandom` (or getentropy) in ixemul: libuv's uv_random and anything seeding from
  it; until then UV_ENOSYS.
