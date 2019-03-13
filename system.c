#include "system.h"

void init_bss() {
  for (char* dst = &_bstart1; dst < &_bend1; dst++)
    *dst = 0;
  for (char* dst = &_bstart2; dst < &_bend2; dst++)
    *dst = 0;
}

void udelay(uint32_t d) {
  for(uint32_t n=0;n<d*200;n++) asm("NOP");
}

