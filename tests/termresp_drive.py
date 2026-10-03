#!/usr/bin/env python3
"""Start nvim in a pty that answers like a terminal (OSC 11 background, DSR,
DA1), with $NVIM_TERMTRACE on, and report:
  - whether E1568 ("Terminal did not respond to DSR ...") appeared,
  - the trace: when the query was queued, written, answered, read, handed to
    the editor, and how vim.wait ended.
Then types text and saves it, to show the TUI takes keys after startup.

  tests/termresp_drive.py [nvim] [--delay MS] [-- nvim args...]
    --delay MS   the terminal answers MS milliseconds after the query
                 (to see where the 100 ms budget breaks)
Env: TUI_RUNTIME (VIMRUNTIME), NVIM_TUI_INPROC (1 one process, 0 two).
Exit 0 when there was no E1568 and the text was saved."""
import fcntl, os, pty, re, select, struct, sys, tempfile, termios, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]
nvim = os.path.join(ROOT, 'build/v012/host/nvim/bin/nvim')
if args and not args[0].startswith('-'):
    nvim = os.path.abspath(args.pop(0))
delay = 0.0
if '--delay' in args:
    i = args.index('--delay'); delay = float(args[i + 1]) / 1000; del args[i:i + 2]
extra = args[args.index('--') + 1:] if '--' in args else ['--clean']

tmp = tempfile.mkdtemp()
trace = os.path.join(tmp, 'termtrace.txt')
env = dict(os.environ, TERM='xterm-256color', HOME=tmp, XDG_CONFIG_HOME=tmp,
           XDG_DATA_HOME=tmp, XDG_STATE_HOME=tmp, NVIM_TERMTRACE=trace)
if 'TUI_RUNTIME' in os.environ:
    env['VIMRUNTIME'] = os.environ['TUI_RUNTIME']

pid, fd = pty.fork()
if pid == 0:
    os.chdir(tmp)
    os.execve(nvim, [nvim, '-i', 'NONE'] + extra, env)
fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack('HHHH', 24, 80, 0, 0))

screen = b''
pending = []   # (due time, bytes)

def pump(secs):
    global screen
    end = time.time() + secs
    while time.time() < end:
        now = time.time()
        for item in list(pending):
            if item[0] <= now:
                os.write(fd, item[1]); pending.remove(item)
        r, _, _ = select.select([fd], [], [], 0.005)
        if not r:
            continue
        try:
            d = os.read(fd, 65536)
        except OSError:
            return False
        if not d:
            return False
        # answer as a terminal would, in order of the queries
        for m in re.finditer(rb'\x1b\]11;\?(\x07|\x1b\\)|\x1b\[5n|\x1b\[c|\x1b\[0c', d):
            q = m.group(0)
            if q.startswith(b'\x1b]11'):
                a = b'\x1b]11;rgb:0000/0000/0000\x1b\\'
            elif q == b'\x1b[5n':
                a = b'\x1b[0n'
            else:
                a = b'\x1b[?62;22c'
            pending.append((time.time() + delay, a))
        screen += d
    return True

pump(3.0)
e1568 = b'E1568' in screen
os.write(fd, b'ihello termresp\x1b')
pump(0.5)
os.write(fd, b':w out.txt\r')
pump(0.5)
os.write(fd, b':qa!\r')
pump(1.0)
try:
    p, st = os.waitpid(pid, os.WNOHANG)
    if not p:
        os.kill(pid, 9); os.waitpid(pid, 0)
except ChildProcessError:
    pass
saved = os.path.exists(os.path.join(tmp, 'out.txt')) and \
    open(os.path.join(tmp, 'out.txt')).read() == 'hello termresp\n'
print('E1568 shown: %s; typed text saved: %s; answer delay %d ms'
      % (e1568, saved, delay * 1000))
print('--- trace')
print(open(trace).read() if os.path.exists(trace) else '(no trace)')
sys.exit(0 if (not e1568 and saved) else 1)
