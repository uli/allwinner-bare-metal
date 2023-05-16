// Fixed addresses that we want to keep consistent

#ifdef JAILHOUSE

#define AWBM_BASE_ADDR		0x49000000
#define SDL_EVENT_BUFFER_ADDR	0x488f1000
#define LIBC_CALL_BUFFER_ADDR	0x488fc000
#define GDBSTUB_PORT_ADDR	0x488fa000

#define AWBM_IRQ_HANDLER_VECTOR	0xfffc
#else

#define AWBM_BASE_ADDR		0x40000000

#endif
