#include "../interrupts.h"
#include "../fixed_addr.h"

void gdbstub_init_jh(void *stack)
{
    // Initialize SP_svc
    // XXX: maybe there's a less hacky way to do this...
    asm("cps #0x13 ; mov sp, r0 ; cps #0x1f");

    volatile char *available_in = (volatile char*)GDBSTUB_PORT_ADDR;
    volatile char *available_out = (volatile char*)GDBSTUB_PORT_ADDR + 2;
    *available_in = 0;
    *available_out = 0;
}

char gdbstub_getc(void)
{
    volatile char *available = (volatile char*)GDBSTUB_PORT_ADDR;
    volatile char *data = (volatile char *)GDBSTUB_PORT_ADDR + 1;
    while (!*available) {
        asm("wfe");
    }
    irq_unpend(125);
    if (*available) {
        char d = *data;
        *available = 0;
        return d;
    }
    return 0;
}

void gdbstub_putc(char byte)
{
    volatile char *available = (volatile char*)GDBSTUB_PORT_ADDR + 2;
    volatile char *data = (volatile char *)GDBSTUB_PORT_ADDR + 3;
    while (*available) {
        asm("wfe");
    }
    *data = byte;
    *available = 1;
}
