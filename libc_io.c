#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include "uart.h"
#include "fatfs/ff.h"
#include <dirent.h>

#define MAX_FILE_DESCRIPTORS 16
static FIL file_descriptor[MAX_FILE_DESCRIPTORS] = { };

static FIL *get_descr(int fd)
{
	fd -= 3;
	if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS) {
		errno = EBADF;
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

int _write(int fd, const void *buf, size_t n)
{
	if (fd == 1 || fd == 2)
		return console_write(buf, n);

	FIL *filp = get_descr(fd);
	if (!filp)
		return -1;

	size_t bw;
	int rc;

	if ((rc = f_write(filp, buf, n, &bw))) {
		errno = rc;
		return -1;
	}
	return bw;
}

int _read(int fd, void *ptr, size_t n)
{
	if (fd == 1 || fd == 2)
		return -1;

	FIL *filp = get_descr(fd);
	if (!filp)
		return -1;

	size_t br;
	int rc;
	
	if ((rc = f_read(filp, ptr, n, &br))) {
		errno = rc;
		return -1;
	}
	return br;
}

int _lseek(int fd, int ptr, int dir)
{
	int rc;
	FIL *fil = get_descr(fd);
	if (!fil)
		return -1;
	
	switch (dir) {
	case SEEK_SET: rc = f_lseek(fil, ptr); break;
	case SEEK_CUR: rc = f_lseek(fil, ptr + fil->fptr); break;
	case SEEK_END: rc = f_lseek(fil, ptr + fil->obj.objsize); break;
	default: errno = EINVAL; return -1;
	}
	if (rc) {
		errno = rc;
		return -1;
	}
	return 0;
}

int _fstat (int fd, struct stat * st)
{
uart_print(__FUNCTION__);
	FIL *fil = get_descr(fd);
	if (!fil)
		return -1;

	memset (st, 0, sizeof (* st));
	st->st_mode = S_IFREG;
	st->st_size = fil->obj.objsize;
	st->st_blksize = 512;
	st->st_blocks = fil->obj.objsize / 512;

	return 0;
}

int _open(const char *path, int c_flags)
{
	int rc, i;
	FIL *fil = NULL;
	int flags = 0;

	if (c_flags & O_RDWR)
		flags |= FA_WRITE;
	else
		flags |= FA_READ;

	if (c_flags & O_CREAT) {
		if (c_flags & O_TRUNC)
			flags |= FA_CREATE_ALWAYS;
		else
			flags |= FA_OPEN_ALWAYS;
	}

	for (i = 0; i < MAX_FILE_DESCRIPTORS; ++i) {
		if (!file_descriptor[i].obj.fs) {
			fil = &file_descriptor[i];
			break;
		}
	}
	if (!fil) {
		errno = ENOMEM; // XXX: right?
		return -1;
	}

	rc = f_open(fil, path, flags);
	if (rc) {
		errno = rc;
		return -1;
	}

	return i + 3;
}

int _close (int fd)
{
	FIL *fil = get_descr(fd);
	if (!fil)
		return -1;

	int rc = f_close(fil);
	if (rc) {
		errno = rc;
		return EOF;
	}

	return 0;
}

int _unlink(const char *path)
{
	FRESULT rc = f_unlink(path);
	if (rc) {
		errno = rc;
		return -1;
	}
	return 0;
}

void *current_brk = (void *)0x40000000;
extern char _hend;	// heap end, defined by linker script

void * _sbrk(ptrdiff_t incr)
{
	void *ret_brk = current_brk;
	if (current_brk + incr >= (void *)&_hend) {
		errno = ENOMEM;
		return (void *)-1;
	}
	current_brk += incr;
	return ret_brk;
}

int _isatty (int fd)
{
	return fd <= 2;
}

void halt(void);

void _exit(int status)
{
	uart_print("exit "); uart_print_uint32(status);
	halt();
}

int _kill(int pid, int sig)
{
	printf("kill %d, %d\n", pid, sig);
	halt();
	return -1;
}

int _getpid(int n)
{
	(void)n;
	return 1;
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
