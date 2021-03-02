#include "uart.h"

void halt(void)
{
	uart_print("System halted.\r\n");
	for (;;);
}

static inline void stack_dump(uint32_t *stack)
{
	uart_print("Dump from "); uart_print_uint32((uint32_t)stack);
	for(;;);
	for (int i = 0; i < 32; ++i) {
		uart_print("\r\nSP-");uart_print_uint8(i);
		uart_print("  ");
		uart_print_uint32(stack[-i]);
	}
	uart_print("\r\n");
}

void __attribute__((interrupt("UNDEF"))) except_undef(void)
{
	register uint32_t *reg_sp asm("sp");
	uint32_t *saved_sp = reg_sp;
	uart_print("undefined instruction");
	stack_dump(saved_sp);
	halt();
}

void __attribute__((interrupt("SWI"))) except_swi(void)
{
	register uint32_t *reg_sp asm("sp");
	uint32_t *saved_sp = reg_sp;
	uart_print("SWI");
	stack_dump(saved_sp);
	halt();
}

void __attribute__((interrupt("ABORT"))) except_prefetch_abort(void)
{
	uart_print("prefetch abort\r\n");
	halt();
}

void __attribute__((interrupt("ABORT"))) except_data_abort(void)
{
	register uint32_t reg_lr asm("lr");
	uint32_t saved_lr = reg_lr;
	uart_print("data abort at ");
	uart_print_uint32(saved_lr);
	halt();
}

void __attribute__((interrupt("FIQ"))) except_fiq(void)
{
	uart_print("FIQ\r\n");
	halt();
}
