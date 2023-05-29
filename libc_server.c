// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

// Libc server

// Provides access to Linux OS functions to bare-metal programs running in a
// Jailhouse cell.

//#define DEBUG

#include "libc_server.h"
#include "fixed_addr.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Force bare-metal versions of data structures into this Linux program:

#define _NO_PROTOTYPES

#define dirent awb_dirent
#define DIR awb_DIR
#include "dirent.h"
#undef dirent
#undef DIR
#define awb_DT_DIR DT_DIR
#define awb_DT_REG DT_REG
#undef DT_DIR
#undef DT_REG

// discount version of newlib's struct stat
struct newlib_stat {
  /* dev_t */   short          st_dev;
  /* ino_t */   unsigned short st_ino;
  /* mode_t */  uint32_t       st_mode;
  /* nlink_t */ unsigned short st_nlink;
  /* uid_t */   unsigned short st_uid;
  /* gid_t */   unsigned short st_gid;
  /* dev_t */   unsigned short st_rdev;
  /* off_t */   long           st_size;
};

#undef _NO_PROTOTYPES

#include <dirent.h>

// This is an Engine BASIC special helper that implements the Linux side of
// a SHELL command.
// The forked process bits have to be done entirely on our side because we
// only have a single thread on the Engine BASIC side.
#include <pty.h>
pid_t jhlibc_forkptyexec(int *fd, struct winsize *ws, char * const *argv)
{
    pid_t pid = forkpty(fd, NULL, NULL, ws);
    if (pid == 0) {
        // shell
        unsetenv("DISPLAY");
        setenv("TERM", "ansi", 1);
        setenv("LANG", "en_US.UTF-8", 1);
        if (argv[0] == NULL)
            execl("/bin/sh", "sh", NULL);
        else if (argv[1] == NULL)
            execl("/bin/sh", "sh", "-c", argv[0], (char *) 0);
        else
            execvp(argv[0], argv);
        _exit(1);
    } else {
        if (pid > 0)
            fcntl(*fd, F_SETFL, O_NONBLOCK);
        return pid;
    }
}

struct libc_call_buffer *callbuf;

typedef param_t (*call4)(param_t, param_t, param_t, param_t);

const call4 libc_functors[LIBC_LAST] = {
    [LIBC_WRITE] = (void *)write,
    [LIBC_READ] = (void *)read,
    [LIBC_LSEEK] = (void *)lseek,
    [LIBC_FSTAT] = (void *)fstat,
    [LIBC_OPEN] = (void *)open,
    [LIBC_CLOSE] = (void *)close,
    [LIBC_UNLINK] = (void *)unlink,
    [LIBC_OPENDIR] = (void *)opendir,
    [LIBC_CLOSEDIR] = (void *)closedir,
    [LIBC_READDIR] = (void *)readdir,
    [LIBC_CHDIR] = (void *)chdir,
    [LIBC_GETCWD] = (void *)getcwd,
    [LIBC_STAT] = (void *)stat,
    [LIBC_RENAME] = (void *)rename,
    [LIBC_MKDIR] = (void *)mkdir,
    [LIBC_RMDIR] = (void *)rmdir,
    [LIBC_WAIT] = (void *)wait,
    [LIBC_JHLIBC_FORKPTYEXEC] = (void *)jhlibc_forkptyexec,
    [LIBC_SYSTEM] = (void *)system,
};

static void translate_stat(struct newlib_stat *dst, struct stat *src)
{
    dst->st_dev = src->st_dev;
    dst->st_ino = src->st_ino;
    dst->st_mode = src->st_mode;
    dst->st_nlink = src->st_nlink;
    dst->st_uid = src->st_uid;
    dst->st_gid = src->st_gid;
    dst->st_rdev = src->st_rdev;
    dst->st_size = src->st_size;
}

static void translate_dirent(struct awb_dirent *dst, struct dirent *src)
{
    dst->d_type = src->d_type;	// XXX: do these need translation?
    strncpy(dst->d_name, src->d_name, sizeof(dst->d_name) - 1);
    dst->d_name[sizeof(dst->d_name) - 1] = 0;
}

int main(void)
{
    // Map shared memory communication regions.
    // We use fixed ID mappings so we don't have to translate bare-metal pointers.

    int mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        perror("failed to open memory device");
        return 0;
    }

    // Map call ring buffer.
    callbuf = (struct libc_call_buffer *)mmap((void *)LIBC_CALL_BUFFER_ADDR, sizeof(*callbuf), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, mem_fd, LIBC_CALL_BUFFER_ADDR);
    if (callbuf == MAP_FAILED) {
        perror("failed to map call buffer");
        return 0;
    }

    // Map BASIC program memory (text, data, bss)
    // XXX: We would like to map up to 2GB here, but that fails for lack of
    // available address space, so we leave 256MB unused on 2GB devices.
    void *basic_mem = mmap((void *)0x49000000, 0x67000000, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_FIXED, mem_fd, 0x49000000);
    if (basic_mem == MAP_FAILED) {
        perror("failed to map BASIC memory");
        return 0;
    }
    printf("BASIC mem at %p\n", basic_mem);

    // We also need access to the BASIC stack because we may get pointers to it.
    void *basic_stack = mmap((void *)0x8000, 0x8000, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_FIXED, mem_fd, 0x48808000);
    if (basic_stack == MAP_FAILED) {
        perror("failed to map BASIC stack");
        return 0;
    }
    printf("BASIC stack at %p\n", basic_stack);

    // Tell BASIC we're ready for business.
    if (callbuf->magic != LIBC_SERVER_READY_MAGIC) {
        callbuf->read_pos = callbuf->write_pos = 0;
        callbuf->magic = LIBC_SERVER_READY_MAGIC;
    }

    for (;;) {
        // Wait until there is something to do.
        time_t wait_start = time(NULL);
        while (callbuf->read_pos == callbuf->write_pos) {
            // When we're idling for a long time we schedule ourselves out a
            // bit to avoid hogging the CPU.
            if (time(NULL) > wait_start + 2)
                usleep(1000);
            else
                asm("wfe");	// XXX: portability, anyone?
        }

        struct libc_call *call = &callbuf->calls[callbuf->read_pos];

#ifdef DEBUG
        printf("proc libc call %d\n", call->func);
#endif

        // Replace fixed-size destination buffers with ones that are able to
        // accommodate the corresponding Linux libc structures.

        static param_t trans_dest;
        switch (call->func) {
            case LIBC_FSTAT:
            case LIBC_STAT:
                trans_dest = call->args[1];
                call->args[1] = (param_t)malloc(sizeof(struct stat));
                // XXX: handle OOM
                break;
            default:
                break;
        }

        // call libc function

        call4 functor = libc_functors[call->func];
        call->retval = functor(call->args[0], call->args[1], call->args[2], call->args[3]);

        // Translate data Linux libc data structures to their bare-metal equivalents.

        switch (call->func) {
            case LIBC_FSTAT:
            case LIBC_STAT:
                if (call->retval == 0)
                    translate_stat((struct newlib_stat *)trans_dest, (struct stat *)call->args[1]);
                free((void *)call->args[1]);
                call->args[1] = trans_dest;
                break;
            case LIBC_READDIR:
                if (call->retval != 0) {
                    translate_dirent((struct awb_dirent *)call->compound_retval,
                                     (struct dirent *)call->retval);
                    call->retval = (param_t)call->compound_retval;
                }
                break;
            default:
                break;
        }

        call->_errno = errno;
        call->processed = 1;

        // Wake up bare-metal cell.
        asm("sev");	// XXX: portability!

        // Move to the next call in the ring buffer.
        callbuf->read_pos = (callbuf->read_pos + 1) % LIBC_CALL_BUFFER_SIZE;
    }
}
