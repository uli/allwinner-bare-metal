#ifdef __cplusplus
extern "C" {
#endif

#define GICD_BASE 0x01C81000

struct gicd_reg {
  uint32_t ctlr;           /* 0x000 */
  uint32_t typer;          /* 0x004 */
  uint32_t iidr;           /* 0x008 */
  uint32_t rfu0[29];       /* 0x00C - 0x07C */
  uint32_t igroupr[32];    /* 0x080 - 0x0FC */
  uint32_t isenabler[32];
  uint32_t icenabler[32];
  uint32_t ispendr[32];
  uint32_t icpendr[32];
  uint32_t isactiver[32];
  uint32_t icactiver[32];
  uint8_t  ipriorityr[1024];
  uint8_t  itargetsr[1024];
  uint32_t icfgr[64];
  uint32_t rfu1[64];
  uint32_t nsacr[64];
  uint32_t sgir;
  uint32_t rfu2[3];
  uint32_t cpendsgir[4];
  uint32_t spendsgir[4];
  uint32_t rfu3[52];
};

#define GICC_BASE 0x01C82000
struct gicc_reg {
  uint32_t ctlr;         /* 0x000 */
  uint32_t pmr;          /* 0x004 */
  uint32_t bpr;          /* 0x008 */
  uint32_t iar;          /* 0x00C */
  uint32_t eoir;
  uint32_t rpr;
  uint32_t hppir;
  uint32_t abpr;
  uint32_t aiar;
  uint32_t aeoir;
  uint32_t ahppir;
  uint32_t rfu0[41];
  uint32_t apr[4];
  uint32_t nsapr[4];
  uint32_t rfu1[3];
  uint32_t iidr;
  uint32_t rfu2[960];
  uint32_t dir;
};

void install_ivt();

void irq_enable(uint32_t irq);
void irq_enable_fiq(uint32_t irq);
void irq_disable(uint32_t irq);
int irq_pending(uint32_t irq);

#ifdef __cplusplus
}
#endif
