#include "uart.h"

static void halt(void)
{
	uart_print("System halted.\n");
	for (;;);
}

void __attribute__((interrupt("UNDEF"))) except_undef(void)
{
	register uint32_t reg_lr asm("lr");
	uint32_t saved_lr = reg_lr;
	uart_print("undefined instruction at ");
	uart_print_uint32(saved_lr);
	halt();
}

void __attribute__((interrupt("SWI"))) except_swi(void)
{
	uart_print("SWI\n");
	halt();
}

void __attribute__((interrupt("ABORT"))) except_prefetch_abort(void)
{
	uart_print("prefetch abort\n");
	halt();
}

void __attribute__((interrupt("ABORT"))) except_data_abort(void)
{
	uart_print("data abort\n");
	halt();
}

void __attribute__((interrupt("FIQ"))) except_fiq(void)
{
	uart_print("FIQ\n");
	halt();
}