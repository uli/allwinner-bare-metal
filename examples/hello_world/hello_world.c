#include <../../uart.h>

int notmain (void)
{
    uart_print("Hello world from new FastECU!!!\n");
    while(1)
    {
        uart_putc(uart_getc());  // wait till it gets new char  -  echo 
    }
    return(0);
}

