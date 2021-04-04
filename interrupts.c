#include <stdint.h>
#include "uart.h"
#include "interrupts.h"
#include "display.h"
#include "audio.h"
#include "system.h"
#include "util.h"

extern uint32_t _ivt;

void usb1_hal_hcd_isr(uint8_t hostid);
void usb2_hal_hcd_isr(uint8_t hostid);
void usb3_hal_hcd_isr(uint8_t hostid);

int irq_pending(uint32_t irq)
{
  volatile struct gicd_reg* gicd = (volatile struct gicd_reg*) GICD_BASE;
  if (gicd->ispendr[irq / 32] & (1 << (irq % 32)))
    return 1;
  return 0;
}

void __attribute__((weak)) hook_display_vblank(void)
{
}

// Called when an interrupt is triggered
// Currently this is always triggered by at new frame at 60Hz
void __attribute__((interrupt("IRQ"))) interrupt(void) {
  if (irq_pending(47))
    audio_queue_samples();

  if (irq_pending(107))
    usb1_hal_hcd_isr(0);

  if (irq_pending(109))
    usb2_hal_hcd_isr(0);

  if (irq_pending(111))
    usb3_hal_hcd_isr(0);

  if (irq_pending(118)) {
    tick_counter++;
    LCD0_GINT0 &= ~(1<<12);
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
  volatile struct gicd_reg* gicd = (volatile struct gicd_reg*) GICD_BASE;
#ifdef GDBSTUB
  gicd->ctlr = BIT(1) | BIT(0);
#else
  gicd->ctlr = BIT(1);
#endif
  gicd->igroupr[irq/32]  |= 1UL << (irq % 32);	// set to group 1
  gicd->isenabler[irq/32] = 1UL << (irq % 32);
  gicd->itargetsr[irq] = 1;	// target core 0
  gicd->ipriorityr[irq] = 2;	// lower than FIQ

}

void irq_enable_fiq(uint32_t irq)
{
  volatile struct gicd_reg* gicd = (volatile struct gicd_reg*) GICD_BASE;

  irq_enable(irq);

  gicd->igroupr[irq/32] &= ~(1UL << (irq % 32));	// set to group 0
  gicd->ipriorityr[irq] = 1;	// higher than IRQ
}

void irq_disable(uint32_t irq)
{
  volatile struct gicd_reg* gicd = (volatile struct gicd_reg*) GICD_BASE;
  gicd->icenabler[irq/32] = 1 << (irq % 32);
}

// Copy the interrupt table from _ivt to 0x0
void __attribute__((no_sanitize("all"))) install_ivt() {
  uint32_t* source = &_ivt;
  uint32_t* destination = (uint32_t*)(0);
  for(int n=0; n<2*8; n++)
    destination[n] = source[n];

  irq_enable(118);	// LCD0

  volatile struct gicc_reg* gicc = (volatile struct gicc_reg*) GICC_BASE;
#ifdef GDBSTUB
  // enable interrupt groups 0 and 1, signal group 0 as FIQ
  gicc->ctlr = BIT(3) | BIT(1) | BIT(0);
#else
  // enable interrupt group 1 only
  gicc->ctlr = BIT(1);
#endif
  gicc->pmr = 10;

  volatile struct gicd_reg* gicd = (volatile struct gicd_reg*) GICD_BASE;
  // set all interrupts to group 1
  for (int i = 0; i < 32; ++i)
    gicd->igroupr[i] = 0xffffffff;

  asm("cpsie if;"); // Enable interrupts
}
