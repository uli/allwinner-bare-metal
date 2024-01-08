#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// The UART registers base address.
#ifdef AWBM_PLATFORM_h3
  #define UART_BASE_ADDR 0x01C28000
  #define UART_BASE(n) ((n) < 4 ? UART_BASE_ADDR + (n) * 0x400 : 0x01F02800)
#elif defined(AWBM_PLATFORM_h616)
  #define UART_BASE_ADDR 0x05000000
  #define UART_BASE(n) (UART_BASE_ADDR + (n) * 0x400)
#else
  #error unknown platform
#endif

// Macros to access UART registers.
#define UART_RBR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x00)
#define UART_THR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x00)
#define UART_DLL(n) *(volatile uint32_t *)(UART_BASE(n) + 0x00)
#define UART_IER(n) *(volatile uint32_t *)(UART_BASE(n) + 0x04)
#define UART_FCR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x08)
#define UART_LCR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x0C)
#define UART_LSR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x14)
#define UART_USR(n) *(volatile uint32_t *)(UART_BASE(n) + 0x7C)

void uart_init(int n);
void uart_print(const char* str);
void uart_print_uint8(unsigned char number);
void uart_print_uint32(uint32_t number);
void uart_putc(char byte);
char uart_getc(void);

#ifdef GDBSTUB

#ifdef JAILHOUSE

char gdbstub_getc(void);
void gdbstub_putc(char byte);

#else

#define gdbstub_getc() uart_getc()
#define gdbstub_putc(c) uart_putc(c)

#endif

#endif

void uart_write_byte(int n, char byte);
void uart_write_bytes(int n, const char *str);
char uart_read_byte(int n);

unsigned char uart_rx_ready(int n);
unsigned char uart_tx_ready(int n);

#ifdef __cplusplus
}
#endif
