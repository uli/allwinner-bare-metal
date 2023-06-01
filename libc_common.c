#include <errno.h>
#include "smp.h"
#include "spinlock.h"

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

// newlib only compiles basename for Unix targets because it's stupid
#include <string.h>
char* basename(char *path)
{
	char *p;
	if( path == NULL || *path == '\0' )
		return ".";
	p = path + strlen(path) - 1;
	while( *p == '/' ) {
		if( p == path )
			return path;
		*p-- = '\0';
	}
	while( p >= path && *p != '/' )
		p--;
	return p + 1;
}
