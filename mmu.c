#include <stdint.h>
#include "mmu.h"

void mmu_init() {

  // Disable MMU
  asm("ldr r8, =0x0;    mcr p15, 0, r8, c1, c0, 0;" : : : "r8");

  // Populate the pagetable
  volatile uint32_t* pagetable = (volatile uint32_t *)0xc000;
  for(int n=0;n<0x1000;n++) {
    if(n==0) {
      // SRAM.  Write back.
      pagetable[n] = (n<<20) | (1<<12) | (3<<10) | (3<<2) | 2;
    } else if(n>=0x400 && n<0x500) {
      // First half of DRAM. Write back.
      pagetable[n] = (n<<20) | (1<<12) | (3<<10) | (3<<2) | 2;
    } else if(n>=0x500 && n<0x600) {
      // DMA buffers and such. Normal uncached.
      pagetable[n] = (n<<20) | (1<<12) | (3<<10) | (0<<2) | 2;
    } else {
      // Other stuff. Strictly ordered for safety.
      pagetable[n] = (n<<20) | (0<<12) | (3<<10) | (0<<2) | 2;
    }
  }

  // Set up the pagetable
  asm("ldr r8, =0xc000; mcr p15, 0, r8, c2, c0, 0" : : : "r8");
  asm("ldr r8, =0x0;    mcr p15, 0, r8, c2, c0, 2" : : : "r8");
  asm("ldr r8, =0x3;    mcr p15, 0, r8, c3, c0, 0" : : : "r8");

  // Enable MMU
  asm(
    "ldr r8, =0x0;"
    "MCR p15, 0, r8, c8, C3, 0;"
    "MCR p15, 0, r8, c8, C5, 0;"
    "MCR p15, 0, r8, c8, C6, 0;"
    "MCR p15, 0, r8, c8, C7, 0;"
    "mcr p15, 0, r8, c12, c0, 0;"

    "ldr r8, =0x1005;"
    "mcr p15, 0, r8, c1, c0, 0;"
    : : : "r8");

}
