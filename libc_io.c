#include <stdio.h>
#include "uart.h"
static size_t stdio_write(FILE *instance, const char *bp, size_t n)
{
	if (instance == stdout || instance == stderr) {
		int c = n;
		while(c--) {
			uart_putc(*bp++);
		}
		return n;
	} else {
		return 0;
	}
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
