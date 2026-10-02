# neovim-amiga project rules

Neovim 0.4.4 for AmigaOS 3.x / 68020 / ixemul 48.2 (UP-Term plan Q1). The global
rules (`~/.claude/CLAUDE.md`) apply; this file adds the project's. The ledger is
`thoughts/shared/plans/2026-10-03-neovim-port.md` (decisions, checklist, gotchas).

1. **`vendor/` holds the upstream trees; the first commit is pristine.** Every
   AmigaOS change is a later commit, so `git diff 829909a -- vendor/<pkg>` is the
   whole port of that package. Our own files inside a vendor tree say so in their
   header comment.
2. **Guard AmigaOS code with `__amigaos__`** (and `__ixemul__` where it matters),
   never `AMIGA`/`amiga`: the compiler predefines those as 1, so an identifier named
   `amiga` breaks.
3. **What ixemul lacks goes in `amiga/compat/`** (wrapper headers with
   `#include_next`, functions in `libamigacompat.a`) and is listed as a request in
   the ledger's section R: it belongs in the ixemul-vtcon SDK.
4. **Nothing here touches the rig.** Amiga runs are written down as exact steps for
   whoever drives the rig.

## Commands

| Task | Command |
|------|---------|
| Build every m68k library | `make` |
| libuv smoke test, m68k binary (run on the rig) | `make build/m68k/uvsmoke` |
| libuv smoke test on the host (same backend config) | `make build/host/uvsmoke && build/host/uvsmoke` |
| libuv's own tests, host build, resumable (records `build/host/uv-tests.tsv`) | `make build/host/uv-run-tests && tools/uv-host-tests.sh [name...]` |
| Re-run only the recorded failures | `tools/uv-host-tests.sh --failed` |
| Host generator Lua (LuaJIT + lpeg + mpack) | `tools/host-lua.sh` |
| Clean | `make clean` |
