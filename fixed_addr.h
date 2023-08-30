// Fixed addresses that we want to keep consistent

#ifdef JAILHOUSE

// start of application memory
#define AWBM_BASE_ADDR		0x49000000

// Jailhouse root cell communication shared memory areas
#define JAILHOUSE_COMM_BASE	0x488f0000
#define SDL_EVENT_BUFFER_ADDR	(JAILHOUSE_COMM_BASE + 0x1000)
#define LIBC_CALL_BUFFER_ADDR	(JAILHOUSE_COMM_BASE + 0xc000)
#define GDBSTUB_PORT_ADDR	(JAILHOUSE_COMM_BASE + 0xa000)
#define VIDEO_ENCODER_PORT_ADDR	(JAILHOUSE_COMM_BASE + 0xb000)

// application exception vectors
#define AWBM_IRQ_HANDLER_VECTOR	0xfffc
#define GDBSTUB_IRQ_HANDLER_VECTOR 0xfff8
#define GDBSTUB_SVC_HANDLER_VECTOR 0xfff4
#define AWBM_DABT_HANDLER_VECTOR 0xfff0

#else

// start of application memory
#define AWBM_BASE_ADDR		0x40000000

#endif

// uncached memory area for DMA buffers
#define AWBM_UNCACHED_ADDR	(AWBM_BASE_ADDR + 0x01000000)
