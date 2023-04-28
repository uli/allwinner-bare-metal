#ifdef __cplusplus
extern "C" {
#endif

#ifdef JAILHOUSE
struct _DIR;
typedef struct _DIR DIR;
#else
#include "fatfs/ff.h"
#endif

struct dirent {
	unsigned char d_type;
	char d_name[256];	/* filename */
};

#define DT_DIR	4
#define DT_REG	8

#ifndef _NO_PROTOTYPES
DIR *opendir(const char *name);
int closedir(DIR *dirp);
struct dirent *readdir(DIR *dir);
#endif

#ifdef __cplusplus
}
#endif
