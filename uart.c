#include <stddef.h>
#include <stdint.h>
#include "uart.h"
#include "ports.h"
#include "ccu.h"

// Set up the UART (serial port)
void uart_init()
{
  // Configure port
  set_pin_mode(PORTA, 4, 2);

  // Enable clock
  BUS_CLK_GATING3 |= (1<<16);
  BUS_SOFT_RST4 |= (1<<16);

  // Configure baud rate
  UART0_LCR = (1<<7) | 3;
  UART0_DLL = 13;
  UART0_LCR = 3;

  // Enable FIFO
  UART0_FCR = 0x00000001;
}

// These functions are used when the stack protector has been triggered
// already.
#pragma GCC push_options
#pragma GCC optimize "-fno-stack-protector"

// UART is ready to receive data to transmit?
unsigned char uart_tx_ready()
{
  return (UART0_USR & 2);
}

// UART has received data?
unsigned char uart_rx_ready()
{
  return (UART0_LSR & 1);
}

// Push one byte to the UART port (blocking until ready to transmit)
void uart_putc(unsigned char byte)
{
  // Wait for UART transmit FIFO to be not full.
  while ( ! uart_tx_ready() );
  UART0_THR = byte;
}

// Write a zero terminated string to the UART
void uart_print(const char* str)
{
  while(*str) {
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
