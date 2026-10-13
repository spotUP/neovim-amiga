#!/usr/bin/env python3
"""nvim_startup_rig.py -- the two startup errors of the kit nvim, on the rig.

  W-N3  "E116: Invalid arguments for function stridx" from plugin/matchit.vim
        line 2 (stridx(&packpath, $VIMRUNTIME)): an environment variable in an
        expression took the character after its name with it (bebbo gcc 6.5
        -O3 ivopts, fixed by -fno-ivopts in amiga/cmake/m68k-amigaos-ixemul012.cmake).
  W-N2  "E303: Unable to open swap file for RAM:b4.txt": a "Volume:path" name was
        made absolute as /BOOTX/RAM:b4.txt, and a ':' inside a name is a volume
        separator for ixemul (vendor012 src/nvim/path.c path_to_absolute).

Drives the binary headless on the rig (UPTERM_RIG selects it, as for vtcon's
tools/rig/ami.py; the rig's vtcon checkout is found like nvim_rig.py does):

  UPTERM_RIG=3 python3 tests/nvim_startup_rig.py [UP-Term:nvim/bin/nvim.n3]

One headless start with a file argument, a script sourced from it (by the
/Ram Disk/T/ spelling: that works on any build), the answers written to a file
and read back. Exit 0 when every check passes. Run it on the old binary and
the checks named above FAIL.
"""
import os, pathlib, sys, tempfile

UPTERM_ROOT = pathlib.Path(os.environ.get('UPTERM_ROOT') or pathlib.Path(__file__).resolve().parents[2])
VTCON = pathlib.Path(os.environ.get('VTCON', UPTERM_ROOT / 'vtcon'))
sys.path.insert(0, str(VTCON / 'tools/rig'))
import ami   # noqa: E402

NV = sys.argv[1] if len(sys.argv) > 1 else 'UP-Term:nvim/bin/nvim'

SCRIPT = r"""
let r = []
function! Ev(e) abort
  try
    return string(eval(a:e))
  catch
    return 'ERR ' . v:exception
  endtry
endfunction
" W-N3: the environment variable ends at its name
call add(r, 'envend=' . Ev("'[' . $VIMRUNTIME . ']'"))
call add(r, 'stridx=' . Ev("stridx(&packpath, $VIMRUNTIME) >= 0"))
call add(r, 'unset=' . Ev("stridx('abc', $N3_NOT_SET)"))
call add(r, 'alone=' . Ev("type($HOME)"))
call add(r, 'matchit=' . exists('g:loaded_matchit'))
" W-N2: Volume:path is /Volume/path, and has a swap file
call add(r, 'full=' . Ev("fnamemodify('RAM:b4.txt', ':p')"))
call add(r, 'fullT=' . Ev("fnamemodify('T:x/y', ':p')"))
call add(r, 'buf=' . expand('%:p'))
call add(r, 'swap=' . swapname('%'))
call add(r, 'messages=' . join(split(execute('messages'), "\n"), ' | '))
call writefile(r, '/Ram Disk/T/n3_startup.txt')
"""

def sh(cmd, secs=300):
    return ami.req(0x02, __import__('struct').pack('>H', secs) + cmd.encode('latin-1'), secs + 30)

def main():
    ok = True
    def check(name, cond, got):
        nonlocal ok
        print('%s  %s  (%s)' % ('ok  ' if cond else 'FAIL', name, got))
        ok = ok and cond
    with tempfile.NamedTemporaryFile('w', suffix='.vim', delete=False) as t:
        t.write(SCRIPT)
    ami.put(t.name, 'T:n3_startup.vim')
    os.unlink(t.name)
    sh('Delete >NIL: RAM:b4.txt T:n3_startup.txt QUIET')
    sh('Echo >RAM:b4.txt hello')
    sh(NV + ' --headless RAM:b4.txt -c "source /Ram\\ Disk/T/n3_startup.vim" -c qa!')
    out = ami.read_file('T:n3_startup.txt')
    if out is None:
        print('FAIL  no answer file: nvim did not run the script')
        return 1
    got = dict(l.split('=', 1) for l in out.decode('latin-1').splitlines() if '=' in l)
    msgs = got.get('messages', '')
    check('startup prints no E116 (W-N3)', 'E116' not in msgs, msgs[:200])
    check('$VIMRUNTIME ends at its name', got['envend'].endswith("runtime]'"), got['envend'])
    check('stridx(&packpath, $VIMRUNTIME) evaluates (W-N3)', got['stridx'] == '1', got['stridx'])
    check('an unset variable as the last argument', got['unset'] == '0', got['unset'])
    check('$HOME alone is a string', got['alone'] == '1', got['alone'])
    check('matchit loaded', got['matchit'] == '1', got['matchit'])
    check('RAM:b4.txt is /RAM/b4.txt (W-N2)', got['full'] == "'/RAM/b4.txt'", got['full'])
    check('T:x/y is /T/x/y', got['fullT'] == "'/T/x/y'", got['fullT'])
    check('the file argument is /RAM/b4.txt', got['buf'] == '/RAM/b4.txt', got['buf'])
    check('the buffer has a swap file (W-N2)', got['swap'].endswith('.swp'), got['swap'])
    check('startup prints no E303 (W-N2)', 'E303' not in msgs, msgs[:200])
    print('PASS' if ok else 'FAILED')
    return 0 if ok else 1

sys.exit(main())
