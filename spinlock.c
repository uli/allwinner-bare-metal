// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "spinlock.h"

void spin_lock(spinlock_t *lock)
{
  int tmp;

  asm volatile(
    "1: ldrex %0, [%1];"
    "teq %0, #0;"
    "wfene;"
    "strexeq %0, %2, [%1];"
    "teqeq %0, #0;"
    "bne 1b"
    : "=&r" (tmp)
    : "r" (lock), "r" (1)
    : "cc"
  );
}

void spin_unlock(spinlock_t *lock)
{
  asm(
    "str %1, [%0];"
    "dsb;"
    "sev;"
    :
    : "r" (lock), "r" (0)
    : "cc"
  );
}
