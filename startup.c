#include "audio.h"
#include "ccu.h"
#include "display.h"
#include "dma.h"
#include "fs.h"
#include "interrupts.h"
#include "mmu.h"
#include "network.h"
#include "ports.h"
#include "smp.h"
#include "system.h"
#include "tve.h"
#include "uart.h"
#include "usb.h"

#include <stdio.h>
#include <stdlib.h>

#include <h3_i2c.h>
#include <h3_spi.h>

#ifdef GDBSTUB
#include "gdb/gdbstub.h"
#endif


volatile uint32_t tick_counter;

void libc_set_heap(void *start, void *end);

void __libc_init_array(void);
void _init(void) {}

void _reset(void);
void init_sp_irq(uint32_t addr);
void main(int argc, char **argv);

void h3_timer_init(void);
void h3_hs_timer_init(void);

void startup()
{
#ifdef JAILHOUSE
  // SP_irq set up by loader
#else
  init_sp_irq(0x2000);
#endif

  // detect memory size
#ifdef GDBSTUB
  libc_set_heap((void *)0x42000000, (void *)0x60000000);
#elif defined(JAILHOUSE)
  // MMU is already on, so memory detection needs to do cache management to
  // work, so it needs to know the cache line size, so we need to call
  // mmu_init() beforehand.
  mmu_init();
  libc_set_heap((void *)0x4b000000, mmu_detect_dram_end());
#else
  // XXX: check why this doesn't work with the stub enabled
  libc_set_heap((void *)0x42000000, mmu_detect_dram_end());
#endif

  install_ivt();

#ifndef JAILHOUSE
  uart_init(0);
#endif

#ifdef GDBSTUB
  gdbstub_init();
#endif

#ifndef JAILHOUSE
  // Set up MMU and paging configuration
  mmu_init();
#endif

#ifndef JAILHOUSE
  // Enble all GPIO
  gpio_init();
#endif

  // Configure the UART for debugging
  uart_print("Booting!\r\n");

  // Illuminate the power LED
  set_pin_mode(PORTL, 10, 1);  // PORT L10 output
  set_pin_data(PORTL, 10, 1);  // PORT L10 high

  // reset button
  set_pin_mode(PORTL, 3, GPIO_MODE_EINT);
  gpio_irq_set_trigger(PORTL, 3, GPIO_EINT_NEGEDGE);
  gpio_irq_enable(PORTL, 3, 1);
  irq_enable(77);  // PORT L interrupt (R_PL_EINT)

#ifndef JAILHOUSE	// XXX??
  dma_init();
#endif
  h3_timer_init();
  h3_hs_timer_init();

  // Configure display; try HDMI/DVI first, fall back to analog if it fails to initialize
  // XXX: We have to init the display because the system timer
  // initialization uses it for calibration.
  if (display_init(NULL) != 0)
    tve_init(TVE_NORM_NTSC);
  else
    audio_hdmi_init();

  sys_init_timer();

#ifndef JAILHOUSE
  // USB
  usb_init();

  network_init();
#endif

  h3_i2c_begin();
  h3_spi_begin();

  uart_print("Ready!\r\n");

#ifndef JAILHOUSE
  set_pin_mode(PORTF, 6, 0);  // SD CD pin
#endif

  __libc_init_array();

  main(0, NULL);
  _reset();
}

unsigned long __dso_handle;
