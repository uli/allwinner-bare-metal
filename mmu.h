#ifdef __cplusplus
extern "C" {
#endif

void mmu_init(void);
void mmu_flush_dcache(void);
void *mmu_detect_dram_end(void);

#ifdef __cplusplus
}
#endif
