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
bool usb3_tusb_init(void);

void usb1_diskio_init(void);
void usb2_diskio_init(void);
void usb3_diskio_init(void);

tusb_error_t usb1_tuh_hid_keyboard_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb2_tuh_hid_keyboard_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb3_tuh_hid_keyboard_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb1_tuh_hid_mouse_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb2_tuh_hid_mouse_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb3_tuh_hid_mouse_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb1_tuh_hid_generic_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb2_tuh_hid_generic_get_report(uint8_t dev_addr, void * p_report);
tusb_error_t usb3_tuh_hid_generic_get_report(uint8_t dev_addr, void * p_report);
int usb1_tuh_hid_generic_get_report_size(uint8_t dev_addr);
int usb2_tuh_hid_generic_get_report_size(uint8_t dev_addr);
int usb3_tuh_hid_generic_get_report_size(uint8_t dev_addr);
bool usb1_tuh_hidh_interface_set_report(uint8_t dev_addr, uint8_t data);
bool usb2_tuh_hidh_interface_set_report(uint8_t dev_addr, uint8_t data);
bool usb3_tuh_hidh_interface_set_report(uint8_t dev_addr, uint8_t data);

void usb1_tusb_task(void);
void usb2_tusb_task(void);
void usb3_tusb_task(void);

// BUS_CLK gating bits
#define USBEHCI1_GATING (1 << 25)
#define USBEHCI2_GATING (1 << 26)
#define USBEHCI3_GATING (1 << 27)
#define USBOHCI1_GATING (1 << 29)
#define USBOHCI2_GATING (1 << 30)
#define USBOHCI3_GATING (1 << 31)

// USBPHY_CFG bits
#define USBPHY1_RST (1 << 1)
#define USBPHY2_RST (1 << 2)
#define USBPHY3_RST (1 << 3)

#define SCLK_GATING_USBPHY1 (1 << 9)
#define SCLK_GATING_USBPHY2 (1 << 10)
#define SCLK_GATING_USBPHY3 (1 << 11)

#define SCLK_GATING_OHCI1 (1 << 17)
#define SCLK_GATING_OHCI2 (1 << 18)
#define SCLK_GATING_OHCI3 (1 << 19)

void usb_init() {
  // Disable clocks and wait a moment. This helps with devices not connecting on boot.
  BUS_CLK_GATING0 &= ~(USBOHCI3_GATING | USBOHCI2_GATING | USBOHCI1_GATING |
                       USBEHCI3_GATING | USBEHCI2_GATING | USBEHCI1_GATING);
  BUS_SOFT_RST0   &= ~(USBOHCI3_GATING | USBOHCI2_GATING | USBOHCI1_GATING |
                       USBEHCI3_GATING | USBEHCI2_GATING | USBEHCI1_GATING);
  USBPHY_CFG      &= ~(SCLK_GATING_OHCI3 | SCLK_GATING_OHCI2 | SCLK_GATING_OHCI1 |
                       SCLK_GATING_USBPHY3 | SCLK_GATING_USBPHY2 | SCLK_GATING_USBPHY1 |
                       USBPHY3_RST | USBPHY2_RST | USBPHY1_RST);
  udelay(10000);

  // Enable clocks
  BUS_CLK_GATING0 |= USBOHCI3_GATING | USBOHCI2_GATING | USBOHCI1_GATING |
                     USBEHCI3_GATING | USBEHCI2_GATING | USBEHCI1_GATING;
  BUS_SOFT_RST0   |= USBOHCI3_GATING | USBOHCI2_GATING | USBOHCI1_GATING |
                     USBEHCI3_GATING | USBEHCI2_GATING | USBEHCI1_GATING;
  USBPHY_CFG      |= SCLK_GATING_OHCI3 | SCLK_GATING_OHCI2 | SCLK_GATING_OHCI1 |
                     SCLK_GATING_USBPHY3 | SCLK_GATING_USBPHY2 | SCLK_GATING_USBPHY1 |
                     USBPHY3_RST | USBPHY2_RST | USBPHY1_RST;

  // Enable INCR16, INCR8, INCR4
  USB1_HCI_ICR  = 0x00000701;
  USB1_HCI_UNK1 = 0;
  USB2_HCI_ICR  = 0x00000701;
  USB2_HCI_UNK1 = 0;
  USB3_HCI_ICR  = 0x00000701;
  USB3_HCI_UNK1 = 0;

  usb1_tusb_init();
  usb2_tusb_init();
  usb3_tusb_init();

  usb1_diskio_init();
  usb2_diskio_init();
  usb3_diskio_init();
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

void usb3_hcd_int_enable(uint8_t rhport)
{
  (void)rhport;
  irq_enable(111);
}

void usb3_hcd_int_disable(uint8_t rhport)
{
  (void)rhport;
  irq_disable(111);
}

uint32_t tusb_hal_millis(void)
{
  return sys_get_msec();
}

static hid_keyboard_report_t usb_keyboard_report __attribute__ ((section ("UNCACHED")));

static void keyboard_get_report(int hcd, uint8_t dev_addr, uint8_t *report) {
  switch (hcd) {
  case 1: usb1_tuh_hid_keyboard_get_report(dev_addr, report); break;
  case 2: usb2_tuh_hid_keyboard_get_report(dev_addr, report); break;
  case 3: usb3_tuh_hid_keyboard_get_report(dev_addr, report); break;
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
void usb3_tuh_hid_keyboard_mounted_cb(uint8_t dev_addr) { keyboard_mounted(3, dev_addr); }

static void keyboard_unmounted(uint8_t hcd, uint8_t dev_addr)
{
  // application tear-down
  printf("\na Keyboard device (hcd %d, address %d) is unmounted\n", hcd, dev_addr);
}

void usb1_tuh_hid_keyboard_unmounted_cb(uint8_t dev_addr) { keyboard_unmounted(1, dev_addr); }
void usb2_tuh_hid_keyboard_unmounted_cb(uint8_t dev_addr) { keyboard_unmounted(2, dev_addr); }
void usb3_tuh_hid_keyboard_unmounted_cb(uint8_t dev_addr) { keyboard_unmounted(3, dev_addr); }

bool usb_keyboard_set_leds(int hcd, uint8_t dev_addr, uint8_t leds)
{
  switch (hcd) {
    case 1: return usb1_tuh_hidh_interface_set_report(dev_addr, leds);
    case 2: return usb2_tuh_hidh_interface_set_report(dev_addr, leds);
    case 3: return usb3_tuh_hidh_interface_set_report(dev_addr, leds);
    default: return false;;
  }
}

void __attribute__((weak)) hook_usb_keyboard_report(int hcd, uint8_t dev_addr, hid_keyboard_report_t *r)
{
  printf("kbdrep %d/%d %02X %02X %02X %02X\n",
         hcd, dev_addr,
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
      hook_usb_keyboard_report(hcd, dev_addr, &usb_keyboard_report);
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
void usb3_tuh_hid_keyboard_isr(uint8_t dev_addr, xfer_result_t event) { keyboard_isr(3, dev_addr, event); }

static hid_mouse_report_t usb_mouse_report __attribute__ ((section ("UNCACHED")));

static void mouse_get_report(int hcd, uint8_t dev_addr, uint8_t *report) {
  switch (hcd) {
  case 1: usb1_tuh_hid_mouse_get_report(dev_addr, report); break;
  case 2: usb2_tuh_hid_mouse_get_report(dev_addr, report); break;
  case 3: usb3_tuh_hid_mouse_get_report(dev_addr, report); break;
  default: break;
  }
}

static void mouse_mounted(int hcd, uint8_t dev_addr)
{
  // application set-up
  printf("\na Mouse device (hcd %d, address %d) is mounted\n", hcd, dev_addr);
  mouse_get_report(hcd, dev_addr, (uint8_t*) &usb_mouse_report); // first report
}
void usb1_tuh_hid_mouse_mounted_cb(uint8_t dev_addr) { mouse_mounted(1, dev_addr); }
void usb2_tuh_hid_mouse_mounted_cb(uint8_t dev_addr) { mouse_mounted(2, dev_addr); }
void usb3_tuh_hid_mouse_mounted_cb(uint8_t dev_addr) { mouse_mounted(3, dev_addr); }

static void mouse_unmounted(int hcd, uint8_t dev_addr)
{
  // application tear-down
  printf("\na Mouse device (hcd %d, address %d) is unmounted\n", hcd, dev_addr);
}
void usb1_tuh_hid_mouse_unmounted_cb(uint8_t dev_addr) { mouse_unmounted(1, dev_addr); }
void usb2_tuh_hid_mouse_unmounted_cb(uint8_t dev_addr) { mouse_unmounted(2, dev_addr); }
void usb3_tuh_hid_mouse_unmounted_cb(uint8_t dev_addr) { mouse_unmounted(3, dev_addr); }

void __attribute__((weak)) hook_usb_mouse_report(int hcd, uint8_t dev_addr, hid_mouse_report_t *r)
{
  printf("mouserep %d/%d btn 0x%x dx %d dy %d w %d\n",
         hcd, dev_addr,
	 r->buttons,
	 r->x,
	 r->y,
	 r->wheel
  );
}

// invoked ISR context
void mouse_isr(int hcd, uint8_t dev_addr, xfer_result_t event)
{
  switch(event)
  {
    case XFER_RESULT_SUCCESS:
      hook_usb_mouse_report(hcd, dev_addr, &usb_mouse_report);
      mouse_get_report(hcd, dev_addr, (uint8_t*) &usb_mouse_report);
    break;

    case XFER_RESULT_FAILED:
      mouse_get_report(hcd, dev_addr, (uint8_t*) &usb_mouse_report); // ignore & continue
    break;

    default :
    break;
  }
}
void usb1_tuh_hid_mouse_isr(uint8_t dev_addr, xfer_result_t event) { mouse_isr(1, dev_addr, event); }
void usb2_tuh_hid_mouse_isr(uint8_t dev_addr, xfer_result_t event) { mouse_isr(2, dev_addr, event); }
void usb3_tuh_hid_mouse_isr(uint8_t dev_addr, xfer_result_t event) { mouse_isr(3, dev_addr, event); }

void usb_task(void)
{
  usb1_tusb_task();
  usb2_tusb_task();
  usb3_tusb_task();
}

static hid_generic_report_t usb_generic_report __attribute__ ((section ("UNCACHED")));

static void generic_get_report(int hcd, uint8_t dev_addr, uint8_t *report) {
  switch (hcd) {
  case 1: usb1_tuh_hid_generic_get_report(dev_addr, report); break;
  case 2: usb2_tuh_hid_generic_get_report(dev_addr, report); break;
  case 3: usb3_tuh_hid_generic_get_report(dev_addr, report); break;
  default: break;
  }
}

static int generic_report_size(uint8_t hcd, uint8_t dev_addr)
{
  switch (hcd) {
  case 1: return usb1_tuh_hid_generic_get_report_size(dev_addr); break;
  case 2: return usb2_tuh_hid_generic_get_report_size(dev_addr); break;
  case 3: return usb3_tuh_hid_generic_get_report_size(dev_addr); break;
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
void usb3_tuh_hid_generic_mounted_cb(uint8_t dev_addr, uint8_t *report_desc, int report_desc_len) { generic_mounted(3, dev_addr, report_desc, report_desc_len); }

void __attribute__((weak)) hook_usb_generic_unmounted(int hcd, uint8_t dev_addr)
{
  (void)hcd; (void)dev_addr;
}

static void generic_unmounted(uint8_t hcd, uint8_t dev_addr)
{
  // application tear-down
  printf("\na generic device (hcd %d, address %d) is unmounted\n", hcd, dev_addr);
  hook_usb_generic_unmounted(hcd, dev_addr);
}

void usb1_tuh_hid_generic_unmounted_cb(uint8_t dev_addr) { generic_unmounted(1, dev_addr); }
void usb2_tuh_hid_generic_unmounted_cb(uint8_t dev_addr) { generic_unmounted(2, dev_addr); }
void usb3_tuh_hid_generic_unmounted_cb(uint8_t dev_addr) { generic_unmounted(3, dev_addr); }

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
void usb3_tuh_hid_generic_isr(uint8_t dev_addr, xfer_result_t event) { generic_isr(3, dev_addr, event); }

static void msc_isr(int hcd, uint8_t dev_addr, xfer_result_t event, uint32_t xferred_bytes)
{
  (void) hcd;  (void) dev_addr;  (void) event;  (void) xferred_bytes;
}

void usb1_tuh_msc_isr(uint8_t dev_addr, xfer_result_t event, uint32_t xferred_bytes) { msc_isr(1, dev_addr, event, xferred_bytes); }
void usb2_tuh_msc_isr(uint8_t dev_addr, xfer_result_t event, uint32_t xferred_bytes) { msc_isr(2, dev_addr, event, xferred_bytes); }
void usb3_tuh_msc_isr(uint8_t dev_addr, xfer_result_t event, uint32_t xferred_bytes) { msc_isr(3, dev_addr, event, xferred_bytes); }

#include "fatfs/ff.h"

// one mount point per host controller;
static FATFS fatfs[4];
static struct usb_drive_t {
  const char *name;
  int hcd;
  uint8_t dev_addr;
} usb_drives[] = {
  { "/usb0", 0, 0 },
  { "/usb1", 0, 0 },
  { "/usb2", 0, 0 },
  { "/usb3", 0, 0 },
};

uint8_t usb1_disk_initialize(BYTE pdrv);
uint8_t usb2_disk_initialize(BYTE pdrv);
uint8_t usb3_disk_initialize(BYTE pdrv);
uint8_t usb1_disk_deinitialize(BYTE pdrv);
uint8_t usb2_disk_deinitialize(BYTE pdrv);
uint8_t usb3_disk_deinitialize(BYTE pdrv);

void msc_mounted_cb(int hcd, uint8_t dev_addr)
{
  printf("\na MassStorage device is mounted, hcd %d, dev_addr %d\n", hcd, dev_addr);
  int logical_vol = -1;
  for (int i = 0; i < 4; i++) {
    if (usb_drives[i].dev_addr == 0) {
      logical_vol = i;
      break;
    }
  }

  if (logical_vol == -1) {
    printf("max number of USB drives reached\n");
    return;
  }

  usb_drives[logical_vol].dev_addr = dev_addr;
  usb_drives[logical_vol].hcd = hcd;

  switch (hcd) {
    case 1: usb1_disk_initialize(dev_addr - 1); break;
    case 2: usb2_disk_initialize(dev_addr - 1); break;
    case 3: usb3_disk_initialize(dev_addr - 1); break;
  };

  // XXX: cannot use disk_is_ready(), it's an inline function and not prefixed with usb?_
  int res;
  if ((res = f_mount(&fatfs[logical_vol], usb_drives[logical_vol].name, 1)) != FR_OK) {
    printf("mount failed, %d\n", res);
  }
}

void usb1_tuh_msc_mounted_cb(uint8_t dev_addr) { msc_mounted_cb(1, dev_addr); }
void usb2_tuh_msc_mounted_cb(uint8_t dev_addr) { msc_mounted_cb(2, dev_addr); }
void usb3_tuh_msc_mounted_cb(uint8_t dev_addr) { msc_mounted_cb(3, dev_addr); }

static struct usb_drive_t *get_logical_volume(int hcd, uint8_t dev_addr)
{
  for (int i = 0; i < 4; i++) {
    if (usb_drives[i].hcd == hcd && usb_drives[i].dev_addr == dev_addr)
      return &usb_drives[i];
  }

  printf("unknown drive\n");
  return NULL;
}

void msc_unmounted_cb(int hcd, uint8_t dev_addr)
{
  printf("\na MassStorage device is unmounted, hcd %d, dev_addr %d\n", hcd, dev_addr);

  struct usb_drive_t *dr = get_logical_volume(hcd, dev_addr);
  if (!dr)
    return;

  f_unmount(dr->name);
  dr->dev_addr = 0;

  switch (hcd) {
    case 1: usb1_disk_deinitialize(dev_addr - 1); break;
    case 2: usb2_disk_deinitialize(dev_addr - 1); break;
    case 3: usb3_disk_deinitialize(dev_addr - 1); break;
  };
}

void usb1_tuh_msc_unmounted_cb(uint8_t dev_addr) { msc_unmounted_cb(1, dev_addr); }
void usb2_tuh_msc_unmounted_cb(uint8_t dev_addr) { msc_unmounted_cb(2, dev_addr); }
void usb3_tuh_msc_unmounted_cb(uint8_t dev_addr) { msc_unmounted_cb(3, dev_addr); }

#include "fatfs/diskio.h"

DRESULT usb1_disk_read ( BYTE pdrv, BYTE *buff, DWORD sector, UINT count );
DRESULT usb2_disk_read ( BYTE pdrv, BYTE *buff, DWORD sector, UINT count );
DRESULT usb3_disk_read ( BYTE pdrv, BYTE *buff, DWORD sector, UINT count );

DRESULT usb_disk_read ( BYTE pdrv, BYTE *buff, DWORD sector, UINT count )
{
  struct usb_drive_t *dr = &usb_drives[pdrv - 2];

  switch (dr->hcd) {
    case 1: return usb1_disk_read(dr->dev_addr - 1, buff, sector, count);
    case 2: return usb2_disk_read(dr->dev_addr - 1, buff, sector, count);
    case 3: return usb3_disk_read(dr->dev_addr - 1, buff, sector, count);
  }

  return RES_NOTRDY;
}

DRESULT usb1_disk_write ( BYTE pdrv, const BYTE *buff, DWORD sector, UINT count );
DRESULT usb2_disk_write ( BYTE pdrv, const BYTE *buff, DWORD sector, UINT count );
DRESULT usb3_disk_write ( BYTE pdrv, const BYTE *buff, DWORD sector, UINT count );

DRESULT usb_disk_write ( BYTE pdrv, const BYTE *buff, DWORD sector, UINT count )
{
  struct usb_drive_t *dr = &usb_drives[pdrv - 2];

  switch (dr->hcd) {
    case 1: return usb1_disk_write(dr->dev_addr - 1, buff, sector, count);
    case 2: return usb2_disk_write(dr->dev_addr - 1, buff, sector, count);
    case 3: return usb3_disk_write(dr->dev_addr - 1, buff, sector, count);
  }

  return RES_NOTRDY;
}

DSTATUS usb1_disk_status(BYTE pdrv);
DSTATUS usb2_disk_status(BYTE pdrv);
DSTATUS usb3_disk_status(BYTE pdrv);

DSTATUS usb_disk_status(BYTE pdrv)
{
  struct usb_drive_t *dr = &usb_drives[pdrv - 2];

  switch (dr->hcd) {
    case 1: return usb1_disk_status(dr->dev_addr - 1);
    case 2: return usb2_disk_status(dr->dev_addr - 1);
    case 3: return usb3_disk_status(dr->dev_addr - 1);
  }

  return STA_NOINIT;
}

DSTATUS usb1_disk_initialize(BYTE pdrv);
DSTATUS usb2_disk_initialize(BYTE pdrv);
DSTATUS usb3_disk_initialize(BYTE pdrv);

DSTATUS usb_disk_initialize(BYTE pdrv)
{
  struct usb_drive_t *dr = &usb_drives[pdrv - 2];

  switch (dr->hcd) {
    case 1: return usb1_disk_initialize(dr->dev_addr - 1);
    case 2: return usb2_disk_initialize(dr->dev_addr - 1);
    case 3: return usb3_disk_initialize(dr->dev_addr - 1);
  }

  return STA_NOINIT;
}

DRESULT usb1_disk_ioctl ( BYTE pdrv, BYTE cmd, void *buff );
DRESULT usb2_disk_ioctl ( BYTE pdrv, BYTE cmd, void *buff );
DRESULT usb3_disk_ioctl ( BYTE pdrv, BYTE cmd, void *buff );

DRESULT usb_disk_ioctl ( BYTE pdrv, BYTE cmd, void *buff )
{
  struct usb_drive_t *dr = &usb_drives[pdrv - 2];

  switch (dr->hcd) {
    case 1: return usb1_disk_ioctl(dr->dev_addr - 1, cmd, buff);
    case 2: return usb2_disk_ioctl(dr->dev_addr - 1, cmd, buff);
    case 3: return usb3_disk_ioctl(dr->dev_addr - 1, cmd, buff);
  }

  return RES_NOTRDY;
}
