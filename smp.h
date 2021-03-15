// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef void (*secondary_task_t)(void);

void smp_start_secondary_core(int cpuid, secondary_task_t task, void *stack,
                              uint32_t stack_size);
void smp_stop_secondary_core(int cpuid);

void smp_startup_secondary(int cpuid);

int smp_get_core_id(void);

#define smp_send_event() do { asm volatile("sev"); } while (0)
#define smp_wait_for_event() do { asm volatile("wfe"); } while (0)

#ifdef __cplusplus
}
#endif
