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

struct port_irq_registers {
  uint32_t cfg0;
  uint32_t cfg1;
  uint32_t cfg2;
  uint32_t cfg3;
  uint32_t ctl;
  uint32_t status;
  uint32_t deb;
};

// GPIO banks
#define GPIO_BANK_A				0
//#define GPIO_BANK_B 			1 // not a valid bank on Allwinner H3
#define GPIO_BANK_C				2
#define GPIO_BANK_D				3
#define GPIO_BANK_E				4
#define GPIO_BANK_F				5
#define GPIO_BANK_G				6

// The PORT registers base address.
#define PIO_BASE				0x01C20800
#define PORTA					PIO_BASE + 0 * 0x24
#define PORTC					PIO_BASE + 2 * 0x24
#define PORTD					PIO_BASE + 3 * 0x24
#define PORTE					PIO_BASE + 4 * 0x24
#define PORTF					PIO_BASE + 5 * 0x24
#define PORTG					PIO_BASE + 6 * 0x24
#define PORTL					0x01F02C00
#define PIO_PORT(n)				((n) == 7 ? PORTL : PIO_BASE + (n) * 0x24)

// PORT configuration registers
#define Pn_CFG0_REG(n)			(PIO_BASE + (n*0x24) + (0x00)) // n = PORT BANK (0-6)
#define Pn_CFG1_REG(n)			(PIO_BASE + (n*0x24) + (0x04))
#define Pn_CFG2_REG(n)			(PIO_BASE + (n*0x24) + (0x08))
#define Pn_CFG3_REG(n)			(PIO_BASE + (n*0x24) + (0x0c))

#define PIO_DATA(n)				(PIO_BASE + (n*0x24) + 0x10) // n = PIN number
#define PIO_PULL0(n)			(PIO_BASE + (n*0x24) + 0x1C) // n = PIN number
#define PIO_PULL1(n)			(PIO_BASE + (n*0x24) + 0x20) // n = PIN number

#define PA_EINT_BASE			(PIO_BASE + 0x200 + (0*0x20))
#define PA_EINT_CFG_REG(n)		(PIO_BASE + 0x200 + (0*0x20) + (4*n)) // n = 0-3
#define PA_EINT_CTL_REG			(PIO_BASE + 0x210 + (0*0x20))
#define PA_EINT_STATUS_REG		(PIO_BASE + 0x214 + (0*0x20))
#define PA_EINT_DEB_REG			(PIO_BASE + 0x218 + (0*0x20))

#define PG_EINT_BASE			(PIO_BASE + 0x200 + (1*0x20))
#define PG_EINT_CFG_REG(n)		(PIO_BASE + 0x200 + (1*0x20) + (4*n)) // n = 0-3
#define PG_EINT_CTL_REG			(PIO_BASE + 0x210 + (1*0x20))
#define PG_EINT_STATUS_REG		(PIO_BASE + 0x214 + (1*0x20))
#define PG_EINT_DEB_REG			(PIO_BASE + 0x218 + (1*0x20))

#define R_PL_EINT_BASE			(PORTL + 0x200 + (0*0x20))

#define readb(addr)				(*((volatile unsigned char  *)(addr)))
#define readw(addr)				(*((volatile unsigned short *)(addr)))
#define readl(addr)				(*((volatile unsigned long  *)(addr)))
#define writeb(v, addr)			(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)			(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define writel(v, addr)			(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

// PORT pin configuration values
#define PIO_PULL_OFF			0
#define PIO_PULL_UP				1
#define PIO_PULL_DOWN			2

#define GPIO_PULL_OFF			0
#define GPIO_PULL_UP			1
#define GPIO_PULL_DOWN			2

#define GPIO_LOW_LEVEL			0
#define GPIO_HIGH_LEVEL			1

#define GPIO_MODE_INPUT			0
#define GPIO_MODE_OUTPUT		1
#define GPIO_MODE_EINT			6

#define PA_EINT					43
#define PG_EINT					49
#define R_PL_EINT				77

#define GPIO_IRQ_DISABLE		0
#define GPIO_IRQ_ENABLE			1

#define GPIO_EINT_RISING		0
#define GPIO_EINT_FALLING		1
#define GPIO_EINT_HIGH_LEVEL	2
#define GPIO_EINT_LOW_LEVEL		3
#define GPIO_EINT_BOTH_EDGE		4


void set_pin_mode(uint32_t port_addr, uint32_t pin, uint32_t mode);
void set_pin_data(uint32_t port_addr, uint32_t pin, uint32_t data);
int get_pin_data(uint32_t port_addr, uint32_t pin);
void set_pin_drive(uint32_t port_addr, uint32_t pin, uint32_t strength);
void set_pin_pull(uint32_t port_addr, uint32_t pin, uint32_t pull);

void gpio_irq_set_trigger(uint32_t port_addr, int pin, uint8_t mode);
void gpio_irq_enable(uint32_t port_addr, int pin, int enable);
void gpio_irq_ack(uint32_t port_addr);

void gpio_init();

#ifdef __cplusplus
}
#endif
