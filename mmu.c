#include <stdint.h>
#include "mmu.h"
#include "uart.h"
#include <arm/synchronize.h>

#define DRAM_START 0x40000000
#define DRAM_MAX   0xc0000000
#define DRAM_STEP  0x02000000

void *mmu_detect_dram_end(void)
{
  volatile uint32_t *dram_start = (uint32_t *)DRAM_START;
  volatile uint32_t *dram_end = dram_start + DRAM_STEP / sizeof(uint32_t);

  uint32_t saved_dram_start = *dram_start;
  uint32_t saved_dram_end;

  *dram_start = 0xdeadbeef;

  while (dram_end < (volatile uint32_t *)DRAM_MAX) {
    // Check for wraparound by writing a value to DRAM, then checking
    // if it overwrote the value at the beginning.
    saved_dram_end = *dram_end;

    *dram_end = 0xcafebabe;
    asm volatile("dsb");
    if (*dram_start == 0xcafebabe)
      break;

    *dram_end = saved_dram_end;
    dram_end += DRAM_STEP / sizeof(uint32_t);
  }
  *dram_start = saved_dram_start;

  uart_print_uint32((uint32_t)dram_end); uart_print(" RAM limit\r\n");
  return (void *)dram_end;
}

void mmu_init() {

  // Disable MMU
  asm("ldr r8, =0x0;    mcr p15, 0, r8, c1, c0, 0;" : : : "r8");

  // Enable cache coherency
  asm("mrc p15, 0, r8, c1, c0, 1;"
      "orr r8, r8, #0x40;"
      "mcr p15, 0, r8, c1, c0, 1;" ::: "r8");

  // Populate the pagetable
  volatile uint32_t* pagetable = (volatile uint32_t *)0xc000;
  for (uint32_t n = 0; n < 0x1000; n++) {
    if(n==0) {
      // SRAM.  Write back.
      pagetable[n] = (n<<20) | (1<<12) | (3<<10) | (3<<2) | 2;
    } else if(n>=0x400 && n<0x410) {
      // First half of DRAM. Write back.
      pagetable[n] = (n<<20) | (1<<12) | (3<<10) | (3<<2) | 2;
    } else if(n>=0x410 && n<0x420) {
      // DMA buffers and such. Normal uncached.
      pagetable[n] = (n<<20) | (1<<12) | (3<<10) | (0<<2) | 2;
    } else if(n>=0x420 && n<0xc00) {
      // Second half of DRAM. Write back.
      pagetable[n] = (n<<20) | (1<<12) | (3<<10) | (3<<2) | 2;
    } else {
      // Other stuff. Strictly ordered for safety.
      pagetable[n] = (n<<20) | (0<<12) | (3<<10) | (0<<2) | 2;
    }
  }

  // Set up the pagetable
  asm("ldr r8, =0xc000; mcr p15, 0, r8, c2, c0, 0" : : : "r8");
  asm("ldr r8, =0x0;    mcr p15, 0, r8, c2, c0, 2" : : : "r8");
  asm("ldr r8, =0x3;    mcr p15, 0, r8, c3, c0, 0" : : : "r8");

  // Word on the street (i.e. in lib-h3) has it that it is necessary to
  // invalidate the L1 cache here because it comes out of reset
  // uninitialized and bogus data might be flushed to RAM. Any attempt to do
  // so at any place (by calling invalidate_data_cache_l1_only()) resulted
  // in crashes of secondary cores that would not be there otherwise, so I'm
  // ignoring this.

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
