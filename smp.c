// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

//#define SMP_DEBUG

#include <stdint.h>

#include "mmu.h"
#include "smp.h"

#define CPU_REG(n) (*(volatile uint32_t *)(0x01f01c00 + (n)))

#define CPU_RST_CTRL_REG(n) CPU_REG(0x40 + (n)*0x40)
#define CPU_CTRL_REG(n)     CPU_REG(0x44 + (n)*0x40)
#define CPU_STATUS_REG(n)   CPU_REG(0x48 + (n)*0x40)

extern volatile uint32_t _smp_secondary_sp;
volatile secondary_task_t tasks[4] = {};

void smp_start_secondary_core(int cpuid, secondary_task_t task, void *stack, uint32_t stack_size)
{
  tasks[cpuid]            = task;
  _smp_secondary_sp       = (uint32_t)stack + stack_size - 0x400;

#ifdef JAILHOUSE
  // Use PSCI to bring up the core
  // We use the "unused" vector in the loader program as entry point.
#ifdef SMP_DEBUG
  printf("%s starting core %d stack %p size %d (0x%x)\n", __FUNCTION__, cpuid, stack, stack_size, _smp_secondary_sp);
#endif
  asm("movw r0, #0xba60\n"
      "movt r0, #0x95c1\n"	// PSCI_CPU_ON_V0_1_UBOOT
      "mov r1, %0\n"		// cpuid
      "mov r2, %1\n"		// start address
      "mov r3, #0\n"		// "context", but that context doesn't
                                // include the SP for some reason, so it's pretty useless
      "smc #0\n" : : "r"(cpuid), "r"(0x00000014) : "r0", "r1", "r2", "r3");
#else
  CPU_RST_CTRL_REG(cpuid) = 0x3;
#endif
}

void smp_stop_secondary_core(int cpuid)
{
  CPU_RST_CTRL_REG(cpuid) = 0;
}

void init_sp_irq(uint32_t addr);

#include "system.h"
#include "uart.h"

void _smp_startup_secondary(int cpuid)
{
#ifdef SMP_DEBUG
  register volatile uint32_t sp asm("sp");
#endif
  init_sp_irq(_smp_secondary_sp + 0x400);

#ifndef JAILHOUSE
  mmu_init();
#endif
  uart_print("Secondary boot\r\n");
#ifdef SMP_DEBUG
  uart_print_uint32(sp);uart_print_uint32(_smp_secondary_sp);
#endif
  while (1) {
    if (tasks[cpuid])
      tasks[cpuid]();
  }
}
