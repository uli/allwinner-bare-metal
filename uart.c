#include <stddef.h>
#include <stdint.h>
#include "ccu.h"
#include "interrupts.h"
#include "ports.h"
#include "uart.h"
#include "util.h"

// Set up a UART (serial port)
void uart_init(int n)
{
  if (n == 0) {
    // Configure port
    set_pin_mode(PORTA, 4, 2);
  }

  // Enable clock
  if (n < 4) {
    BUS_CLK_GATING3 |= BIT(16 + n);
    BUS_SOFT_RST4 |= BIT(16 + n);
  } else {
    // XXX: S_UART?
  }

  // Configure baud rate
  UART_LCR(n) = (1 << 7) | 3;
  UART_DLL(n) = 13;
  UART_LCR(n) = 3;

  // Enable FIFO
  UART_FCR(n) = 0x00000001;

#ifdef GDBSTUB
  if (n == 0) {
    // signal UART0 interrupt as FIQ for the GDB stub
    irq_enable_fiq(32);
    UART_IER(n) |= BIT(0);
  }
#endif
}

// These functions are used when the stack protector has been triggered
// already.
#pragma GCC push_options
#pragma GCC optimize "-fno-stack-protector"

// UART is ready to receive data to transmit?
unsigned char uart_tx_ready(int n)
{
  return (UART_USR(n) & 2);
}

// UART has received data?
unsigned char uart_rx_ready(int n)
{
  return (UART_LSR(n) & 1);
}

// Push one byte to the UART port (blocking until ready to transmit)
void uart_write_byte(int n, char byte)
{
  // Wait for UART transmit FIFO to be not full.
  while (!uart_tx_ready(n))
    ;
  UART_THR(n) = byte;
}

void uart_putc(char byte)
{
  uart_write_byte(0, byte);
}

char uart_read_byte(int n)
{
  while (!uart_rx_ready(n))
    ;
  return UART_RBR(n);
}

char uart_getc(void)
{
  return uart_read_byte(0);
}

// Write a zero terminated string to the UART
void uart_print(const char *str)
{
  while (*str) {
    uart_putc(*str);
    str++;
  }
}

// Print a char to the UART as ASCII HEX
void uart_print_uint8(unsigned char number)
{
  unsigned char chars[] = "0123456789ABCDEF";
  uart_putc(chars[(number >> 4) & 0xF]);
  uart_putc(chars[(number >> 0) & 0xF]);
}

// Print a uint32 to the UART as ASCII HEX
void uart_print_uint32(uint32_t number)
{
  unsigned char chars[] = "0123456789ABCDEF";
  uart_putc(chars[(number >> 28) & 0xF]);
  uart_putc(chars[(number >> 24) & 0xF]);
  uart_putc(chars[(number >> 20) & 0xF]);
  uart_putc(chars[(number >> 16) & 0xF]);
  uart_putc(chars[(number >> 12) & 0xF]);
  uart_putc(chars[(number >> 8) & 0xF]);
  uart_putc(chars[(number >> 4) & 0xF]);
  uart_putc(chars[(number >> 0) & 0xF]);
}

#pragma GCC pop_options
