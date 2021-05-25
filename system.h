#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TIMER_BASE        0x01C20C00
#define WDOG0_MODE    *(volatile uint32_t *)(TIMER_BASE + 0xB8)
void udelay(uint32_t d);

void init_bss();
extern char _bstart1;
extern char _bend1;
extern char _bstart2;
extern char _bend2;
extern char _hend;

extern volatile uint32_t tick_counter;

void sys_init_timer(void);
uint64_t sys_get_tick(void);
uint64_t sys_get_usec(void);
uint64_t sys_get_msec(void);

uint32_t sys_mem_free(void);

void sys_reset(void);

#define SYS_CPU_MULTIPLIER_MIN     1
#define SYS_CPU_MULTIPLIER_DEFAULT 84
#define SYS_CPU_MULTIPLIER_MAX     108

void sys_set_cpu_multiplier(int factor);

#ifdef __cplusplus
}
#endif
