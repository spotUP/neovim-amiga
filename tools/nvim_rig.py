#!/usr/bin/env python3
"""nvim_rig.py -- Neovim 0.4.4 (neovim-amiga) on the UP-Term FS-UAE rig.

Run by whoever drives the rig (the vtcon main session), never by the port's
agent. It reuses vtcon's rig tools read-only (vtcon/tools/rig in the workspace: ami,
install_rig.run, ixpty_rig.use_ixemul, screen_rig.typeline) and COPIES
build/m68k/dist (nvim tree, uvsmoke, lua51) to VTC:nvim-test/ -- that is,
into vtcon/build/rig/vtc/nvim-test.

Needs: the rig up (python3 tools/rig/rig.py start in vtcon), PTY:
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

UPTERM_ROOT = pathlib.Path(os.environ.get('UPTERM_ROOT') or pathlib.Path(__file__).resolve().parents[2])
VTCON = pathlib.Path(os.environ.get('VTCON', UPTERM_ROOT / 'vtcon'))
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


RIG_VIM = r"""" nvim_rig.py: sourced with -S; VimEnter fires once startup is
" over (screen drawn, input taken), VimLeave when nvim really quits
autocmd VimEnter * call writefile(['ready', 'v:termresponse=' . strtrans(v:termresponse)]
      \ + split(execute('messages'), "\n"), 'RAM:nvready.txt')
autocmd VimLeave * call writefile(['bye'], 'RAM:nvbye.txt')
"""

RIG2_VIM = r"""" nvim_rig.py: the parser check, sourced from the typed :source line
lua << END
local f = io.open('RAM:nvts.txt', 'w')
local function add() return vim.treesitter.language.add('vimdoc') end
local ok, r = pcall(add)
local hok, herr = pcall(vim.cmd, 'help')
f:write(string.format('add=%s %s\nhelp=%s %s ft=%s\n', tostring(ok), tostring(r),
  tostring(hok), tostring(herr), vim.bo.filetype))
-- how runtime Lua modules are found (vim._load_package), step by step
local rt = vim.env.VIMRUNTIME or '?'
local mod = rt .. '/lua/vim/treesitter.lua'
local function try(what, fn)
  local fok, v = pcall(fn)
  f:write(what .. ' = ' .. (fok and vim.inspect(v) or ('ERROR ' .. tostring(v))) .. '\n')
end
try('VIMRUNTIME', function() return rt end)
try('rtp', function() return vim.o.runtimepath end)
try('get_runtime', function()
  return vim.api.nvim__get_runtime({ 'lua/vim/treesitter.lua' }, false, { is_lua = true })
end)
try('get_runtime_file', function() return vim.api.nvim_get_runtime_file('lua/vim/treesitter.lua', false) end)
try('isdirectory(rt/lua)', function() return vim.fn.isdirectory(rt .. '/lua') end)
try('isdirectory(rt/lua/)', function() return vim.fn.isdirectory(rt .. '/lua/') end)
try('filereadable(mod)', function() return vim.fn.filereadable(mod) end)
try('fs_access(mod)', function() return { vim.uv.fs_access(mod, 'R') } end)
try('fs_stat(mod).mode', function() local st = vim.uv.fs_stat(mod) return st and st.mode end)
try('fs_lstat(rt).type', function() local st = vim.uv.fs_lstat(rt) return st and st.type end)
try('fs_lstat(rt/lua).type', function() local st = vim.uv.fs_lstat(rt .. '/lua') return st and st.type end)
try('fs_stat(rt/lua/).type', function() local st = vim.uv.fs_stat(rt .. '/lua/') return st and st.type end)
try('glob(rt)', function() return vim.fn.glob(rt, true, true) end)
try('runtime_inspect', function() return vim.api.nvim__runtime_inspect() end)
f:close()
END
helpclose
call writefile(['messages:'] + split(execute('messages'), "\n"), 'RAM:nvmsg.txt')
"""


def poll(path, done, secs):
    """the text of the Amiga file path once done(text), or what it held at
    the end of secs ('' when it never appeared)"""
    t0, got = time.time(), ''
    while time.time() - t0 < secs:
        got = run('Type %s' % path)[1]
        if done(got):
            break
        time.sleep(3)
    return got


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
                  '"+call writefile([string(1 + 1), eval(\'$\' . \'VIMRUNTIME\'), string(luaeval(\'(vim.uv or vim.loop).hrtime() > 4294967296\'))], \'RAM:nvh.txt\')" '
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

    # 7. the TUI in an UP-Term window. Nothing is typed until nvim says it
    #    is ready (rig.vim, sourced with -S, writes RAM:nvready.txt on
    #    VimEnter with v:termresponse and the messages so far, and
    #    RAM:nvbye.txt on VimLeave);
    #    the parser check is a sourced file (no brackets to type: amiagent
    #    types [ as ( ); nvim's terminal round trips are traced to
    #    RAM:nvtrace.txt ($NVIM_TERMTRACE, 0.12 only).
    if '--tui' in sys.argv:
        (VTC / DIR / 'rig.vim').write_text(RIG_VIM)
        (VTC / DIR / 'rig2.vim').write_text(RIG2_VIM)
        run('Delete RAM:nvready.txt RAM:nvts.txt RAM:nvmsg.txt RAM:nvtrace.txt RAM:nvtui.txt '
            'RAM:nvbye.txt QUIET')
        ami.req(0x02, struct.pack('>H', 10) + b'run >NIL: newshell "XCON:0/20/780/560/nvim/CLOSE"')
        time.sleep(4)
        screen_rig.typeline('VTC:vsh', 3)
        # the kit (and its vtcon terminfo) is not installed on the rig: the
        # engine's xterm personality with Neovim's own xterm-256color entry
        screen_rig.typeline('export TERM=xterm-256color', 2)
        if V012:
            screen_rig.typeline('export NVIM_TERMTRACE=RAM:nvtrace.txt', 2)
        before = avail()
        t0 = time.time()
        screen_rig.typeline(NV + ' -u NONE -i NONE -S VTC:%s/rig.vim' % DIR, 1)
        ready = poll('RAM:nvready.txt', lambda t: t.startswith('ready'), 300)
        check(ready.startswith('ready'), 'the TUI starts (%.0f s to ready)' % (time.time() - t0),
              ready or '(no RAM:nvready.txt after 300 s)')
        print('--- at ready:\n' + ready)
        during = avail()
        SHOT.parent.mkdir(parents=True, exist_ok=True)
        ami.main(['shot', str(SHOT)])
        print('AvailMem before %d, with nvim running %d: nvim uses %d bytes'
              % (before, during, before - during))
        if V012:
            screen_rig.typeline(':source VTC:%s/rig2.vim' % DIR, 1)
            # rig2.vim writes RAM:nvts.txt, closes the help, then writes
            # RAM:nvmsg.txt: the second file means nvim is back in Normal mode
            msgs = poll('RAM:nvmsg.txt', lambda t: t.startswith('messages:'), 300)
            got = run('Type RAM:nvts.txt')[1]
            print('--- parser check (RAM:nvts.txt):\n' + got)
            print('--- messages (RAM:nvmsg.txt):\n' + msgs)
            check(got.startswith('add=true true'), 'the vimdoc parser loads (static uv_dlopen)',
                  got or '(no RAM:nvts.txt)')
        # typed while nvim is known to be idle in Normal mode (VimEnter, and
        # for 0.12 the parser check, have written their files); the result
        # files are polled, not read after a fixed sleep
        screen_rig.typeline('ihello amiga', 3)
        ami.key(0x45)  # Esc
        time.sleep(2)
        screen_rig.typeline(':w RAM:nvtui.txt', 1)
        got = poll('RAM:nvtui.txt', lambda t: 'hello amiga' in t, 120)
        screen_rig.typeline(':q', 1)
        bye = poll('RAM:nvbye.txt', lambda t: 'bye' in t, 120)
        check(got.strip() == 'hello amiga' and 'bye' in bye,
              'the TUI: typed text saved, nvim quit',
              'RAM:nvtui.txt: %r; RAM:nvbye.txt (VimLeave): %r' % (got, bye))
        if V012:
            print('--- RAM:nvtrace.txt (NVIM_TERMTRACE):\n' + run('Type RAM:nvtrace.txt')[1])
        if '--keep' not in sys.argv:
            screen_rig.typeline('exit', 2)
            screen_rig.typeline('endcli', 2)
        print('screenshot of the TUI at ready:', SHOT)
        print('AvailMem after nvim quit: %d' % avail())

    print('nvim_rig: passed %d of %d' % (install_rig.passed, install_rig.total))
    return 0 if install_rig.passed == install_rig.total else 1


if __name__ == '__main__':
    sys.exit(main())
