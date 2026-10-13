#!/usr/bin/env python3
"""nvim's TUI on a terminal that wraps the moment the last column is written
(UP-Term's AMIGA personality, TERM=vtcon-amiga: terminfo am, no xenl).

nvim used to take the wrap rule from the TERM name, so it wrote every row full
width plus CR LF, and on such a terminal each row moved two rows: alternate rows
kept the window's background, typing landed on the wrong rows (W-N1). It also
scrolled the screen when it wrote the bottom right cell.

Runs the host nvim through the vtcon engine (tests/tui_vtpipe.c over a pty) and
reads the engine's screen. Prints "ok <name>" / "FAIL <name>: why", exit 0 when
all pass.

  tests/tui_eager_wrap.py [nvim]
  env: VTCON=<vtcon checkout> (default ../vtcon, needs `make build/kit-terminfo/stamp` there)
"""
import fcntl, os, pty, select, struct, subprocess, sys, tempfile, termios, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VTCON = os.path.abspath(os.environ.get('VTCON', os.path.join(ROOT, '..', 'vtcon')))
NVIM = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(ROOT, 'build/v012/host/nvim/bin/nvim')
RUNTIME = os.path.join(os.path.dirname(os.path.dirname(NVIM)), 'runtime')
COLS, ROWS = 80, 20
BLUE = ['-c', 'set notermguicolors', '-c', 'hi Normal ctermfg=7 ctermbg=4',
        '-c', 'hi StatusLine ctermfg=7 ctermbg=4', '-c', 'hi StatusLineNC ctermfg=7 ctermbg=4']
fails = 0


def check(ok, name, why=''):
    global fails
    print(('ok %s' % name) if ok else ('FAIL %s: %s' % (name, why)))
    if not ok:
        fails += 1


def build_pipe(tmp):
    exe = os.path.join(tmp, 'tui_vtpipe')
    subprocess.run(['cc', '-std=c89', '-O1', '-Wno-long-long', '-I', os.path.join(VTCON, 'engine'), '-o', exe,
                    os.path.join(ROOT, 'tests/tui_vtpipe.c'), os.path.join(VTCON, 'engine/vtengine.c')], check=True)
    return exe


def run(pipe, tmp, name, args, keys=b''):
    """nvim under vtcon-amiga on the engine; returns (text rows, background rows)."""
    out = os.path.join(tmp, name + '.dump')
    h = subprocess.Popen([pipe, str(COLS), str(ROWS), 'amiga', out], stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE, bufsize=0)
    home = os.path.join(tmp, name + '.home')
    os.makedirs(home)
    pid, fd = pty.fork()
    if pid == 0:
        env = {'TERM': 'vtcon-amiga', 'TERMINFO': os.path.join(VTCON, 'build/kit-terminfo'),
               'VIMRUNTIME': RUNTIME, 'HOME': home, 'XDG_CONFIG_HOME': home, 'XDG_STATE_HOME': home,
               'XDG_DATA_HOME': home, 'PATH': '/usr/bin:/bin'}
        os.chdir(tmp)
        os.execve(NVIM, [NVIM, '-u', 'NONE', '-i', 'NONE', '--noplugin'] + args, env)
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack('HHHH', ROWS, COLS, 0, 0))
    sent = False
    start = last = time.time()
    while time.time() - start < 15:
        r, _, _ = select.select([fd, h.stdout], [], [], 0.1)
        if fd in r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            h.stdin.write(d)
            last = time.time()
        if h.stdout in r:
            os.write(fd, os.read(h.stdout.fileno(), 4096))
        if time.time() - last > 0.7:  # quiet: drawn
            if keys and not sent:
                os.write(fd, keys)
                sent = True
                last = time.time()
            else:
                break
    os.kill(pid, 9)
    os.waitpid(pid, 0)
    h.stdin.close()
    h.wait()
    lines = open(out).read().split('\n')
    return lines[:ROWS], [l.split() for l in lines[ROWS:2 * ROWS]]


def main():
    ti = os.path.join(VTCON, 'build/kit-terminfo/v/vtcon-amiga')
    if not os.path.exists(ti):
        sys.exit('no %s: cd %s && make build/kit-terminfo/stamp' % (ti, VTCON))
    tmp = tempfile.mkdtemp()
    pipe = build_pipe(tmp)

    # an empty buffer, typed into: hello on row 1, the status line on its row,
    # every row in nvim's background (no alternate rows left in the window's)
    text, bg = run(pipe, tmp, 'empty', BLUE + ['-c', 'set laststatus=2 noruler'], b'ihello\x1b')
    row0 = [i for i, l in enumerate(text) if 'hello' in l]
    check(row0 == [0], 'hello is on row 1', 'rows with hello: %s' % row0)
    st = [i for i, l in enumerate(text) if '[No Name]' in l]
    check(st == [ROWS - 2], 'status line is on the second last row', 'rows with [No Name]: %s' % st)
    bad = [i for i, r in enumerate(bg) if set(r) != {'4'} and i != ROWS - 1]
    check(not bad and bg[0][0] == '4', 'every row has nvim background (no alternating rows)',
          'rows not all blue: %s, bgs of row 0..3: %s' % (bad, [sorted(set(r)) for r in bg[:4]]))

    # a screenful of full-width lines: the bottom right cell is written and
    # the screen must not scroll
    full = os.path.join(tmp, 'full.txt')
    with open(full, 'w') as f:
        for i in range(ROWS):
            f.write(chr(65 + i) * COLS + '\n')
    text, bg = run(pipe, tmp, 'full', BLUE + ['-c', 'set cmdheight=0 laststatus=0 nonumber', full])
    want = [chr(65 + i) * COLS for i in range(ROWS)]
    check(text == want, 'full-width lines, bottom right cell written, screen does not scroll',
          'first rows: %s ... last: %r' % (text[:2], text[-1]))


main()
sys.exit(1 if fails else 0)
