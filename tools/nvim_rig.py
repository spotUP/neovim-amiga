#!/usr/bin/env python3
"""nvim_rig.py -- Neovim 0.4.4 (neovim-amiga) on the UP-Term FS-UAE rig.

Run by whoever drives the rig (the vtcon main session), never by the port's
agent. It reuses vtcon's rig tools read-only (~/Code/vtcon/tools/rig: ami,
install_rig.run, ixpty_rig.use_ixemul, screen_rig.typeline) and COPIES
build/m68k/dist (nvim tree, uvsmoke, lua51) to VTC:nvim-test/ -- that is,
into ~/Code/vtcon/build/rig/vtc/nvim-test.

Needs: the rig up (python3 tools/rig/rig.py start in ~/Code/vtcon), PTY:
mountable from VTC:ptymount, vsh as VTC:vsh, the kit NOT installed;
`make dist` here first. Each step prints ok/FAIL and what it saw; a FAIL
stops nothing, so one run collects every answer.

  tools/nvim_rig.py              steps 1-6 (no window)
  tools/nvim_rig.py --tui        and step 7, the TUI in an XCON: window
  tools/nvim_rig.py --tui --keep leave the window open afterwards
  tools/nvim_rig.py --v012 ...   Neovim 0.12.5 instead (build/v012/dist,
                                 `make -f Makefile.v012 dist`) into
                                 VTC:nvim012/; step 2 runs compat_probe
                                 instead of lua51; step 7 also opens :help
                                 (vimdoc through the static parser) and
                                 reports AvailMem before, during and after
"""
import os, pathlib, shutil, struct, sys, time

VTCON = pathlib.Path(os.environ.get('VTCON', pathlib.Path.home() / 'Code/vtcon'))
sys.path.insert(0, str(VTCON / 'tools/rig'))
import ami, ixpty_rig, screen_rig          # noqa: E402  (vtcon's rig tools)
import install_rig                         # noqa: E402
from install_rig import run, check         # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[1]
V012 = '--v012' in sys.argv
DIST = ROOT / ('build/v012/dist' if V012 else 'build/m68k/dist')
VTC = VTCON / 'build/rig/vtc'
DIR = 'nvim012' if V012 else 'nvim-test'
NV = 'VTC:%s/nvim/bin/nvim' % DIR
SHOT = ROOT / ('build/rig/nvim012_tui.png' if V012 else 'build/rig/nvim_tui.png')


def avail():
    """free memory (bytes) from AmigaDOS Avail"""
    rc, out = run('Avail TOTAL', 20)
    try:
        return int(out.split()[-1])
    except (ValueError, IndexError):
        return -1


def install():
    dst = VTC / DIR
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(DIST, dst)


def main():
    if not (DIST / 'nvim/bin/nvim').exists():
        print('make dist first')
        return 2
    install()
    ixpty_rig.use_ixemul()
    if run('Assign >NIL: PTY: EXISTS DEVICES')[0] != 0:
        run('Mount PTY: FROM VTC:ptymount')
    run('Delete RAM:nv#? QUIET')

    # 1. libuv backend: every check prints ok/FAIL, PASS at the end
    rc, out = run('VTC:%s/uvsmoke' % DIR, 120)
    print(out)
    check(rc == 0 and 'PASS: 0 failed' in out, 'uvsmoke (libuv backend) passes', out[-400:])

    if V012:
        # 2. libamigacompat's atomics and clocks (0.12's libuv and tree-sitter use them)
        rc, out = run('VTC:%s/compat_probe' % DIR, 60)
        print(out)
        check(rc == 0 and 'PASS: 0 failed' in out, 'compat_probe passes', out[-300:])
    else:
        # 2. PUC Lua 5.1 on the 68020, the math libamigacompat adds
        #    (no * in the command: the AmigaDOS shell's escape character)
        rc, out = run('VTC:nvim-test/lua51 -e "print(_VERSION, math.atan2(1, 0), '
                      'string.format(\'%5.2f\', 3.14159), 2^53)"', 60)
        check(rc == 0 and 'Lua 5.1' in out and '1.5707' in out and '3.14' in out,
              'lua51 runs', out)

    # 3. the binary loads and starts (stack cookie, ixemul vectors)
    rc, out = run(NV + ' --version', 120)
    check(rc == 0 and ('NVIM v0.12.5' if V012 else 'NVIM v0.4.4') in out, 'nvim --version', out)

    # 4. headless: the editor, its runtime found from the binary's path,
    #    Lua and vim.loop, a file written ($VIMRUNTIME spelled through eval:
    #    the AmigaDOS shell would expand a $VAR in the command line)
    t0 = time.time()
    # ixemul's argv parsing honours a quote only at the start of a word:
    # +"call f(a, b)" splits at the spaces, "+call f(a, b)" stays whole
    rc, out = run(NV + ' -u NONE -i NONE --headless '
                  '"+call writefile([string(1 + 1), eval(\'$\' . \'VIMRUNTIME\'), string(luaeval(\'(vim.uv or vim.loop).hrtime() > 0\'))], \'RAM:nvh.txt\')" '
                  '+qa!', 300)
    secs = time.time() - t0
    got = run('Type RAM:nvh.txt')[1]
    check('2' in got.splitlines()[:1] and 'runtime' in got and 'true' in got,
          'nvim --headless writes a file (%.1f s)' % secs, out + ' | ' + got)

    # 5. a shell command through uv_spawn (vfork + exec, socketpair stdio)
    rc, out = run(NV + ' -u NONE -i NONE --headless "--cmd" "set shell=/VTC/vsh" '
                  '"+call writefile(systemlist(\'echo spawned-ok\'), \'RAM:nvs.txt\')" +qa!', 300)
    got = run('Type RAM:nvs.txt')[1]
    check('spawned-ok' in got, 'system() runs a shell command', out + ' | ' + got)

    # 6. startup time with the runtime's defaults (filetype, syntax off)
    t0 = time.time()
    rc, out = run(NV + ' -i NONE --headless +qa!', 300)
    check(rc == 0, 'nvim --headless +qa! with defaults (%.1f s)' % (time.time() - t0), out)

    # 7. the TUI in an UP-Term window: type, save, quit
    if '--tui' in sys.argv:
        ami.req(0x02, struct.pack('>H', 10) + b'run >NIL: newshell "XCON:0/20/780/560/nvim/CLOSE"')
        time.sleep(4)
        screen_rig.typeline('VTC:vsh', 3)
        # the kit (and its vtcon terminfo) is not installed on the rig: the
        # engine's xterm personality with Neovim's own xterm-256color entry
        screen_rig.typeline('export TERM=xterm-256color', 2)
        before = avail()
        screen_rig.typeline(NV + ' -u NONE -i NONE', 90 if V012 else 60)
        during = avail()
        SHOT.parent.mkdir(parents=True, exist_ok=True)
        ami.main(['shot', str(SHOT)])
        print('AvailMem before %d, with nvim running %d: nvim uses %d bytes'
              % (before, during, before - during))
        if V012:
            # :help is highlighted by the statically linked vimdoc parser
            screen_rig.typeline(':help', 30)
            # no brackets in typed text: amiagent types [ as (
            screen_rig.typeline(":lua local f = io.open('RAM:nvts.txt', 'w') f:write(tostring("
                                "vim.treesitter.language.add('vimdoc'))) f:close()", 10)
            screen_rig.typeline(':q', 3)  # close the help window
            got = run('Type RAM:nvts.txt')[1]
            check('true' in got, 'the vimdoc parser loads (static uv_dlopen)', got)
        screen_rig.typeline('ihello amiga', 3)
        ami.key(0x45)  # Esc
        time.sleep(2)
        screen_rig.typeline(':w RAM:nvtui.txt', 5)
        screen_rig.typeline(':q', 10)
        got = run('Type RAM:nvtui.txt')[1]
        check(got.strip() == 'hello amiga', 'the TUI: typed text saved, nvim quit', got)
        if '--keep' not in sys.argv:
            screen_rig.typeline('exit', 2)
            screen_rig.typeline('endcli', 2)
        print('screenshot of the TUI at start:', SHOT)
        print('AvailMem after nvim quit: %d' % avail())

    print('nvim_rig: passed %d of %d' % (install_rig.passed, install_rig.total))
    return 0 if install_rig.passed == install_rig.total else 1


if __name__ == '__main__':
    sys.exit(main())
