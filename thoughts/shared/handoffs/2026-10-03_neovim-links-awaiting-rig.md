---
date: 2026-10-03
topic: Neovim 0.4.4 for AmigaOS links; waiting for the first rig run
tags: [neovim, libuv, ixemul, handoff]
status: draft
---

# Handoff 2026-10-03: Neovim links, rig run pending

## Task
UP-Term plan Q1 (`~/Code/vtcon/thoughts/shared/plans/2026-09-28-vtcon.md`, Phase Q): Neovim on
AmigaOS 3.x/68020/ixemul. Ledger: `thoughts/shared/plans/2026-10-03-neovim-port.md`
(31 of 35 done; open: B10, D8, E1, E2 -- all need the rig).

## Critical references
- libuv backend: `vendor/libuv/src/unix/{nothreads.c,amiga.c,amiga-os.c}`, `src/threadpool.c`
  (UV_NO_THREADS branch), `src/unix/process.c` (UV__SPAWN_VFORK), `include/uv/nothreads.h`
- Neovim changes: `vendor/neovim/src/nvim/tui/{tui.c,input.c,input.h}` (UV_NO_THREADS),
  `os/pty_process_unix.c` (amiga_pty_spawn), `main.c` (stack cookie), `CMakeLists.txt`
- Build: `Makefile`, `amiga/cmake/m68k-amigaos-ixemul.cmake`, `tools/configure-nvim.sh`
- `git diff 829909a -- vendor/<pkg>` is the whole port of a package

## Learnings
- The host proof build (`build/host/nvim`, no-threads libuv) runs the inline TUI:
  `tests/tui_drive.py` 8 of 8. Only `__amigaos__` code is untested.
- macOS vfork does not share memory: vfork spawn is testable only on the Amiga.
- CMake self-regeneration ignores the configure script's environment: settings that must
  survive it live in the toolchain file.

## Next steps
1. Main session runs `python3 ~/Code/neovim-amiga/tools/nvim_rig.py --tui` (ledger: Rig steps).
2. Fix what it shows (E1), then :terminal on PTY:, startup time and memory (E2).
3. Then E3 (stack depth), E4 (Q2 tui_spec replay).
