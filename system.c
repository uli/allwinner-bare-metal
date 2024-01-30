#include <stdio.h>
#include <h3_watchdog.h>
#include "ccu.h"
#include "system.h"
#include "uart.h"

#ifdef AWBM_PLATFORM_h3
// used by the lib-h3 EMAC driver
uint8_t libh3_coherent_region[1048576]  __attribute__ ((section ("UNCACHED")));
#endif

static uint32_t sys_freq;
static uint32_t sys_per_usec;

// used before the stack guard is set up
void __attribute__((optimize("-fno-stack-protector"))) init_bss()
{
  for (char *dst = &_bstart1; dst < &_bend1; dst++)
    *dst = 0;
  for (char *dst = &_bstart2; dst < &_bend2; dst++)
    *dst = 0;
}

void udelay(uint32_t d)
{
  for (uint32_t n = 0; n < d * 200; n++)
    asm("NOP");
}

uint64_t sys_get_tick(void)
{
  uint32_t low, high;
  uint64_t tick;
  asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(low), "=r"(high));
  tick = ((uint64_t)high << 32) | low;
  return tick;
}

uint64_t sys_get_usec(void)
{
  return sys_get_tick() / sys_per_usec;
}

uint64_t sys_get_msec(void)
{
  return sys_get_tick() / sys_per_usec / 1000;
}

void sys_init_timer(void)
{
  uint32_t t;
  uint64_t t1, t2;

  // arch timer doesn't seem to need initialization

  t = tick_counter;
  while (t == tick_counter) {}

  t1 = sys_get_tick();

  t = tick_counter;
  while (t == tick_counter) {}

  t2 = sys_get_tick();

  sys_freq     = (t2 - t1) * 60;
  sys_per_usec = sys_freq / 1000000;
  printf("freq %lu per usec %lu\n", sys_freq, sys_per_usec);
}

#include <malloc.h>

extern void *current_brk;
extern void *max_brk;

uint32_t sys_mem_free(void)
{
  struct mallinfo mi = mallinfo();
  return mi.fordblks + (max_brk - current_brk);
}

void sys_reset(void)
{
#ifdef AWBM_PLATFORM_h3	// XXX: "&& !JAILHOUSE"?
  h3_watchdog_enable();
#endif
  uart_print("reset!\r\n");
  for (;;);
}

void sys_set_cpu_multiplier(int factor)
{
#ifdef AWBM_PLATFORM_h3
  if (factor > SYS_CPU_MULTIPLIER_MAX)
    factor = SYS_CPU_MULTIPLIER_MAX;
  else if (factor < SYS_CPU_MULTIPLIER_MIN)
    factor = SYS_CPU_MULTIPLIER_MIN;

  uint32_t factor_n = factor;
  uint32_t factor_k = 1;
  while (factor_n > 32) {
    factor_k++;
    factor_n = factor / factor_k;
  }

  factor_n--;
  factor_k--;

  PLL_CPUX_CTRL = (PLL_CPUX_CTRL & ~(PLL_CPUX_FACTOR_K_MASK | PLL_CPUX_FACTOR_N_MASK)) |
    (factor_n << PLL_CPUX_FACTOR_N_SHIFT) | (factor_k << PLL_CPUX_FACTOR_K_SHIFT);
#else
#warning unimplemented
#endif
}
