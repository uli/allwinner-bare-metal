#include <stdint.h>
#include "uart.h"
#include "interrupts.h"
#include "display.h"
#include "audio.h"

extern uint32_t _ivt;
void game_tick_next();
void hal_hcd_isr(uint8_t hostid);

int irq_pending(uint32_t irq)
{
  struct gicd_reg* gicd = (struct gicd_reg*) GICD_BASE;
  if (gicd->ispendr[irq / 32] & (1 << (irq % 32)))
    return 1;
  return 0;
}

// Called when an interrupt is triggered
// Currently this is always triggered by at new frame at 60Hz
void __attribute__((interrupt("FIQ"))) interrupt(void) {
  if (irq_pending(47)) {
    audio_queue_samples();
  }
  if (irq_pending(107))
    hal_hcd_isr(0);
  if (irq_pending(118)) {
    game_tick_next();
    LCD0_GINT0 &= ~(1<<12);
  }
}

void irq_enable(uint32_t irq)
{
  struct gicd_reg* gicd = (struct gicd_reg*) GICD_BASE;
  gicd->ctlr = 1;
  gicd->isenabler[irq/32] = 1<<(irq%32);
  gicd->itargetsr[irq] = 1;
  gicd->ipriorityr[irq] = 1;

}

void irq_disable(uint32_t irq)
{
  struct gicd_reg* gicd = (struct gicd_reg*) GICD_BASE;
  gicd->icenabler[irq/32] = 1 << (irq % 32);
}

// Copy the interrupt table from _ivt to 0x0
void install_ivt() {
  uint32_t* source = &_ivt;
  uint32_t* destination = (uint32_t*)(0);
  for(int n=0; n<2*8; n++)
    destination[n] = source[n];

  irq_enable(118);	// LCD0

  struct gicc_reg* gicc = (struct gicc_reg*) GICC_BASE;
  gicc->ctlr = 1;
  gicc->pmr = 10;
  asm("cpsie if;"); // Enable interrupts
}
