#include <stdint.h>
#include "mmu.h"
#include "smp.h"

#define CPU_REG(n) (*(volatile uint32_t *)(0x01f01c00 + (n)))

#define CPU_RST_CTRL_REG(n)	CPU_REG(0x40 + (n) * 0x40)
#define CPU_CTRL_REG(n)		CPU_REG(0x44 + (n) * 0x40)
#define CPU_STATUS_REG(n)	CPU_REG(0x48 + (n) * 0x40)

extern volatile uint32_t _smp_secondary_sp;
volatile secondary_task_t tasks[4] = {};

void smp_start_secondary_core(int cpuid, secondary_task_t task, void *stack,
                              uint32_t stack_size)
{
    tasks[cpuid] = task;
    _smp_secondary_sp = (uint32_t)stack + stack_size - 0x400;
    CPU_RST_CTRL_REG(cpuid) = 0x3;
}

void smp_stop_secondary_core(int cpuid)
{
    CPU_RST_CTRL_REG(cpuid) = 0;
}

void init_sp_irq(uint32_t addr);

#include "uart.h"
#include "system.h"

void _smp_startup_secondary(int cpuid) {
  init_sp_irq(_smp_secondary_sp + 0x400);
  mmu_init();
  uart_print("Secondary boot\r\n");
  while (1) {
    if (tasks[cpuid])
      tasks[cpuid]();
  }
}

