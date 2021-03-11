#include "dma.h"
#include "ccu.h"
#include "system.h"
#include "mmu.h"
#include <stdint.h>

#define DMA_BASE 0x1c02000
#define DMA_REG(n) (*(volatile uint32_t *)(DMA_BASE + (n)))
#define DMA_STA_REG 		DMA_REG(0x30)

#define DMA_EN_REG(n) 		DMA_REG(0x100 + (n) * 0x40)
#define DMA_DESC_ADDR_REG(n)	DMA_REG(0x108 + (n) * 0x40)

void dma_init(void)
{
    // reset
    BUS_SOFT_RST0 &= ~0x40;
    BUS_CLK_GATING0 |= 0x40;	// enable clock

    for (int i = 0 ; i < 12; ++i)
        DMA_EN_REG(i) = 0;

    udelay(10);
    BUS_SOFT_RST0 |= 0x40;
}

struct dma_desc {
    uint32_t config;
    uint32_t src_addr;
    uint32_t dest_addr;
    uint32_t count;
    uint32_t param;
    uint32_t next;
};

volatile struct dma_desc memcpy_desc __attribute__ ((section ("UNCACHED")));

void dma_memcpy(void *dest, void *src, int size, int channel)
{
    memcpy_desc.config = 0x04c104c1; //0x04810481;
    memcpy_desc.src_addr = (uint32_t)src;
    memcpy_desc.dest_addr = (uint32_t)dest;
    memcpy_desc.count = size;
    memcpy_desc.param = 8;	// NORMAL_WAIT
    memcpy_desc.next = 0xfffff800;	// end of list

    DMA_EN_REG(channel) = 0;
    DMA_DESC_ADDR_REG(channel) = (uint32_t)&memcpy_desc;
    mmu_flush_dcache();
    DMA_EN_REG(channel) = 1;
}

void dma_wait(int channel)
{
    while (DMA_STA_REG & (1 << channel)) {
    }
}
