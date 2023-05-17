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
#include "libc_server.h"
#include "fixed_addr.h"

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

struct libc_call_buffer *callbuf = (struct libc_call_buffer *)LIBC_CALL_BUFFER_ADDR;
static spinlock_t libc_lock;

static uint32_t libc_call(int func, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, int *_errno)
{
	while (callbuf->magic != LIBC_SERVER_READY_MAGIC) {
		asm("wfe");
	}

	spin_lock(&libc_lock);

	struct libc_call *call = &callbuf->calls[callbuf->write_pos];
	call->func = func;
	call->args[0] = arg0;
	call->args[1] = arg1;
	call->args[2] = arg2;
	call->args[3] = arg3;
	call->processed = 0;

	callbuf->write_pos = (callbuf->write_pos + 1) % LIBC_CALL_BUFFER_SIZE;

	spin_unlock(&libc_lock);

	asm("sev");
	while (call->processed == 0) {
		asm("wfe");
	}

	*_errno = call->_errno;
	return call->retval;
}

#define LIBC_CALL0(func, eno) libc_call(func, 0, 0, 0, 0, &eno)
#define LIBC_CALL1(func, eno, arg0) libc_call(func, (uint32_t)arg0, 0, 0, 0, &eno)
#define LIBC_CALL2(func, eno, arg0, arg1) libc_call(func, (uint32_t)arg0, (uint32_t)arg1, 0, 0, &eno)
#define LIBC_CALL3(func, eno, arg0, arg1, arg2) libc_call(func, (uint32_t)arg0, (uint32_t)arg1, (uint32_t)arg2, 0, &eno)

int _write_r(struct _reent *r, int fd, const void *buf, size_t n)
{
	if (fd == 1 || fd == 2)
		return console_write(buf, n);

	if (fd > 2)
		dbg_libc("write %d %p %d\n", fd, buf, n);

	return LIBC_CALL3(LIBC_WRITE, r->_errno, fd, buf, n);
}

int _read_r(struct _reent *r, int fd, void *ptr, size_t n)
{
	if (fd == 1 || fd == 2)
		return -1;

	return LIBC_CALL3(LIBC_READ, r->_errno, fd, ptr, n);
}

off_t _lseek_r(struct _reent *r, int fd, off_t ptr, int dir)
{
	return LIBC_CALL3(LIBC_LSEEK, r->_errno, fd, ptr, dir);
}

int _fstat_r(struct _reent *r, int fd, struct stat * st)
{
	return LIBC_CALL2(LIBC_FSTAT, r->_errno, fd, st);
}

int _open_r(struct _reent *r, const char *path, int c_flags)
{
	printf("opn %s\n", path);
	return LIBC_CALL2(LIBC_OPEN, r->_errno, path, c_flags);
}

int _close_r(struct _reent *r, int fd)
{
	return LIBC_CALL1(LIBC_CLOSE, r->_errno, fd);
}

int _unlink_r(struct _reent *r, const char *path)
{
	return LIBC_CALL1(LIBC_UNLINK, r->_errno, path);
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

int _wait_r(struct _reent *r, int *wstatus)
{
	return LIBC_CALL1(LIBC_WAIT, r->_errno, wstatus);
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
	return (DIR *)LIBC_CALL1(LIBC_OPENDIR, errno, name);
}

int closedir(DIR *dir)
{
	return LIBC_CALL1(LIBC_CLOSEDIR, errno, dir);
}

struct dirent *readdir(DIR *dir)
{
	return (struct dirent *)LIBC_CALL1(LIBC_READDIR, errno, dir);
}

int chdir(const char *path)
{
	return LIBC_CALL1(LIBC_CHDIR, errno, path);
}

char *getcwd(char *buf, size_t size)
{
	return (char *)LIBC_CALL2(LIBC_GETCWD, errno, buf, size);
}

int _stat_r(struct _reent *r, const char *pathname, struct stat *buf)
{
	return LIBC_CALL2(LIBC_STAT, r->_errno, pathname, buf);
}

int _rename_r(struct _reent *r, const char *oldpath, const char *newpath)
{
	return LIBC_CALL2(LIBC_RENAME, r->_errno, oldpath, newpath);
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
	return LIBC_CALL2(LIBC_MKDIR, errno, pathname, mode);
}

int rmdir(const char *pathname)
{
	return LIBC_CALL1(LIBC_RMDIR, errno, pathname);
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
