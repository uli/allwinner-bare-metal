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
#include <stdio.h>
#include <stdlib.h>

volatile uint32_t tick_counter;

void __libc_init_array(void);
void _init(void)
{
}

void _reset(void);
void main(int argc, char **argv);

void startup() {
  install_ivt();

  // Set up MMU and paging configuration
  mmu_init();

  init_bss();

  // Enble all GPIO
  gpio_init();

  // Configure the UART for debugging
  uart_init();
  uart_print("Booting!\r\n");

  // Illuminate the power LED
  set_pin_mode(PORTL, 10, 1); // PORT L10 output
  set_pin_data(PORTL, 10, 1); // PORT L10 high

  // Configure display
  display_init();
  display_set_mode(480, 270, 16, 16);
  audio_hdmi_init();
  audio_i2s2_init();
  audio_i2s2_on();

  sys_init_timer();

  // USB
  usb_init();

  uart_print("Ready!\r\n");

  set_pin_mode(PORTF, 6, 0);	// SD CD pin

  __libc_init_array();

  main(0, NULL);
  _reset();
}

unsigned long __dso_handle;
