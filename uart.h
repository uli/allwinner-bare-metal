#include <stdint.h>

// The UART registers base address.
#define UART0_BASE 0x01C28000
// Macros to access UART registers.
#define UART0_RBR *(volatile uint32_t *)(UART0_BASE + 0x00)
#define UART0_THR *(volatile uint32_t *)(UART0_BASE + 0x00)
#define UART0_DLL *(volatile uint32_t *)(UART0_BASE + 0x00)
#define UART0_IER *(volatile uint32_t *)(UART0_BASE + 0x04)
#define UART0_FCR *(volatile uint32_t *)(UART0_BASE + 0x08)
#define UART0_LCR *(volatile uint32_t *)(UART0_BASE + 0x0C)
#define UART0_LSR *(volatile uint32_t *)(UART0_BASE + 0x14)
#define UART0_USR *(volatile uint32_t *)(UART0_BASE + 0x7C)

void uart_init();
void uart_print(const char* str);
void uart_print_uint8(unsigned char number);
void uart_print_uint32(uint32_t number);
void uart_putc(unsigned char byte);
