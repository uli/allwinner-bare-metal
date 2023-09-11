#include <stdint.h>
#include <stdio.h>

#include "audio.h"
#include "display.h"
#include "interrupts.h"
#include "ports.h"
#include "system.h"
#include "uart.h"
#include "util.h"

extern uint32_t _ivt;

void usb1_hal_hcd_isr(uint8_t hostid);
void usb2_hal_hcd_isr(uint8_t hostid);
void usb3_hal_hcd_isr(uint8_t hostid);

int irq_pending(uint32_t irq)
{
  volatile struct gicd_reg *gicd = (volatile struct gicd_reg *)GICD_BASE;
  if (gicd->ispendr[irq / 32] & (1 << (irq % 32)))
    return 1;
  return 0;
}

void __attribute__((weak)) hook_display_vblank(void)
{
}

// analog audio codec interrupt handler
extern void codec_fiq_handler(void);

// Called when an interrupt is triggered
void __attribute__((interrupt("IRQ"))) interrupt(void)
{
  // PA EINT
  if (irq_pending(43)) {
    char buf[50];
	volatile struct port_irq_registers *port = (volatile struct port_irq_registers *)(PA_EINT_BASE);
    sprintf(buf, "Interrupt source %lu triggered!\r\n", port->status);
    uart_print(buf);
    gpio_irq_ack(PA_EINT_BASE);
  }

  // digital audio (I2S/PCM-0)
  if (irq_pending(45))
    audio_queue_samples();

  // digital audio (I2S/PCM-1)
  if (irq_pending(46))
    audio_queue_samples();

  // digital audio (I2S/PCM-2)
  if (irq_pending(47))
    audio_queue_samples();

  // PG EINT
  if (irq_pending(49)) {
    char buf[50];
	volatile struct port_irq_registers *port = (volatile struct port_irq_registers *)(PG_EINT_BASE);
    sprintf(buf, "Interrupt pin %lu triggered!\r\n", port->status);
    uart_print(buf);
    gpio_irq_ack(PG_EINT_BASE);
  }

  // R PL EINT
  if (irq_pending(77)) {
    char buf[50];
	volatile struct port_irq_registers *port = (volatile struct port_irq_registers *)(R_PL_EINT_BASE);
    sprintf(buf, "Interrupt pin %lu triggered!\r\n", port->status);
    uart_print(buf);
    gpio_irq_ack(R_PL_EINT_BASE);
  }

  // analog audio
  if (irq_pending(82))
    codec_fiq_handler();

  // USB controllers
  if (irq_pending(107))
    usb1_hal_hcd_isr(0);

  if (irq_pending(109))
    usb2_hal_hcd_isr(0);

  if (irq_pending(111))
    usb3_hal_hcd_isr(0);

  // LCD controllers 0 and 1
  // NB: Only one of these will be enabled at any time.
  if (irq_pending(118)) {
    tick_counter++;
    LCD0_GINT0 &= ~(1 << 14);
    hook_display_vblank();
  }
  if (irq_pending(119)) {
    tick_counter++;
    LCD1_GINT0 &= ~(1 << 14);
    hook_display_vblank();
  }
}

// XXX: Something is really wrong here.
// Enabling _any_ interrupt enables all of them, and disabling them does not
// seem to have any effect. This is not much of a problem in practice
// because the device drivers silence the interrupts at the source, but
// something needs fixing here.

void irq_enable(uint32_t irq)
{
  volatile struct gicd_reg *gicd = (volatile struct gicd_reg *)GICD_BASE;
#ifdef GDBSTUB
  gicd->ctlr = BIT(1) | BIT(0);
#else
  gicd->ctlr = BIT(1);
#endif
  gicd->igroupr[irq / 32] |= 1UL << (irq % 32);  // set to group 1
  gicd->isenabler[irq / 32] = 1UL << (irq % 32);
  gicd->itargetsr[irq]      = 1;  // target core 0
  gicd->ipriorityr[irq]     = 2;  // lower than FIQ
}

void irq_enable_fiq(uint32_t irq)
{
  volatile struct gicd_reg *gicd = (volatile struct gicd_reg *)GICD_BASE;

  irq_enable(irq);

  gicd->igroupr[irq / 32] &= ~(1UL << (irq % 32));  // set to group 0
  gicd->ipriorityr[irq] = 1;                        // higher than IRQ
}

void irq_disable(uint32_t irq)
{
  volatile struct gicd_reg *gicd = (volatile struct gicd_reg *)GICD_BASE;
  gicd->icenabler[irq / 32]      = 1 << (irq % 32);
}

// Copy the interrupt table from _ivt to 0x0
void __attribute__((no_sanitize("all"))) install_ivt()
{
  uint32_t *source      = &_ivt;
  uint32_t *destination = (uint32_t *)(0);
  for (int n = 0; n < 2 * 8; n++)
    destination[n] = source[n];

  volatile struct gicc_reg *gicc = (volatile struct gicc_reg *)GICC_BASE;
#ifdef GDBSTUB
  // enable interrupt groups 0 and 1, signal group 0 as FIQ
  gicc->ctlr = BIT(3) | BIT(1) | BIT(0);
#else
  // enable interrupt group 1 only
  gicc->ctlr = BIT(1);
#endif
  gicc->pmr = 10;

  volatile struct gicd_reg *gicd = (volatile struct gicd_reg *)GICD_BASE;
  // set all interrupts to group 1
  for (int i = 0; i < 32; ++i)
    gicd->igroupr[i] = 0xffffffff;

  asm("cpsie if;");  // Enable interrupts
}
