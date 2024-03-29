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
#include <sys/time.h>
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
  /* off_t */   int32_t        st_size;
};

struct newlib_timeval {
  uint64_t tv_sec;
  uint64_t tv_usec;
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

        // Quoting libvte: "Reset the handlers for all signals to their
        // defaults. The parent (or one of the libraries it links to) may
        // have changed one to be ignored."
        for (int n = 1; n < NSIG; n++) {
            if (n == SIGSTOP || n == SIGKILL)
                    continue;

            signal(n, SIG_DFL);
        }

        unsetenv("DISPLAY");	// XXX: is that necessary?
        setenv("TERM", "cons25-debian", 1);
        setenv("LANG", "en_US.UTF-8", 1);
        setenv("HOME", "/sd", 1);
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
    [LIBC_GETTIMEOFDAY] = (void *)gettimeofday,
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

static void translate_timeval(struct newlib_timeval *ntv, const struct timeval *tv)
{
    ntv->tv_sec = tv->tv_sec;
    ntv->tv_usec = tv->tv_usec;
}

// discount version of newlib's fcntl.h
#define NEWLIB__FOPEN          (-1)    /* from sys/file.h, kernel use only */
#define NEWLIB__FREAD          0x0001  /* read enabled */
#define NEWLIB__FWRITE         0x0002  /* write enabled */
#define NEWLIB__FAPPEND        0x0008  /* append (writes guaranteed at the end) */
#define NEWLIB__FMARK          0x0010  /* internal; mark during gc() */
#define NEWLIB__FDEFER         0x0020  /* internal; defer for next gc pass */
#define NEWLIB__FASYNC         0x0040  /* signal pgrp when data ready */
#define NEWLIB__FSHLOCK        0x0080  /* BSD flock() shared lock present */
#define NEWLIB__FEXLOCK        0x0100  /* BSD flock() exclusive lock present */
#define NEWLIB__FCREAT         0x0200  /* open with file create */
#define NEWLIB__FTRUNC         0x0400  /* open with truncation */
#define NEWLIB__FEXCL          0x0800  /* error on open if file exists */
#define NEWLIB__FNBIO          0x1000  /* non blocking I/O (sys5 style) */
#define NEWLIB__FSYNC          0x2000  /* do all writes synchronously */
#define NEWLIB__FNONBLOCK      0x4000  /* non blocking I/O (POSIX style) */
#define NEWLIB__FNDELAY        NEWLIB__FNONBLOCK      /* non blocking I/O (4.2 style) */
#define NEWLIB__FNOCTTY        0x8000  /* don't assign a ctty on this open */
#define NEWLIB__FNOINHERIT	0x40000
#define NEWLIB__FDIRECT	0x80000
#define NEWLIB__FNOFOLLOW	0x100000
#define NEWLIB__FDIRECTORY	0x200000
#define NEWLIB__FEXECSRCH	0x400000

#define NEWLIB_O_RDONLY        0               /* +1 == FREAD */
#define NEWLIB_O_WRONLY        1               /* +1 == FWRITE */
#define NEWLIB_O_RDWR          2               /* +1 == FREAD|FWRITE */
#define NEWLIB_O_APPEND        NEWLIB__FAPPEND
#define NEWLIB_O_CREAT         NEWLIB__FCREAT
#define NEWLIB_O_TRUNC         NEWLIB__FTRUNC
#define NEWLIB_O_EXCL          NEWLIB__FEXCL
#define NEWLIB_O_SYNC          NEWLIB__FSYNC
/*      NEWLIB_O_NDELAY        NEWLIB__FNDELAY        set in include/fcntl.h */
/*      NEWLIB_O_NDELAY        NEWLIB__FNBIO          set in include/fcntl.h */
#define NEWLIB_O_NONBLOCK      NEWLIB__FNONBLOCK
#define NEWLIB_O_NOCTTY        NEWLIB__FNOCTTY

/* POSIX-1.2008 specific flags */
#define NEWLIB_O_CLOEXEC       NEWLIB__FNOINHERIT
#define NEWLIB_O_NOFOLLOW      NEWLIB__FNOFOLLOW
#define NEWLIB_O_DIRECTORY     NEWLIB__FDIRECTORY
#define NEWLIB_O_EXEC          NEWLIB__FEXECSRCH
#define NEWLIB_O_SEARCH        NEWLIB__FEXECSRCH

static int translate_fcntl_flags(int flags_in) {
    int flags_out = 0;
#define N2L(flag) if (flags_in & NEWLIB_ ## flag) flags_out |= flag;
    N2L(O_RDONLY)
    N2L(O_WRONLY)
    N2L(O_RDWR)
    N2L(O_APPEND)
    N2L(O_CREAT)
    N2L(O_TRUNC)
    N2L(O_EXCL)
    N2L(O_SYNC)
    N2L(O_NONBLOCK)
    N2L(O_NOCTTY)
    N2L(O_CLOEXEC)
    N2L(O_NOFOLLOW)
    N2L(O_DIRECTORY)
    // N2L(O_EXEC) POSIX.1
    // N2L(O_SEARCH) no idea

    return flags_out;
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
            case LIBC_OPEN:
                call->args[1] = translate_fcntl_flags(call->args[1]);
                break;
            case LIBC_GETTIMEOFDAY:
                trans_dest = call->args[0];
                call->args[0] = (param_t)malloc(sizeof(struct timeval));
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
            case LIBC_GETTIMEOFDAY:
                if (call->retval == 0)
                    translate_timeval((struct newlib_timeval *)trans_dest, (struct timeval *)call->args[0]);
                free((void *)call->args[0]);
                call->args[0] = trans_dest;
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
