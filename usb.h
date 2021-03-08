#ifndef __USB_H
#define __USB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <tusb.h>

#define USB0_BASE 0x01C1A000

#define USB1_BASE 0x01C1B000

#define USB1_E_CAPLENGTH           *(volatile uint8_t  *)(USB1_BASE + 0x000)
#define USB1_E_HCIVERSION          *(volatile uint16_t *)(USB1_BASE + 0x002)
#define USB1_E_HCSPARAMS           *(volatile uint32_t *)(USB1_BASE + 0x004)
#define USB1_E_HCCPARAMS           *(volatile uint32_t *)(USB1_BASE + 0x008)
#define USB1_E_HCSPPORTROUTE       *(volatile uint32_t *)(USB1_BASE + 0x00C)

#define USB1_E_USBCMD              *(volatile uint32_t *)(USB1_BASE + 0x010)
#define USB1_E_USBSTS              *(volatile uint32_t *)(USB1_BASE + 0x014)
#define USB1_E_USBINTR             *(volatile uint32_t *)(USB1_BASE + 0x018)
#define USB1_E_FRINDEX             *(volatile uint32_t *)(USB1_BASE + 0x01C)
#define USB1_E_CTRLDSSEGMENT       *(volatile uint32_t *)(USB1_BASE + 0x020)
#define USB1_E_PERIODICLISTBASE    *(volatile uint32_t *)(USB1_BASE + 0x024)
#define USB1_E_ASYNCLISTADDR       *(volatile uint32_t *)(USB1_BASE + 0x028)
#define USB1_E_CONFIGFLAG          *(volatile uint32_t *)(USB1_BASE + 0x050)
#define USB1_E_PORTSC              *(volatile uint32_t *)(USB1_BASE + 0x054)

#define USB1_O_HCREVISION          *(volatile uint32_t *)(USB1_BASE + 0x400)
#define USB1_O_HCCONTROL           *(volatile uint32_t *)(USB1_BASE + 0x404)
#define USB1_O_HCCOMMANDSTATUS     *(volatile uint32_t *)(USB1_BASE + 0x408)
#define USB1_O_HCINTERRUPTSTATUS   *(volatile uint32_t *)(USB1_BASE + 0x40C)
#define USB1_O_HCINTERRUPTENABLE   *(volatile uint32_t *)(USB1_BASE + 0x410)
#define USB1_O_HCINTERRUPTDISABLE  *(volatile uint32_t *)(USB1_BASE + 0x414)

#define USB1_O_HCHCCA              *(volatile uint32_t *)(USB1_BASE + 0x418)
#define USB1_O_HCPERIODCURRENTED   *(volatile uint32_t *)(USB1_BASE + 0x41C)
#define USB1_O_HCCONTROLHEADED     *(volatile uint32_t *)(USB1_BASE + 0x420)
#define USB1_O_HCCONTROLCURRENTED  *(volatile uint32_t *)(USB1_BASE + 0x424)
#define USB1_O_HCBULKHEADED        *(volatile uint32_t *)(USB1_BASE + 0x428)
#define USB1_O_HCBULKCURRENTED     *(volatile uint32_t *)(USB1_BASE + 0x42C)
#define USB1_O_HCDONEHEAD          *(volatile uint32_t *)(USB1_BASE + 0x430)

#define USB1_O_HCFMINTERVAL        *(volatile uint32_t *)(USB1_BASE + 0x434)
#define USB1_O_HCFMREMAINING       *(volatile uint32_t *)(USB1_BASE + 0x438)
#define USB1_O_HCFMNUMBER          *(volatile uint32_t *)(USB1_BASE + 0x43C)
#define USB1_O_HCPERIODDICSTART    *(volatile uint32_t *)(USB1_BASE + 0x440)
#define USB1_O_HCLSTHRESHOLD       *(volatile uint32_t *)(USB1_BASE + 0x444)

#define USB1_O_HCRHDESCRIPTORA     *(volatile uint32_t *)(USB1_BASE + 0x448)
#define USB1_O_HCRHDESCRIPTORB     *(volatile uint32_t *)(USB1_BASE + 0x44C)
#define USB1_O_HCRHSTATUS          *(volatile uint32_t *)(USB1_BASE + 0x450)
#define USB1_O_HCRHPORTSTATUS      *(volatile uint32_t *)(USB1_BASE + 0x454)

#define USB1_HCI_ICR               *(volatile uint32_t *)(USB1_BASE + 0x800)
#define USB1_HSIC_STATUS           *(volatile uint32_t *)(USB1_BASE + 0x804)
#define USB1_HCI_UNK1              *(volatile uint32_t *)(USB1_BASE + 0x810)

#define USB2_BASE 0x01C1C000
#define USB2_HCI_ICR               *(volatile uint32_t *)(USB2_BASE + 0x800)
#define USB2_HSIC_STATUS           *(volatile uint32_t *)(USB2_BASE + 0x804)
#define USB2_HCI_UNK1              *(volatile uint32_t *)(USB2_BASE + 0x810)

#define USB_PHY_CFG                *(volatile uint32_t *)0x01c19410

struct td {
  uint32_t info;
  uint32_t cbp;
  uint32_t nexttd;
  uint32_t bufferend;
} __attribute__ ((aligned (16)));

struct ed {
  uint32_t info;
  struct td* tailp; 
  struct td* headp; 
  struct ed* nexted; 
} __attribute__ ((aligned (16)));

struct hcca {
  uint32_t HccaInterrruptTable[32];
  uint16_t HccaFrameNumber;
  uint16_t HccaPad1;
  uint32_t HccaDoneHead;
  uint8_t reserved[120];
} __attribute__ ((aligned (256)));

void usb_init();
void usb_task();

void hook_usb_keyboard_report(hid_keyboard_report_t *);
void hook_usb_generic_report(int hcd, uint8_t dev_addr, hid_generic_report_t *data);
void hook_usb_generic_mounted(int hcd, uint8_t dev_addr, uint8_t *report_desc, int report_desc_len);
void hook_usb_generic_unmounted(int hcd, uint8_t dev_addr);

#ifdef __cplusplus
}
#endif

#endif
