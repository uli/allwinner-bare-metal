#ifdef __cplusplus
extern "C" {
#endif

void dma_memcpy(void *dest, void *src, int size, int channel);
void dma_wait(int channel);
void dma_init(void);

#ifdef __cplusplus
}
#endif
