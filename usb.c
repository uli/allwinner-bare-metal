#include "usb.h"
#include "uart.h"
#include "ccu.h"
#include "ports.h"
#include "system.h"

#include <tusb.h>


void usb_init() {
  // Disable clocks and wait a moment. This helps with devices not connecting on boot.
  BUS_CLK_GATING0 &= ~((1<<29)|(1<<25));
  BUS_SOFT_RST0   &= ~((1<<29)|(1<<25));
  USBPHY_CFG      &= ~((1<<17) | (1<<9) | (1<<1));
  udelay(10000);

  // Enable clocks
  BUS_CLK_GATING0 |= (1<<29)|(1<<25);
  BUS_SOFT_RST0   |= (1<<29)|(1<<25);
  USBPHY_CFG      |= (1<<17) | (1<<9) | (1<<1);
  // Enabe INCR16, INCR8, INCR4
  USB1_HCI_ICR    = 0x00000701;
  USB1_HCI_UNK1 = 0;

  tusb_init();
}

#include <common/binary.h>
#include <common/tusb_types.h>
#include "interrupts.h"

void hcd_int_enable(uint8_t rhport)
{
  irq_enable(107);
}

void hcd_int_disable(uint8_t rhport)
{
  irq_disable(107);
}

uint32_t tusb_hal_millis(void)
{
  return tick_counter * 1000 / 60;
}

static hid_keyboard_report_t usb_keyboard_report __attribute__ ((section ("UNCACHED")));

void tuh_hid_keyboard_mounted_cb(uint8_t dev_addr)
{
  // application set-up
  printf("\na Keyboard device (address %d) is mounted\n", dev_addr);
  tuh_hid_keyboard_get_report(dev_addr, (uint8_t*) &usb_keyboard_report); // first report
}

void tuh_hid_keyboard_unmounted_cb(uint8_t dev_addr)
{
  // application tear-down
  printf("\na Keyboard device (address %d) is unmounted\n", dev_addr);
}

void __attribute__((weak)) hook_usb_keyboard_report(hid_keyboard_report_t *r)
{
  printf("kbdrep %02X %02X %02X %02X\n",
	 r->keycode[0],
	 r->keycode[1],
	 r->keycode[2],
	 r->keycode[3]
  );
}

// invoked ISR context
void tuh_hid_keyboard_isr(uint8_t dev_addr, xfer_result_t event)
{
  switch(event)
  {
    case XFER_RESULT_SUCCESS:
      hook_usb_keyboard_report(&usb_keyboard_report);
      tuh_hid_keyboard_get_report(dev_addr, (uint8_t*) &usb_keyboard_report);
      break;

    case XFER_RESULT_FAILED:
      tuh_hid_keyboard_get_report(dev_addr, (uint8_t*) &usb_keyboard_report); // ignore & continue
      break;

    default :
    break;
  }
}

void tuh_hid_mouse_mounted_cb(uint8_t dev_addr)
{
  // application set-up
  printf("\na Mouse device (address %d) is mounted\n", dev_addr);
}

void tuh_hid_mouse_unmounted_cb(uint8_t dev_addr)
{
  // application tear-down
  printf("\na Mouse device (address %d) is unmounted\n", dev_addr);
}

// invoked ISR context
void tuh_hid_mouse_isr(uint8_t dev_addr, xfer_result_t event)
{
}

void usb_task(void)
{
  tusb_task();
}
