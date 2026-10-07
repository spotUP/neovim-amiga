# neovim-amiga

Neovim port for AmigaOS 3.x (68020+) with ixemul, for the UP-Term kit. Part of the upterm source tree.

## Build

Commands: `RULES.md`, section `## Commands`. Two targets share `amiga/compat`:
Neovim 0.4.4 (the baseline, `Makefile`, `vendor/`, output `build/m68k/dist/`)
and Neovim 0.12.5 (`Makefile.v012`, `vendor012/`, output
`build/v012/dist/nvim`). The kit takes the second (`NVIM_DIST=`). The sources
of both are committed; `dl/` holds the upstream tarballs (gitignored, not
needed to build).

The 0.12 path, from the script guards (`tools/configure-nvim012.sh` refuses
to run m68k before the host build has made `nlua0`); not run end to end on a
clean machine, check with `make -f Makefile.v012 -n dist`:

    make -f Makefile.v012 libs host-deps
    tools/configure-nvim012.sh --host        # host nvim: makes the nlua0 and the generators the m68k build needs
    make -f Makefile.v012 sysroot
    tools/configure-nvim012.sh               # m68k
    make -f Makefile.v012 dist

Needs cmake and ninja (`brew install cmake ninja`), bebbo's gcc 6.5 in
`~/opt/amiga`, and ixemul-vtcon's SDK headers and `libixcompat.a` installed
(see the `upterm` repo's README, "Set up the whole thing").
