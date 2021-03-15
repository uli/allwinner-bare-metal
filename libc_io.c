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

//#define DEBUG_LIBC

#ifdef DEBUG_LIBC
#define dbg_libc(x...) printf(x)
#else
#define dbg_libc(x...) do {} while (0)
#endif

struct _reent re[4] = {
	_REENT_INIT(re[0]),
	_REENT_INIT(re[1]),
	_REENT_INIT(re[2]),
	_REENT_INIT(re[3]),
};

struct _reent *__getreent(void)
{
	return &re[smp_get_core_id()];
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

static int console_write(const void *buf, size_t n)
{
	const char *bp = buf;
	int c = n;

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

	FIL *filp = get_descr(r, fd);
	dbg_libc("write filp %p\n", filp);
	if (!filp)
		return -1;

	size_t bw;
	int rc;

	if ((rc = f_write(filp, buf, n, &bw))) {
		dbg_libc("write err %d\n", rc);
		r->_errno = rc;
		return -1;
	}
	return bw;
}

int _read_r(struct _reent *r, int fd, void *ptr, size_t n)
{
	if (fd == 1 || fd == 2)
		return -1;

	FIL *filp = get_descr(r, fd);
	if (!filp)
		return -1;

	size_t br;
	int rc;

	if ((rc = f_read(filp, ptr, n, &br))) {
		r->_errno = rc;
		return -1;
	}
	return br;
}

off_t _lseek_r(struct _reent *r, int fd, off_t ptr, int dir)
{
	int rc;
	FIL *fil = get_descr(r, fd);
	if (!fil) {
		r->_errno = EBADF;
		return -1;
	}

	off_t dest = ptr;
	switch (dir) {
	case SEEK_SET: break;
	case SEEK_CUR: dest += fil->fptr; break;
	case SEEK_END: dest += fil->obj.objsize; break;
	default: r->_errno = EINVAL; return -1;
	}
	rc = f_lseek(fil, dest);
	if (rc) {
		r->_errno = rc;
		return -1;
	}
	return dest;
}

int _fstat_r(struct _reent *r, int fd, struct stat * st)
{
uart_print(__FUNCTION__);
	FIL *fil = get_descr(r, fd);
	if (!fil) {
		r->_errno = EBADF;
		return -1;
	}

	memset (st, 0, sizeof (* st));
	st->st_mode = S_IFREG;
	st->st_size = fil->obj.objsize;
	st->st_blksize = 512;
	st->st_blocks = fil->obj.objsize / 512;

	return 0;
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

	for (i = 0; i < MAX_FILE_DESCRIPTORS; ++i) {
		if (!file_descriptor[i].obj.fs) {
			fil = &file_descriptor[i];
			break;
		}
	}
	if (!fil) {
		r->_errno = ENOMEM; // XXX: right?
		return -1;
	}

	rc = f_open(fil, path, flags);
	dbg_libc("open %s %d rc %d\n", path, flags, rc);
	if (rc) {
		r->_errno = rc;
		return -1;
	}

	return i + 3;
}

int _close_r(struct _reent *r, int fd)
{
	FIL *fil = get_descr(r, fd);
	if (fd > 2)
		dbg_libc("close filp %p\n", fil);
	if (!fil)
		return -1;

	int rc = f_close(fil);
	if (rc) {
		dbg_libc("close err %d\n", rc);
		r->_errno = rc;
		return EOF;
	}
	dbg_libc("close ok\n");

	return 0;
}

int _unlink_r(struct _reent *r, const char *path)
{
	FRESULT rc = f_unlink(path);
	if (rc) {
		r->_errno = rc;
		return -1;
	}
	return 0;
}

void *current_brk = (void *)0x40000000;
extern char _hend;	// heap end, defined by linker script

void * _sbrk_r(struct _reent *r, ptrdiff_t incr)
{
	void *ret_brk = current_brk;
	if (current_brk + incr >= (void *)&_hend) {
		r->_errno = ENOMEM;
		return (void *)-1;
	}
	current_brk += incr;
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

	int rc = f_opendir(dir, name);
	if (rc) {
		free(dir);
		errno = rc;
		return NULL;
	}
	return dir;
}

int closedir(DIR *dir)
{
	int rc = f_closedir(dir);
	if (rc) {
		// XXX: free?
		errno = rc;
		return -1;
	}
	free(dir);
	return 0;
}

struct dirent *readdir(DIR *dir)
{
	FILINFO fi;
	static struct dirent de;
	int rc = f_readdir(dir, &fi);
	if (rc) {
		// error
		errno = rc;
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
	int rc = f_chdir(path);
	if (rc) {
		errno = rc;
		return -1;
	}
	return 0;
}

char *getcwd(char *buf, size_t size)
{
	int rc = f_getcwd(buf, size);
	if (rc) {
		errno = rc;
		return NULL;
	}
	return buf;
}

int _stat_r(struct _reent *r, const char *pathname, struct stat *buf)
{
	FILINFO fi;
	int rc = f_stat(pathname, &fi);
	if (rc) {
		r->_errno = rc;
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
	r->_errno = EINVAL;
	return 1;
}

int mkdir(const char *pathname, mode_t mode)
{
	(void)mode;
	int rc = f_mkdir(pathname);
	if (rc) {
		errno = rc;
		return -1;
	}
	return 0;
}

int rmdir(const char *pathname)
{
	int rc = f_rmdir(pathname);
	if (rc) {
		errno = rc;
		return -1;
	}
	return 0;
}
