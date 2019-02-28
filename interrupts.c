#include <stdint.h>
#include "uart.h"
#include "interrupts.h"
#include "display.h"

extern uint32_t _ivt;
void game_tick_next();
void hal_hcd_isr(uint8_t hostid);

// Called when an interrupt is triggered
// Currently this is always triggered by at new frame at 60Hz
void __attribute__((interrupt("FIQ"))) interrupt(void) {
  hal_hcd_isr(0);
  game_tick_next();
  LCD0_GINT0 &= ~(1<<12);
}

// Copy the interrupt table from _ivt to 0x0
void install_ivt() {
  uint32_t* source = &_ivt;
  uint32_t* destination = (uint32_t*)(0);
  for(int n=0; n<2*8; n++)
    destination[n] = source[n];

  struct gicd_reg* gicd = (struct gicd_reg*) GICD_BASE;
  gicd->ctlr = 1;
  gicd->isenabler[118/32] = 1<<(118%32);
  gicd->itargetsr[118] = 1;
  gicd->ipriorityr[118] = 1;
  struct gicc_reg* gicc = (struct gicc_reg*) GICC_BASE;
  gicc->ctlr = 1;
  gicc->pmr = 10;
  asm("cpsie if;"); // Enable interrupts
}
