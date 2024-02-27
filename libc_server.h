// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

#include <stdint.h>
#include <limits.h>

#if !defined(INT_MAX) || !defined(LONG_MAX)
#error INT_MAX/LONG_MAX not defined
#endif

// Find a data type that fits both pointers and integers.

// XXX: Instead of hardcoding the platform this should use the Linux cell
// architecture.
// Problem is that this header is parsed when compiling both
// the libc server (32-bit or 64-bit) and the payload (always 32-bit)...
#ifdef AWBM_PLATFORM_h3
typedef uint32_t param_t;
#elif defined(AWBM_PLATFORM_h616)
typedef uint64_t param_t;
#else
#error platform param_t type undefined
#endif

enum libc_funcs {
    LIBC_WRITE,
    LIBC_READ,
    LIBC_LSEEK,
    LIBC_FSTAT,
    LIBC_OPEN,
    LIBC_CLOSE,
    LIBC_UNLINK,
    LIBC_OPENDIR,
    LIBC_CLOSEDIR,
    LIBC_READDIR,
    LIBC_CHDIR,
    LIBC_GETCWD,
    LIBC_STAT,
    LIBC_RENAME,
    LIBC_MKDIR,
    LIBC_RMDIR,
    LIBC_WAIT,
    LIBC_JHLIBC_FORKPTYEXEC,
    LIBC_SYSTEM,
    LIBC_GETTIMEOFDAY,
    LIBC_LAST
};

// Data structure describing the libc function to be called.

// The bare-metal client fills in the funcion index and arguments and sets
// processed to 0, then adds the call to the ring buffer.

// Once the server has executed, it sets retval and _errno and sets
// processed to 1.

// If a pointer to a compound data type that had to be translated is
// returned, the translated data is saved in compound_retval[], and retval
// is set to point there, making the translation transparent to the client.

// XXX: This currently relies on all libc functions having four arguments or
// less, and thus don't need to use the stack.

struct libc_call {
    int func;
    param_t args[4];
    param_t retval;
    uint32_t _errno;
    int processed;
    uint8_t compound_retval[384];
};

#define LIBC_CALL_BUFFER_SIZE 32

#define LIBC_SERVER_READY_MAGIC 0x00137590

struct libc_call_buffer {
    uint32_t magic;
    int read_pos;
    int write_pos;
    struct libc_call calls[LIBC_CALL_BUFFER_SIZE];
};
