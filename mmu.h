#ifdef __cplusplus
extern "C" {
#endif

void mmu_init(void);
void mmu_flush_dcache(void);
void *mmu_detect_dram_end(void);

void mmu_flush_dcache_range(void *addr, unsigned long size, int flush);

#define MMU_DCACHE_CLEAN 0
#define MMU_DCACHE_INVALIDATE 1
#define MMU_DCACHE_CLEAN_INVALIDATE 2

#ifdef __cplusplus
}
#endif
