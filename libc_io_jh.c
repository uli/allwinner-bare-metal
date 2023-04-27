#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include "uart.h"
#include <dirent.h>
#include "smp.h"
#include "spinlock.h"

//#define DEBUG_LIBC
//#define DEBUG_LOCK

#ifdef DEBUG_LIBC
static char dbg_buf[256];
#define dbg_libc(x...) do { sprintf(dbg_buf, x); uart_print(dbg_buf); uart_putc('\r'); } while (0)
#else
#define dbg_libc(x...) do {} while (0)
#endif

// _REENT_INIT is what you're supposed to use to initialize the reent
// context, but it doesn't initialize _h_errno and __sf :/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
struct _reent re[4] = {
	_REENT_INIT(re[0]),
	_REENT_INIT(re[1]),
	_REENT_INIT(re[2]),
	_REENT_INIT(re[3]),
};
#pragma GCC diagnostic pop

struct _reent *__getreent(void)
{
	return &re[smp_get_core_id()];
}

int _libc_disable_stdout = 0;

static int console_write(const void *buf, size_t n)
{
	const char *bp = buf;
	int c = n;

	if (_libc_disable_stdout)
		return n;

	while(c--) {
		if (*bp == '\n')
			uart_putc('\r');
		uart_putc(*bp++);
	}
	return n;
}

int _write_r(struct _reent *r, int fd, const void *buf, size_t n)
{
	if (fd == 1 || fd == 2)
		return console_write(buf, n);

	if (fd > 2)
		dbg_libc("write %d %p %d\n", fd, buf, n);

	r->_errno = ENOTSUP;
	return -1;
}

int _read_r(struct _reent *r, int fd, void *ptr, size_t n)
{
	if (fd == 1 || fd == 2)
		return -1;

	r->_errno = ENOTSUP;
	return -1;
}

off_t _lseek_r(struct _reent *r, int fd, off_t ptr, int dir)
{
	int rc;

	r->_errno = ENOTSUP;
	return -1;
}

int _fstat_r(struct _reent *r, int fd, struct stat * st)
{
	r->_errno = ENOTSUP;
	return -1;
}

int _open_r(struct _reent *r, const char *path, int c_flags)
{
	r->_errno = ENOTSUP;
	return -1;
}

int _close_r(struct _reent *r, int fd)
{
	r->_errno = ENOTSUP;
	return -1;
}

int _unlink_r(struct _reent *r, const char *path)
{
	r->_errno = ENOTSUP;
	return -1;
}

void *current_brk;
void *max_brk;

static spinlock_t sbrk_lock;

void libc_set_heap(void *start, void *end)
{
	current_brk = start;
	max_brk = end;
}

void * _sbrk_r(struct _reent *r, ptrdiff_t incr)
{
	spin_lock(&sbrk_lock);
	void *ret_brk = current_brk;
	if (current_brk + incr >= max_brk) {
		r->_errno = ENOMEM;
		spin_unlock(&sbrk_lock);
		return (void *)-1;
	}
	current_brk += incr;
	spin_unlock(&sbrk_lock);
	return ret_brk;
}

int _isatty_r(struct _reent *r, int fd)
{
	(void)r;
	return fd <= 2;
}

void halt(void);

void _exit(int status)
{
	uart_print("exit "); uart_print_uint32(status);
	halt();
}

int _kill_r(struct _reent *r, int pid, int sig)
{
	(void)r;
	printf("kill %d, %d\n", pid, sig);
	halt();
	return -1;
}

int _getpid_r(struct _reent *r, int n)
{
	(void)n; (void)r;
	return 1;
}

int _link(const char *oldpath, const char *newpath)
{
	(void)oldpath;
	(void)newpath;
	errno = EPERM;
	return -1;
}

DIR *opendir(const char *name)
{
	errno = ENOTSUP;
	return NULL;
}

int closedir(DIR *dir)
{
	errno = ENOTSUP;
	return -1;
}

struct dirent *readdir(DIR *dir)
{
	errno = ENOTSUP;
	return NULL;
}

int chdir(const char *path)
{
	errno = ENOTSUP;
	return -1;
}

char *getcwd(char *buf, size_t size)
{
	errno = ENOTSUP;
	return NULL;
}

int _stat_r(struct _reent *r, const char *pathname, struct stat *buf)
{
	r->_errno = ENOTSUP;
	return -1;
}

int _rename_r(struct _reent *r, const char *oldpath, const char *newpath)
{
	r->_errno = ENOTSUP;
	return -1;
}

#include "rtc.h"
#include "system.h"

static int64_t timeoff = 0;

int _gettimeofday_r (struct _reent *r, struct timeval *tp, void *tzp)
{
	(void)r; (void)tzp;
	if (timeoff == 0) {
		struct tm t;
		t.tm_sec = rtc_get_second();
		t.tm_min = rtc_get_minute();
		t.tm_hour = rtc_get_hour();
		t.tm_mday = rtc_get_day();
		t.tm_mon = rtc_get_month();
		t.tm_year = rtc_get_year() - 1900;
		timeoff = mktime(&t) * 1000000 - sys_get_usec();
	}
	uint64_t us = sys_get_usec() + timeoff;
	tp->tv_sec = us / 1000000;
	tp->tv_usec = us % 1000000;
	return 0;
}

int mkdir(const char *pathname, mode_t mode)
{
	errno = ENOTSUP;
	return -1;
}

int rmdir(const char *pathname)
{
	errno = ENOTSUP;
	return -1;
}

// __malloc_lock and __malloc_unlock are not weak symbols because newlib's
// primary goal is to be as annoying as possible. To work around that we
// enlist ld's help and let it wrap these functions so we can override them.
// Make absolutely sure your program is linked with "-Wl,-wrap,__malloc_lock
// -Wl,-wrap,__malloc_unlock" or the implementations here will not be used!

// newlib malloc wants a recursive lock...
static spinlock_t malloc_lock_v;
static int malloc_lock_owner = 0;
static int malloc_lock_count = 0;

void __wrap___malloc_lock(struct _reent *r)
{
	(void)r;
#ifdef DEBUG_LOCK
	uart_print("malloc lock\r\n");
#endif
	for (;;) {
		spin_lock(&malloc_lock_v);
		if (malloc_lock_count <= 0) {
			malloc_lock_count = 1;
			malloc_lock_owner = smp_get_core_id();
			spin_unlock(&malloc_lock_v);
			return;
		}
		if (malloc_lock_owner == smp_get_core_id()) {
			malloc_lock_count++;
			spin_unlock(&malloc_lock_v);
			return;
		}
		spin_unlock(&malloc_lock_v);
	}
}

void __wrap___malloc_unlock(struct _reent *r)
{
	(void)r;
#ifdef DEBUG_LOCK
	uart_print("malloc unlock\r\n");
#endif
	spin_lock(&malloc_lock_v);
	if (--malloc_lock_count <= 0)
		malloc_lock_count = 0;
	spin_unlock(&malloc_lock_v);
}
