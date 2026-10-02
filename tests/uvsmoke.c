/* uvsmoke: the libuv features Neovim 0.4.4 uses, one self-checking run.
 * Prints one line per check ("ok <name>" / "FAIL <name>: why") and exits 0
 * only when every check passed. Built for the Amiga (build/m68k/uvsmoke)
 * and for the host (build/host/uvsmoke) from the same source.
 *
 * Checks: hrtime moves; a timer fires after its timeout and repeats;
 * idle/prepare/check run each iteration; a pipe pair carries bytes through
 * uv_pipe_open + uv_read_start/uv_write; uv_async_send wakes the loop; a
 * signal handler runs (raise SIGUSR1); uv_queue_work runs work then
 * after_work, not inside the submit; an async uv_fs_open/read; uv_spawn
 * runs a child with a stdout pipe and its exit callback sees status 0;
 * a spawn of a missing program fails with UV_ENOENT; uv_exepath, uv_cwd,
 * uv_os_homedir answer; uv_tty_init on fd 1 when it is
 * a terminal. */
#include <uv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int fails;
static uv_loop_t *loop;

static void check(int ok, const char *name, const char *why)
{
	if (ok)
		printf("ok %s\n", name);
	else {
		printf("FAIL %s: %s\n", name, why);
		fails++;
	}
	fflush(stdout);
}

/* ---- timer, idle, prepare, check */
static uv_timer_t tm;
static int tm_count;
static uint64_t tm_start;
static uv_idle_t idle;
static uv_prepare_t prep;
static uv_check_t chk;
static int n_idle, n_prep, n_chk;

static void on_idle(uv_idle_t *h) { n_idle++; if (n_idle >= 3) uv_close((uv_handle_t *)h, NULL); }
static void on_prep(uv_prepare_t *h) { (void)h; n_prep++; }
static void on_chk(uv_check_t *h) { (void)h; n_chk++; }

static void on_timer(uv_timer_t *h)
{
	if (++tm_count == 1) {
		uint64_t el = uv_now(loop) - tm_start;
		char why[64];
		snprintf(why, sizeof why, "fired after %lu ms", (unsigned long)el);
		check(el >= 50, "timer-timeout", why);
	}
	if (tm_count == 3) {
		uv_close((uv_handle_t *)h, NULL);
		check(1, "timer-repeat", "");
		uv_close((uv_handle_t *)&prep, NULL);
		uv_close((uv_handle_t *)&chk, NULL);
	}
}

/* ---- pipe pair */
static uv_pipe_t pr, pw;
static uv_write_t wreq;
static char rbuf[64];
static char got[64];
static size_t got_len;

static void alloc_cb(uv_handle_t *h, size_t s, uv_buf_t *b)
{
	(void)h; (void)s;
	b->base = rbuf;
	b->len = sizeof rbuf;
}

static void on_read(uv_stream_t *s, ssize_t n, const uv_buf_t *b)
{
	if (n > 0 && got_len + n < sizeof got) {
		memcpy(got + got_len, b->base, n);
		got_len += n;
	}
	if (n == UV_EOF || got_len >= 5) {
		got[got_len] = 0;
		check(strcmp(got, "hello") == 0, "pipe-read", got);
		uv_read_stop(s);
		uv_close((uv_handle_t *)s, NULL);
	}
}

static void on_write(uv_write_t *r, int status)
{
	check(status == 0, "pipe-write", uv_strerror(status));
	uv_close((uv_handle_t *)r->handle, NULL);
}

/* ---- async */
static uv_async_t as;
static void on_async(uv_async_t *h)
{
	check(1, "async-wakes", "");
	uv_close((uv_handle_t *)h, NULL);
}

/* ---- signal */
static uv_signal_t sg;
static void on_signal(uv_signal_t *h, int signum)
{
	check(signum == SIGUSR1, "signal", "wrong signal number");
	uv_close((uv_handle_t *)h, NULL);
}

/* ---- work queue */
static uv_work_t work;
static int work_ran, submit_returned, after_ran;
static void work_cb(uv_work_t *w) { (void)w; work_ran = 1; check(submit_returned, "work-not-inside-submit", "work ran inside uv_queue_work"); }
static void after_cb(uv_work_t *w, int status)
{
	(void)w;
	after_ran = 1;
	check(work_ran && status == 0, "work-after", uv_strerror(status));
}

/* ---- fs */
static uv_fs_t fs_open_req, fs_read_req, fs_close_req;
static char fbuf[16];
static uv_buf_t fiov;
static void on_fs_read(uv_fs_t *r)
{
	check(r->result > 0, "fs-async-read", uv_strerror((int)r->result));
	uv_fs_req_cleanup(r);
	uv_fs_close(loop, &fs_close_req, (uv_file)fs_open_req.result, NULL);
	uv_fs_req_cleanup(&fs_close_req);
}
static void on_fs_open(uv_fs_t *r)
{
	check(r->result >= 0, "fs-async-open", uv_strerror((int)r->result));
	if (r->result >= 0) {
		fiov = uv_buf_init(fbuf, sizeof fbuf);
		uv_fs_read(loop, &fs_read_req, (uv_file)r->result, &fiov, 1, 0, on_fs_read);
	}
	uv_fs_req_cleanup(r);
}

/* ---- spawn */
static uv_process_t proc;
static uv_pipe_t child_out;
static char cbuf[128];
static size_t clen;
static int exit_seen;
static void child_alloc(uv_handle_t *h, size_t s, uv_buf_t *b)
{
	(void)h; (void)s;
	b->base = cbuf + clen;
	b->len = sizeof cbuf - 1 - clen;
}
static void child_read(uv_stream_t *s, ssize_t n, const uv_buf_t *b)
{
	(void)b;
	if (n > 0)
		clen += n;
	else if (n < 0) {
		cbuf[clen] = 0;
		check(strstr(cbuf, "child-said-hi") != NULL, "spawn-stdout", cbuf);
		uv_close((uv_handle_t *)s, NULL);
	}
}
static void on_exit_cb(uv_process_t *p, int64_t status, int sig)
{
	char why[64];
	snprintf(why, sizeof why, "status %ld signal %d", (long)status, sig);
	exit_seen = 1;
	check(status == 0 && sig == 0, "spawn-exit", why);
	uv_close((uv_handle_t *)p, NULL);
}

int main(int argc, char **argv)
{
	int fds[2], r;
	uint64_t t0;
	char path[1024];
	size_t n;
	const char *self = argv[0];

	if (argc > 1 && strcmp(argv[1], "child") == 0) {
		printf("child-said-hi\n");
		return 0;
	}
	loop = uv_default_loop();

	t0 = uv_hrtime();
	{ volatile int i; for (i = 0; i < 100000; i++) ; }
	check(uv_hrtime() > t0, "hrtime-moves", "uv_hrtime did not advance");

	n = sizeof path;
	r = uv_exepath(path, &n);
	check(r == 0, "exepath", r ? uv_strerror(r) : path);
	if (r == 0) printf("   exepath = %s\n", path);
	n = sizeof path;
	r = uv_cwd(path, &n);
	check(r == 0, "cwd", r ? uv_strerror(r) : path);
	if (r == 0) printf("   cwd = %s\n", path);
	n = sizeof path;
	r = uv_os_homedir(path, &n);
	check(r == 0, "homedir", r ? uv_strerror(r) : path);
	if (r == 0) printf("   homedir = %s\n", path);

	uv_timer_init(loop, &tm);
	tm_start = uv_now(loop);
	uv_timer_start(&tm, on_timer, 50, 20);
	uv_idle_init(loop, &idle);
	uv_idle_start(&idle, on_idle);
	uv_prepare_init(loop, &prep);
	uv_prepare_start(&prep, on_prep);
	uv_check_init(loop, &chk);
	uv_check_start(&chk, on_chk);

	if (pipe(fds) != 0) {
		check(0, "pipe", "pipe() failed");
	} else {
		uv_buf_t b = uv_buf_init("hello", 5);
		uv_pipe_init(loop, &pr, 0);
		uv_pipe_init(loop, &pw, 0);
		r = uv_pipe_open(&pr, fds[0]);
		check(r == 0, "pipe-open-read", uv_strerror(r));
		r = uv_pipe_open(&pw, fds[1]);
		check(r == 0, "pipe-open-write", uv_strerror(r));
		uv_read_start((uv_stream_t *)&pr, alloc_cb, on_read);
		uv_write(&wreq, (uv_stream_t *)&pw, &b, 1, on_write);
	}

	uv_async_init(loop, &as, on_async);
	uv_async_send(&as);

	uv_signal_init(loop, &sg);
	uv_signal_start(&sg, on_signal, SIGUSR1);
	raise(SIGUSR1);

	r = uv_queue_work(loop, &work, work_cb, after_cb);
	submit_returned = 1;
	check(r == 0 && !work_ran, "work-submit", r ? uv_strerror(r) : "ran inline");

	uv_fs_open(loop, &fs_open_req, argc > 2 ? argv[2] : self, O_RDONLY, 0, on_fs_open);

	{
		uv_process_options_t o;
		uv_stdio_container_t st[3];
		char *args[3];

		memset(&o, 0, sizeof o);
		uv_pipe_init(loop, &child_out, 0);
		st[0].flags = UV_IGNORE;
		st[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
		st[1].data.stream = (uv_stream_t *)&child_out;
		st[2].flags = UV_INHERIT_FD;
		st[2].data.fd = 2;
		args[0] = (char *)self;
		args[1] = "child";
		args[2] = NULL;
		o.file = self;
		o.args = args;
		o.exit_cb = on_exit_cb;
		o.stdio = st;
		o.stdio_count = 3;
		r = uv_spawn(loop, &proc, &o);
		check(r == 0, "spawn", uv_strerror(r));
		if (r == 0)
			uv_read_start((uv_stream_t *)&child_out, child_alloc, child_read);
		else
			uv_close((uv_handle_t *)&child_out, NULL);
	}

	{
		/* an exec that fails is reported by uv_spawn itself */
		static uv_process_t bad;
		uv_process_options_t o;
		char *args[2];

		memset(&o, 0, sizeof o);
		args[0] = "no-such-program-xyz";
		args[1] = NULL;
		o.file = args[0];
		o.args = args;
		r = uv_spawn(loop, &bad, &o);
		check(r == UV_ENOENT, "spawn-enoent", uv_strerror(r));
		uv_close((uv_handle_t *)&bad, NULL);
	}

	if (uv_guess_handle(1) == UV_TTY) {
		uv_tty_t tty;
		int w = 0, h = 0;
		r = uv_tty_init(loop, &tty, 1, 0);
		check(r == 0, "tty-init", uv_strerror(r));
		if (r == 0) {
			r = uv_tty_get_winsize(&tty, &w, &h);
			printf("   winsize = %dx%d (%s)\n", w, h, r ? uv_strerror(r) : "ok");
			uv_close((uv_handle_t *)&tty, NULL);
		}
	} else
		printf("   (fd 1 is not a terminal: tty checks skipped)\n");

	uv_run(loop, UV_RUN_DEFAULT);

	check(n_idle >= 3, "idle", "idle ran fewer than 3 times");
	check(n_prep > 0 && n_chk > 0, "prepare-check", "prepare/check did not run");
	check(after_ran, "work-done", "after_work never ran");
	check(exit_seen, "spawn-exit-seen", "exit callback never ran");
	r = uv_loop_close(loop);
	check(r == 0, "loop-close", uv_strerror(r));
	printf("%s: %d failed\n", fails ? "FAIL" : "PASS", fails);
	return fails ? 1 : 0;
}
