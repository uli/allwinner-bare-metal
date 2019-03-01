#include <stdio.h>
#include <stdlib.h>
#include "uart.h"
#include "sdgpio/ff.h"

int errno;

static size_t stdio_write(FILE *instance, const char *bp, size_t n)
{
	int c = n;
	(void)instance;	// always stdout or stderr
	while(c--) {
		if (*bp == '\n')
			uart_putc('\r');
		uart_putc(*bp++);
	}
	return n;
}

static struct File_methods stdio_methods = {
        &stdio_write, NULL
};

static struct File _stdout = {
        &stdio_methods
};

static struct File _stderr = {
        &stdio_methods
};

FILE* const stdout = &_stdout;
FILE* const stderr = &_stderr;

struct glue_file {
	struct File file;
	FIL fil;
};

static size_t glue_write(FILE *instance, const char *bp, size_t n)
{
	FIL *filp = &((struct glue_file *)instance)->fil;
	size_t bw;
	int rc;

	if ((rc = f_write(filp, bp, n, &bw))) {
		errno = rc;
		return 0;
	}
	return bw;
}

static size_t glue_read(FILE *instance, char *bp, size_t n)
{
	FIL *filp = &((struct glue_file *)instance)->fil;
	size_t br;
	int rc;
	
	if ((rc = f_read(filp, bp, n, &br))) {
		errno = rc;
		return 0;
	}
	return br;
}

static struct File_methods glue_methods = {
	&glue_write, &glue_read
};

FILE *fopen(const char *path, const char *mode)
{
	int rc;
	struct glue_file *gf;
	int flags = 0;
	switch (mode[0]) {
	case 'r':	flags = FA_READ; break;
	case 'w':	flags = FA_WRITE | FA_CREATE_ALWAYS; break;
	case 'a':	flags = FA_WRITE | FA_OPEN_ALWAYS; break;
	default:	return 0;
	}

	gf = (struct glue_file *)malloc(sizeof(struct glue_file));
	if (!gf) {
		errno = 12; /* ENOMEM */
		return 0;
	}
	gf->file.vmt = &glue_methods;

	rc = f_open(&gf->fil, path, flags);
	if (rc) {
		errno = rc;
		free(gf);
		return 0;
	}

	return (FILE *)gf;
}

int fclose(FILE *stream)
{
	struct glue_file *gf = (struct glue_file *)stream;

	int rc = f_close(&gf->fil);
	if (rc) {
		errno = rc;
		/* free? */
		return EOF;
	}

	free(gf);
	return 0;
}
