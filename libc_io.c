#include <stdio.h>
#include "uart.h"
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
