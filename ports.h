#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Port access struct
struct port_registers {
  uint32_t cfg0;
  uint32_t cfg1;
  uint32_t cfg2;
  uint32_t cfg3;
  uint32_t data;
  uint32_t drv0;
  uint32_t drv1;
  uint32_t pul0;
  uint32_t pul1;
};

// The PORT registers base address.
#define PIO_BASE          0x01C20800
#define PORTA             PIO_BASE + 0 * 0x24
#define PORTC             PIO_BASE + 2 * 0x24
#define PORTD             PIO_BASE + 3 * 0x24
#define PORTE             PIO_BASE + 4 * 0x24
#define PORTF             PIO_BASE + 5 * 0x24
#define PORTG             PIO_BASE + 6 * 0x24
#define PORTL             0x01F02C00
#define PIO_PORT(n)	  ((n) == 7 ? PORTL : PIO_BASE + (n) * 0x24)

#define PIO_PULL_OFF	0
#define PIO_PULL_UP	1
#define PIO_PULL_DOWN	2

void set_pin_mode(uint32_t port_addr, uint32_t pin, uint32_t mode);
void set_pin_data(uint32_t port_addr, uint32_t pin, uint32_t data);
int get_pin_data(uint32_t port_addr, uint32_t pin);
void set_pin_drive(uint32_t port_addr, uint32_t pin, uint32_t strength);
void set_pin_pull(uint32_t port_addr, uint32_t pin, uint32_t pull);

void gpio_init();

#ifdef __cplusplus
}
#endif
