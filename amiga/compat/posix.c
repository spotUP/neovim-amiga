/* POSIX calls ixemul.library 48.2 has no vectors for, built on what it has.
 * One thread per ixemul process, so the _r forms copy the library's static
 * results. Declared in the compat/include wrappers. (Request R1: these
 * belong in libixcompat.) */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <inttypes.h>
#include <net/if.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* pread/pwrite: seek, transfer, seek back. Not atomic against another
 * process sharing the descriptor's offset (the one thing the real calls
 * guarantee); within this process it is, there being one thread. */
ssize_t
pread(int fd, void *buf, size_t n, off_t off)
{
	off_t was = lseek(fd, 0, SEEK_CUR);
	ssize_t r;
	int e;

	if (was == (off_t)-1 || lseek(fd, off, SEEK_SET) == (off_t)-1)
		return -1;
	r = read(fd, buf, n);
	e = errno;
	lseek(fd, was, SEEK_SET);
	errno = e;
	return r;
}

ssize_t
pwrite(int fd, const void *buf, size_t n, off_t off)
{
	off_t was = lseek(fd, 0, SEEK_CUR);
	ssize_t r;
	int e;

	if (was == (off_t)-1 || lseek(fd, off, SEEK_SET) == (off_t)-1)
		return -1;
	r = write(fd, buf, n);
	e = errno;
	lseek(fd, was, SEEK_SET);
	errno = e;
	return r;
}

/* AmigaOS keeps no owner on a link apart from its target. */
int
lchown(const char *path, uid_t uid, gid_t gid)
{
	(void)path;
	(void)uid;
	(void)gid;
	errno = ENOSYS;
	return -1;
}

int
ttyname_r(int fd, char *buf, size_t len)
{
	char *s = ttyname(fd);

	if (s == NULL)
		return errno ? errno : ENOTTY;
	if (strlen(s) >= len)
		return ERANGE;
	strcpy(buf, s);
	return 0;
}

size_t
strnlen(const char *s, size_t max)
{
	size_t n = 0;

	while (n < max && s[n] != '\0')
		n++;
	return n;
}

/* XXXXXX -> a fresh directory, 0700. */
char *
mkdtemp(char *tmpl)
{
	static const char set[] =
	    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	size_t len = strlen(tmpl), i;
	struct timeval tv;
	unsigned long seed;
	int tries;

	if (len < 6 || strcmp(tmpl + len - 6, "XXXXXX") != 0) {
		errno = EINVAL;
		return NULL;
	}
	gettimeofday(&tv, NULL);
	seed = (unsigned long)tv.tv_sec * 1000003u ^ (unsigned long)tv.tv_usec ^
	    ((unsigned long)getpid() << 8);
	for (tries = 0; tries < 1000; tries++) {
		for (i = len - 6; i < len; i++) {
			seed = seed * 1103515245u + 12345u;
			tmpl[i] = set[(seed >> 16) % (sizeof set - 1)];
		}
		if (mkdir(tmpl, 0700) == 0)
			return tmpl;
		if (errno != EEXIST)
			return NULL;
	}
	errno = EEXIST;
	return NULL;
}

/* select() sleeps; on a signal, *rem gets what was left. */
int
nanosleep(const struct timespec *req, struct timespec *rem)
{
	struct timeval tv, t0, t1;
	long long want, done;

	if (req->tv_nsec < 0 || req->tv_nsec >= 1000000000L) {
		errno = EINVAL;
		return -1;
	}
	tv.tv_sec = req->tv_sec;
	tv.tv_usec = (req->tv_nsec + 999) / 1000;
	gettimeofday(&t0, NULL);
	if (select(0, NULL, NULL, NULL, &tv) == 0)
		return 0;
	if (rem != NULL) {
		gettimeofday(&t1, NULL);
		want = (long long)req->tv_sec * 1000000000LL + req->tv_nsec;
		done = ((long long)(t1.tv_sec - t0.tv_sec) * 1000000LL +
		    (t1.tv_usec - t0.tv_usec)) * 1000LL;
		if (done < 0)
			done = 0;
		want = want > done ? want - done : 0;
		rem->tv_sec = (time_t)(want / 1000000000LL);
		rem->tv_nsec = (long)(want % 1000000000LL);
	}
	errno = EINTR;
	return -1;
}

static int
copy_pw(const struct passwd *p, struct passwd *pw, char *buf, size_t len,
    struct passwd **res)
{
	const char *f[6];
	char **d[6];
	size_t need = 0, n;
	int i;

	*res = NULL;
	f[0] = p->pw_name;	d[0] = &pw->pw_name;
	f[1] = p->pw_passwd;	d[1] = &pw->pw_passwd;
	f[2] = p->pw_class;	d[2] = &pw->pw_class;
	f[3] = p->pw_gecos;	d[3] = &pw->pw_gecos;
	f[4] = p->pw_dir;	d[4] = &pw->pw_dir;
	f[5] = p->pw_shell;	d[5] = &pw->pw_shell;
	for (i = 0; i < 6; i++)
		need += f[i] ? strlen(f[i]) + 1 : 1;
	if (need > len)
		return ERANGE;
	*pw = *p;
	for (i = 0; i < 6; i++) {
		n = f[i] ? strlen(f[i]) : 0;
		if (n)
			memcpy(buf, f[i], n);
		buf[n] = '\0';
		*d[i] = buf;
		buf += n + 1;
	}
	*res = pw;
	return 0;
}

int
getpwuid_r(uid_t uid, struct passwd *pw, char *buf, size_t len,
    struct passwd **res)
{
	struct passwd *p;

	errno = 0;
	p = getpwuid(uid);
	if (p == NULL) {
		*res = NULL;
		return errno;	/* 0: no such user */
	}
	return copy_pw(p, pw, buf, len, res);
}

int
getpwnam_r(const char *name, struct passwd *pw, char *buf, size_t len,
    struct passwd **res)
{
	struct passwd *p;

	errno = 0;
	p = getpwnam(name);
	if (p == NULL) {
		*res = NULL;
		return errno;
	}
	return copy_pw(p, pw, buf, len, res);
}

/* No interface table is kept (the index space is if_nametoindex's). */
char *
if_indextoname(unsigned int index, char *name)
{
	(void)index;
	(void)name;
	errno = ENXIO;
	return NULL;
}

intmax_t
imaxabs(intmax_t n)
{
	return n < 0 ? -n : n;
}

imaxdiv_t
imaxdiv(intmax_t n, intmax_t d)
{
	imaxdiv_t r;

	r.quot = n / d;
	r.rem = n % d;
	return r;
}

intmax_t
strtoimax(const char *s, char **end, int base)
{
	return strtoq(s, end, base);
}

uintmax_t
strtoumax(const char *s, char **end, int base)
{
	return strtouq(s, end, base);
}
