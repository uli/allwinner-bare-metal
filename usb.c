#include "usb.h"
#include "uart.h"
#include "ccu.h"
#include "ports.h"
#include "system.h"

/*
 * This uses tinyusb in an especially hacky way. Tinyusb only supports one
 * host controller, and the codebase is not easy to fix regarding that. The
 * host stack has been described as "under rework" for years, so that is not
 * likely to get fixed anytime soon.
 *
 * As a workaround, this implementation uses some preprocessor trickery to
 * compile (almost) the entire tinyusb library once for each host
 * controller, each time redefining its function names with a different
 * prefix (usb#_). Is this hacky? Yes. Is it worse than having to fix the
 * host stack code? No.
 */

#include <tusb.h>

bool usb1_tusb_init(void);
bool usb2_tusb_init(void);

tusb_error_t usb1_tuh_hid_keyboard_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb2_tuh_hid_keyboard_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb1_tuh_hid_generic_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb2_tuh_hid_generic_get_report(uint8_t dev_addr, void * p_report);
int usb1_tuh_hid_generic_get_report_size(uint8_t dev_addr);
int usb2_tuh_hid_generic_get_report_size(uint8_t dev_addr);

void usb1_tusb_task(void);
void usb2_tusb_task(void);

// BUS_CLK gating bits
#define USBEHCI1_GATING (1 << 25)
#define USBEHCI2_GATING (1 << 26)
#define USBOHCI1_GATING (1 << 29)
#define USBOHCI2_GATING (1 << 30)

// USBPHY_CFG bits
#define USBPHY1_RST (1 << 1)
#define USBPHY2_RST (1 << 2)

#define SCLK_GATING_USBPHY1 (1 << 9)
#define SCLK_GATING_USBPHY2 (1 << 10)

#define SCLK_GATING_OHCI1 (1 << 17)
#define SCLK_GATING_OHCI2 (1 << 18)

void usb_init() {
  // Disable clocks and wait a moment. This helps with devices not connecting on boot.
  BUS_CLK_GATING0 &= ~(USBOHCI2_GATING | USBOHCI1_GATING | USBEHCI2_GATING | USBEHCI1_GATING);
  BUS_SOFT_RST0   &= ~(USBOHCI2_GATING | USBOHCI1_GATING | USBEHCI2_GATING | USBEHCI1_GATING);
  USBPHY_CFG      &= ~(SCLK_GATING_OHCI2 | SCLK_GATING_OHCI1 | SCLK_GATING_USBPHY2 | SCLK_GATING_USBPHY1 | USBPHY2_RST | USBPHY1_RST);
  udelay(10000);

  // Enable clocks
  BUS_CLK_GATING0 |= USBOHCI2_GATING | USBOHCI1_GATING | USBEHCI2_GATING | USBEHCI1_GATING;
  BUS_SOFT_RST0   |= USBOHCI2_GATING | USBOHCI1_GATING | USBEHCI2_GATING | USBEHCI1_GATING;
  USBPHY_CFG      |= SCLK_GATING_OHCI2 | SCLK_GATING_OHCI1 | SCLK_GATING_USBPHY2 | SCLK_GATING_USBPHY1 | USBPHY2_RST | USBPHY1_RST;

  // Enable INCR16, INCR8, INCR4
  USB1_HCI_ICR  = 0x00000701;
  USB1_HCI_UNK1 = 0;
  USB2_HCI_ICR  = 0x00000701;
  USB2_HCI_UNK1 = 0;

  usb1_tusb_init();
  usb2_tusb_init();
}

#include <common/binary.h>
#include <common/tusb_types.h>
#include "interrupts.h"

void usb1_hcd_int_enable(uint8_t rhport)
{
  (void)rhport;		// bogus, always zero
  irq_enable(107);
}

void usb1_hcd_int_disable(uint8_t rhport)
{
  (void)rhport;
  irq_disable(107);
}

void usb2_hcd_int_enable(uint8_t rhport)
{
  (void)rhport;
  irq_enable(109);
}

void usb2_hcd_int_disable(uint8_t rhport)
{
  (void)rhport;
  irq_disable(109);
}

uint32_t tusb_hal_millis(void)
{
  return tick_counter * 1000 / 60;
}

static hid_keyboard_report_t usb_keyboard_report __attribute__ ((section ("UNCACHED")));

static void keyboard_get_report(int hcd, uint8_t dev_addr, uint8_t *report) {
  switch (hcd) {
  case 1: usb1_tuh_hid_keyboard_get_report(dev_addr, report); break;
  case 2: usb2_tuh_hid_keyboard_get_report(dev_addr, report); break;
  default: break;
  }
}

static void keyboard_mounted(uint8_t hcd, uint8_t dev_addr)
{
  // application set-up
  printf("\na Keyboard device (hcd %d, address %d) is mounted\n", hcd, dev_addr);
  keyboard_get_report(hcd, dev_addr, (uint8_t*) &usb_keyboard_report); // first report
}

void usb1_tuh_hid_keyboard_mounted_cb(uint8_t dev_addr) { keyboard_mounted(1, dev_addr); }
void usb2_tuh_hid_keyboard_mounted_cb(uint8_t dev_addr) { keyboard_mounted(2, dev_addr); }

static void keyboard_unmounted(uint8_t hcd, uint8_t dev_addr)
{
  // application tear-down
  printf("\na Keyboard device (hcd %d, address %d) is unmounted\n", hcd, dev_addr);
}

void usb1_tuh_hid_keyboard_unmounted_cb(uint8_t dev_addr) { keyboard_unmounted(1, dev_addr); }
void usb2_tuh_hid_keyboard_unmounted_cb(uint8_t dev_addr) { keyboard_unmounted(2, dev_addr); }

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
static void keyboard_isr(int hcd, uint8_t dev_addr, xfer_result_t event)
{
  switch(event)
  {
    case XFER_RESULT_SUCCESS:
      hook_usb_keyboard_report(&usb_keyboard_report);
      keyboard_get_report(hcd, dev_addr, (uint8_t*) &usb_keyboard_report);
      break;

    case XFER_RESULT_FAILED:
      keyboard_get_report(hcd, dev_addr, (uint8_t*) &usb_keyboard_report); // ignore & continue
      break;

    default :
    break;
  }
}
void usb1_tuh_hid_keyboard_isr(uint8_t dev_addr, xfer_result_t event) { keyboard_isr(1, dev_addr, event); }
void usb2_tuh_hid_keyboard_isr(uint8_t dev_addr, xfer_result_t event) { keyboard_isr(2, dev_addr, event); }

static void mouse_mounted(int hcd, uint8_t dev_addr)
{
  // application set-up
  printf("\na Mouse device (hcd %d, address %d) is mounted\n", hcd, dev_addr);
}
void usb1_tuh_hid_mouse_mounted_cb(uint8_t dev_addr) { mouse_mounted(1, dev_addr); }
void usb2_tuh_hid_mouse_mounted_cb(uint8_t dev_addr) { mouse_mounted(2, dev_addr); }

static void mouse_unmounted(int hcd, uint8_t dev_addr)
{
  // application tear-down
  printf("\na Mouse device (hcd %d, address %d) is unmounted\n", hcd, dev_addr);
}
void usb1_tuh_hid_mouse_unmounted_cb(uint8_t dev_addr) { mouse_unmounted(1, dev_addr); }
void usb2_tuh_hid_mouse_unmounted_cb(uint8_t dev_addr) { mouse_unmounted(2, dev_addr); }

// invoked ISR context
void mouse_isr(int hcd, uint8_t dev_addr, xfer_result_t event)
{
}
void usb1_tuh_hid_mouse_isr(uint8_t dev_addr, xfer_result_t event) { mouse_isr(1, dev_addr, event); }
void usb2_tuh_hid_mouse_isr(uint8_t dev_addr, xfer_result_t event) { mouse_isr(2, dev_addr, event); }

void usb_task(void)
{
  usb1_tusb_task();
  usb2_tusb_task();
}

static hid_generic_report_t usb_generic_report __attribute__ ((section ("UNCACHED")));

static void generic_get_report(int hcd, uint8_t dev_addr, uint8_t *report) {
  switch (hcd) {
  case 1: usb1_tuh_hid_generic_get_report(dev_addr, report); break;
  case 2: usb2_tuh_hid_generic_get_report(dev_addr, report); break;
  default: break;
  }
}

static int generic_report_size(uint8_t hcd, uint8_t dev_addr)
{
  switch (hcd) {
  case 1: return usb1_tuh_hid_generic_get_report_size(dev_addr); break;
  case 2: return usb2_tuh_hid_generic_get_report_size(dev_addr); break;
  default: return -1;
  }
}

void __attribute__((weak)) hook_usb_generic_mounted(int hcd, uint8_t dev_addr, uint8_t *report_desc, int report_desc_len)
{
  (void)hcd; (void)dev_addr; (void)report_desc; (void)report_desc_len;
}

static void generic_mounted(uint8_t hcd, uint8_t dev_addr, uint8_t *report_desc, int report_desc_len)
{
  // application set-up
  printf("\na generic device (hcd %d, address %d) is mounted\n", hcd, dev_addr);
  hook_usb_generic_mounted(hcd, dev_addr, report_desc, report_desc_len);
  generic_get_report(hcd, dev_addr, (uint8_t*) &usb_generic_report); // first report
}

void usb1_tuh_hid_generic_mounted_cb(uint8_t dev_addr, uint8_t *report_desc, int report_desc_len) { generic_mounted(1, dev_addr, report_desc, report_desc_len); }
void usb2_tuh_hid_generic_mounted_cb(uint8_t dev_addr, uint8_t *report_desc, int report_desc_len) { generic_mounted(2, dev_addr, report_desc, report_desc_len); }

static void generic_unmounted(uint8_t hcd, uint8_t dev_addr)
{
  // application tear-down
  printf("\na generic device (hcd %d, address %d) is unmounted\n", hcd, dev_addr);
}

void usb1_tuh_hid_generic_unmounted_cb(uint8_t dev_addr) { generic_unmounted(1, dev_addr); }
void usb2_tuh_hid_generic_unmounted_cb(uint8_t dev_addr) { generic_unmounted(2, dev_addr); }

void __attribute__((weak)) hook_usb_generic_report(int hcd, uint8_t dev_addr, hid_generic_report_t *r)
{
  printf("genrep %d/%d sz %d %02X %02X %02X %02X\n", hcd, dev_addr, generic_report_size(hcd, dev_addr),
	 r->data[0],
	 r->data[1],
	 r->data[2],
	 r->data[3]
  );
}

// invoked ISR context
static void generic_isr(int hcd, uint8_t dev_addr, xfer_result_t event)
{
  switch(event)
  {
    case XFER_RESULT_SUCCESS:
      hook_usb_generic_report(hcd, dev_addr, &usb_generic_report);
      generic_get_report(hcd, dev_addr, (uint8_t*) &usb_generic_report);
      break;

    case XFER_RESULT_FAILED:
      generic_get_report(hcd, dev_addr, (uint8_t*) &usb_generic_report); // ignore & continue
      break;

    default :
    break;
  }
}
void usb1_tuh_hid_generic_isr(uint8_t dev_addr, xfer_result_t event) { generic_isr(1, dev_addr, event); }
void usb2_tuh_hid_generic_isr(uint8_t dev_addr, xfer_result_t event) { generic_isr(2, dev_addr, event); }
