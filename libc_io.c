#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include "uart.h"
#include "fatfs/ff.h"
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

static spinlock_t fs_lock_v = 0;

#ifdef DEBUG_LOCK
#define fs_lock() do { uart_print(__FUNCTION__); uart_print(" lock\r\n"); spin_lock(&fs_lock_v); } while (0)
#define fs_unlock() do { uart_print(__FUNCTION__); uart_print(" unlock\r\n"); spin_unlock(&fs_lock_v); } while (0)
#else
#define fs_lock() spin_lock(&fs_lock_v)
#define fs_unlock() spin_unlock(&fs_lock_v)
#endif

#define fs_unlock_ret(n) do { fs_unlock(); return (n); } while (0);

static int rc2errno(int rc) {
	switch (rc) {
	case FR_OK: return 0;
	case FR_DISK_ERR: return EIO;
	case FR_NOT_READY: return EAGAIN;
	case FR_NO_FILE: return ENOENT;
	case FR_NO_PATH: return ENOENT;
	case FR_INVALID_NAME: return EINVAL;
	case FR_DENIED: return EPERM;
	case FR_EXIST: return EEXIST;
	case FR_INVALID_OBJECT: return EBADF;
	case FR_WRITE_PROTECTED: return EROFS;
	case FR_NOT_ENOUGH_CORE: return ENOMEM;
	case FR_TOO_MANY_OPEN_FILES: return EMFILE;
	case FR_INVALID_PARAMETER: return EINVAL;
	default: return EIO;
	}
}

#define MAX_FILE_DESCRIPTORS 16
static FIL file_descriptor[MAX_FILE_DESCRIPTORS] = { };

static FIL *get_descr(struct _reent *r, int fd)
{
	fd -= 3;
	if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS) {
		r->_errno = EBADF;
		return NULL;
	}
	return &file_descriptor[fd];
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

	void *bbuf = (void *)buf;
	int free_tmp_buffer = 0;

	// the MMC driver doesn't like unaligned buffers
	if (((ptrdiff_t)buf) & 3) {
		dbg_libc("unaligned write buffer\n");
		bbuf = malloc(n);
		memcpy(bbuf, buf, n);
		free_tmp_buffer = 1;
	}

	fs_lock();
	FIL *filp = get_descr(r, fd);
	dbg_libc("write filp %p\n", filp);
	if (!filp) {
		if (free_tmp_buffer)
			free(bbuf);
		fs_unlock_ret(-1);
	}

	size_t bw;
	int rc;

	if ((rc = f_write(filp, bbuf, n, &bw))) {
		dbg_libc("write err %d\n", rc);
		r->_errno = rc2errno(rc);
		if (free_tmp_buffer)
			free(bbuf);
		fs_unlock_ret(-1);
	}
	if (free_tmp_buffer)
		free(bbuf);
	fs_unlock_ret(bw);
}

int _read_r(struct _reent *r, int fd, void *ptr, size_t n)
{
	if (fd == 1 || fd == 2)
		return -1;

	void *pptr = (void *)ptr;
	int free_tmp_buffer = 0;

	// the MMC driver doesn't like unaligned buffers
	if (((ptrdiff_t)ptr) & 3) {
		dbg_libc("unaligned read buffer\n");
		pptr = malloc(n);
		free_tmp_buffer = 1;
	}

	fs_lock();
	FIL *filp = get_descr(r, fd);
	if (!filp) {
		if (free_tmp_buffer)
			free(pptr);
		fs_unlock_ret(-1);
	}

	size_t br;
	int rc;

	if ((rc = f_read(filp, pptr, n, &br))) {
		r->_errno = rc2errno(rc);

		if (free_tmp_buffer)
			free(pptr);
		fs_unlock_ret(-1);
	}

	if (free_tmp_buffer) {
		memcpy(ptr, pptr, n);
		free(pptr);
	}

	fs_unlock_ret(br);
}

off_t _lseek_r(struct _reent *r, int fd, off_t ptr, int dir)
{
	int rc;

	fs_lock();
	FIL *fil = get_descr(r, fd);
	if (!fil) {
		r->_errno = EBADF;
		fs_unlock_ret(-1);
	}

	off_t dest = ptr;
	switch (dir) {
	case SEEK_SET: break;
	case SEEK_CUR: dest += fil->fptr; break;
	case SEEK_END: dest += fil->obj.objsize; break;
	default: r->_errno = EINVAL; fs_unlock_ret(-1);
	}
	rc = f_lseek(fil, dest);
	if (rc) {
		r->_errno = rc2errno(rc);
		fs_unlock_ret(-1);
	}
	fs_unlock_ret(dest);
}

int _fstat_r(struct _reent *r, int fd, struct stat * st)
{
	fs_lock();
	FIL *fil = get_descr(r, fd);
	if (!fil) {
		r->_errno = EBADF;
		fs_unlock_ret(-1);
	}

	memset (st, 0, sizeof (* st));
	st->st_mode = S_IFREG;
	st->st_size = fil->obj.objsize;
	st->st_blksize = 512;
	st->st_blocks = fil->obj.objsize / 512;

	fs_unlock_ret(0);
}

int _open_r(struct _reent *r, const char *path, int c_flags)
{
	int rc, i;
	FIL *fil = NULL;
	int flags = 0;

	dbg_libc("open inflags %d\n", c_flags);

	if (c_flags & (O_RDWR | O_WRONLY))
		flags |= FA_WRITE;
	else
		flags |= FA_READ;

	if (c_flags & O_CREAT) {
		if (c_flags & O_TRUNC)
			flags |= FA_CREATE_ALWAYS;
		else
			flags |= FA_OPEN_ALWAYS;
	}

	if (c_flags & O_APPEND)
		flags |= FA_OPEN_APPEND;

	fs_lock();
	for (i = 0; i < MAX_FILE_DESCRIPTORS; ++i) {
		if (!file_descriptor[i].obj.fs) {
			fil = &file_descriptor[i];
			break;
		}
	}
	if (!fil) {
		r->_errno = ENOMEM; // XXX: right?
		fs_unlock_ret(-1);
	}

	rc = f_open(fil, path, flags);
	dbg_libc("open %s %d rc %d\n", path, flags, rc);
	if (rc) {
		r->_errno = rc2errno(rc);
		fs_unlock_ret(-1);
	}

	fs_unlock_ret(i + 3);
}

int _close_r(struct _reent *r, int fd)
{
	fs_lock();
	FIL *fil = get_descr(r, fd);
	if (fd > 2)
		dbg_libc("close filp %p\n", fil);
	if (!fil)
		fs_unlock_ret(-1);

	int rc = f_close(fil);
	if (rc) {
		dbg_libc("close err %d\n", rc);
		r->_errno = rc2errno(rc);
		fs_unlock_ret(EOF);
	}
	dbg_libc("close ok\n");

	fs_unlock_ret(0);
}

int _unlink_r(struct _reent *r, const char *path)
{
	fs_lock();
	FRESULT rc = f_unlink(path);
	fs_unlock();

	if (rc) {
		r->_errno = rc2errno(rc);
		return -1;
	}
	return 0;
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
	DIR* dir = (DIR *)malloc(sizeof(DIR));
	if (!dir) {
		errno = ENOMEM;
		return NULL;
	}

	fs_lock();
	int rc = f_opendir(dir, name);
	fs_unlock();

	if (rc) {
		free(dir);
		errno = rc2errno(rc);
		return NULL;
	}
	return dir;
}

int closedir(DIR *dir)
{
	fs_lock();
	int rc = f_closedir(dir);
	fs_unlock();

	if (rc) {
		// XXX: free?
		errno = rc2errno(rc);
		return -1;
	}
	free(dir);
	return 0;
}

struct dirent *readdir(DIR *dir)
{
	FILINFO fi;
	static struct dirent de;

	fs_lock();
	int rc = f_readdir(dir, &fi);
	fs_unlock();

	if (rc) {
		// error
		errno = rc2errno(rc);
		return NULL;
	}
	if (!fi.fname[0]) {
		// end of directory
		return NULL;
	}
	memset(de.d_name, 0, sizeof(de.d_name));
	strncpy(de.d_name, fi.fname, sizeof(de.d_name) - 1);
	if (fi.fattrib & AM_DIR)
		de.d_type = DT_DIR;
	else
		de.d_type = DT_REG;

	return &de;
}

int chdir(const char *path)
{
	fs_lock();
	int rc = f_chdir(path);
	fs_unlock();

	if (rc) {
		errno = rc2errno(rc);
		return -1;
	}
	return 0;
}

char *getcwd(char *buf, size_t size)
{
	fs_lock();
	int rc = f_getcwd(buf, size);
	fs_unlock();

	if (rc) {
		errno = rc2errno(rc);
		return NULL;
	}
	return buf;
}

int _stat_r(struct _reent *r, const char *pathname, struct stat *buf)
{
	FILINFO fi;

	fs_lock();
	int rc = f_stat(pathname, &fi);
	fs_unlock();

	if (rc) {
		r->_errno = rc2errno(rc);
		return -1;
	}

	memset(buf, 0, sizeof(struct stat));
	buf->st_size = fi.fsize;
	buf->st_mtime = fi.ftime; // XXX: unit?
	buf->st_mode = fi.fattrib & 0x10 ? S_IFDIR : S_IFREG;

	return 0;
}

int _rename_r(struct _reent *r, const char *oldpath, const char *newpath)
{
	(void)oldpath; (void)newpath;
	fs_lock();
	int rc = f_rename(oldpath, newpath);
	fs_unlock();

	if (rc) {
		r->_errno = rc2errno(rc);
		return -1;
	}

	return 0;
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
	(void)mode;

	fs_lock();
	int rc = f_mkdir(pathname);
	fs_unlock();

	if (rc) {
		errno = rc2errno(rc);
		return -1;
	}
	return 0;
}

int rmdir(const char *pathname)
{
	fs_lock();
	int rc = f_rmdir(pathname);
	fs_unlock();

	if (rc) {
		errno = rc2errno(rc);
		return -1;
	}
	return 0;
}
