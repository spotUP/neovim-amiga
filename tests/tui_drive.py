#!/usr/bin/env python3
"""Drive an nvim binary's built-in TUI through a pty, the way a user types.

Self-checking: types into nvim, saves through :w, and checks the file, the
exit status, and that the TUI drew (cursor addressing / the alternate
screen). Used on the host against build/host/nvim (the UV_NO_THREADS build:
TUI on the main loop). Prints "ok <name>" / "FAIL <name>: why", exits 0 when
all pass.

  tests/tui_drive.py [path/to/nvim]     (TUI_RUNTIME=<runtime dir> for 0.12.5)
"""
import os, pty, select, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NVIM = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(ROOT, 'build/host/nvim/bin/nvim')  # the test chdirs
fails = 0


def check(ok, name, why=''):
    global fails
    print(('ok %s' % name) if ok else ('FAIL %s: %s' % (name, why)))
    if not ok:
        fails += 1


def read_for(fd, secs):
    out = b''
    end = time.time() + secs
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                d = os.read(fd, 65536)
            except OSError:
                break
            if not d:
                break
            out += d
    return out


def session(keys_steps, env_extra=None, args=()):
    tmp = tempfile.mkdtemp()
    env = dict(os.environ, TERM='xterm-256color',
               VIMRUNTIME=os.environ.get('TUI_RUNTIME', os.path.join(ROOT, 'vendor/neovim/runtime')),
               XDG_DATA_HOME=tmp, XDG_CONFIG_HOME=tmp, HOME=tmp)
    env.update(env_extra or {})
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(tmp)
        os.execve(NVIM, [NVIM, '-u', 'NONE', '-i', 'NONE'] + list(args), env)
    import fcntl, struct, termios
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack('HHHH', 24, 80, 0, 0))
    out = read_for(fd, 1.5)
    for keys, wait in keys_steps:
        if callable(keys):
            keys(fd, pid)
        else:
            os.write(fd, keys)
        out += read_for(fd, wait)
    deadline = time.time() + 5
    status = None
    while time.time() < deadline:
        p, st = os.waitpid(pid, os.WNOHANG)
        if p:
            status = st
            break
        out += read_for(fd, 0.1)
    if status is None:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
    return tmp, out, status


# 1. type text, save, quit
tmp, out, st = session([(b'ihello amiga\x1b', 0.5), (b':w out.txt\r', 0.5), (b':q\r', 0.5)])
check(st is not None and os.WIFEXITED(st) and os.WEXITSTATUS(st) == 0,
      'tui-quits', 'status %r' % (st,))
try:
    data = open(os.path.join(tmp, 'out.txt')).read()
except OSError as e:
    data = repr(e)
check(data == 'hello amiga\n', 'tui-typed-text-saved', repr(data))
check(b'\x1b[?1049h' in out, 'tui-alt-screen', 'no smcup in output')
check(b'hello amiga' in out, 'tui-echoed-text', 'typed text never drawn')

# 1b. the process model: NVIM_TUI_INPROC=1 -> no `nvim --embed` child (the
#     TUI and the editor in one process); =0 -> exactly one such child
mode = os.environ.get('NVIM_TUI_INPROC')
if mode in ('0', '1'):
    import subprocess
    seen = {}
    def count_children(fd, pid):
        out = subprocess.run(['pgrep', '-P', str(pid)], capture_output=True, text=True).stdout
        kids = [k for k in out.split() if k]
        embeds = 0
        for k in kids:
            cmd = subprocess.run(['ps', '-o', 'command=', '-p', k], capture_output=True, text=True).stdout
            embeds += '--embed' in cmd
        seen['embeds'] = embeds
    tmp, out, st = session([(count_children, 0.2), (b':q\r', 0.5)])
    want = 0 if mode == '1' else 1
    check(seen.get('embeds') == want, 'tui-process-model',
          '%r --embed children, want %d' % (seen.get('embeds'), want))

# 2. :! runs a shell command (uv_spawn) and the editor keeps going
tmp, out, st = session([(b':r !echo spawned-ok\r', 1.5), (b':w out.txt\r', 0.5), (b':q!\r', 0.5)])
try:
    data = open(os.path.join(tmp, 'out.txt')).read()
except OSError as e:
    data = repr(e)
check('spawned-ok' in data, 'tui-shell-read', repr(data))

# 3. a resize reaches the editor (SIGWINCH watcher on the main loop)
def resize(fd, pid):
    import fcntl, signal, struct, termios
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack('HHHH', 30, 100, 0, 0))
    os.kill(pid, signal.SIGWINCH)

tmp, out, st = session([(resize, 1.0), (b':put =&columns . \'x\' . &lines\r', 0.5),
                        (b':w out.txt\r', 0.5), (b':q!\r', 0.5)])
try:
    data = open(os.path.join(tmp, 'out.txt')).read()
except OSError as e:
    data = repr(e)
check('100x30' in data, 'tui-resize', repr(data))

# 4. a long paste (bracketed) arrives whole
blob = b'x' * 3000
tmp, out, st = session([(b'i', 0.2), (b'\x1b[200~' + blob + b'\x1b[201~', 2.0), (b'\x1b:w out.txt\r', 0.5), (b':q!\r', 0.5)])
try:
    data = open(os.path.join(tmp, 'out.txt')).read()
except OSError as e:
    data = repr(e)
check(data.strip() == 'x' * 3000, 'tui-paste-3000', 'got %d chars' % len(data.strip()))

# 5. many typed keys at once (not a paste): the key buffer back-pressure
keys = b'i' + b'abcdefgh' * 1200 + b'\x1b'
tmp, out, st = session([(keys, 3.0), (b':w out.txt\r', 0.5), (b':q!\r', 0.5)])
try:
    data = open(os.path.join(tmp, 'out.txt')).read()
except OSError as e:
    data = repr(e)
check(data.strip() == 'abcdefgh' * 1200, 'tui-typeahead-9600', 'got %d chars' % len(data.strip()))

print('%s: %d failed' % ('FAIL' if fails else 'PASS', fails))
sys.exit(1 if fails else 0)
