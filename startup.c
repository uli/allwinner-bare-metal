#include "ports.h"
#include "uart.h"
#include "mmu.h"
#include "system.h"
#include "display.h"
#include "audio.h"
#include "interrupts.h"
#include "ccu.h"
#include "usb.h"
#include "fs.h"
#include "dma.h"
#include "smp.h"
#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef GDBSTUB
#include "gdb/gdbstub.h"
#endif

#include <h3_codec.h>

volatile uint32_t tick_counter;

void libc_set_heap(void *start, void *end);

void __libc_init_array(void);
void _init(void)
{
}

void _reset(void);
void init_sp_irq(uint32_t addr);
void main(int argc, char **argv);

extern void h3_timer_init(void);

void startup() {
  init_sp_irq(0x2000);

  // detect memory size
#ifdef GDBSTUB
  libc_set_heap((void *)0x42000000, (void *)0x60000000);
#else
  // XXX: check why this doesn't work with the stub enabled
  libc_set_heap((void *)0x42000000, mmu_detect_dram_end());
#endif

  install_ivt();

  uart_init();
#ifdef GDBSTUB
  gdbstub_init();
#endif

  // Set up MMU and paging configuration
  mmu_init();

  // Enble all GPIO
  gpio_init();

  // Configure the UART for debugging
  uart_print("Booting!\r\n");

  // Illuminate the power LED
  set_pin_mode(PORTL, 10, 1); // PORT L10 output
  set_pin_data(PORTL, 10, 1); // PORT L10 high

  dma_init();

  // Configure display
  // We have to init the display because the system timer initialization
  // uses it for calibration.
  display_init(NULL);

  sys_init_timer();
  h3_timer_init();

  // USB
  usb_init();

  network_init();

  uart_print("Ready!\r\n");

  set_pin_mode(PORTF, 6, 0);	// SD CD pin

  __libc_init_array();

  main(0, NULL);
  _reset();
}

unsigned long __dso_handle;
