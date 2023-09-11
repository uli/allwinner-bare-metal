#include "ccu.h"
#include "ports.h"
#include "util.h"

void gpio_init()
{
  BUS_CLK_GATING2 |= (1 << 5);
  APB0_CLK_GATING |= (1 << 0);
}

void set_pin_mode(uint32_t port_addr, uint32_t pin, uint32_t mode)
{
  volatile struct port_registers *port = (volatile struct port_registers *)port_addr;
  if (pin < 8) {
    port->cfg0 &= ~(7 << ((pin - 0) * 4));
    port->cfg0 |= (mode << ((pin - 0) * 4));
  } else if (pin < 16) {
    port->cfg1 &= ~(7 << ((pin - 8) * 4));
    port->cfg1 |= (mode << ((pin - 8) * 4));
  } else if (pin < 24) {
    port->cfg2 &= ~(7 << ((pin - 16) * 4));
    port->cfg2 |= (mode << ((pin - 16) * 4));
  } else {
    port->cfg3 &= ~(7 << ((pin - 24) * 4));
    port->cfg3 |= (mode << ((pin - 24) * 4));
  }
}

void set_pin_data(uint32_t port_addr, uint32_t pin, uint32_t data)
{
  volatile struct port_registers *port = (volatile struct port_registers *)port_addr;
  if (data) {
    port->data |= (1 << pin);
  } else {
    port->data &= ~(1 << pin);
  }
}

int get_pin_data(uint32_t port_addr, uint32_t pin)
{
  volatile struct port_registers *port = (volatile struct port_registers *)port_addr;
  return !!(port->data & (1 << pin));
}

void set_pin_drive(uint32_t port_addr, uint32_t pin, uint32_t strength)
{
  volatile struct port_registers *port = (struct port_registers *)port_addr;
  if (pin < 16) {
    port->drv0 &= ~(3 << pin * 2);
    port->drv0 |= strength << pin * 2;
  } else {
    pin -= 16;
    port->drv1 &= ~(3 << pin * 2);
    port->drv1 |= strength << pin * 2;
  }
}

void set_pin_pull(uint32_t port_addr, uint32_t pin, uint32_t pull)
{
  volatile struct port_registers *port = (struct port_registers *)port_addr;
  if (pin < 16) {
    port->pul0 &= ~(3 << pin * 2);
    port->pul0 |= pull << pin * 2;
  } else {
    pin -= 16;
    port->pul1 &= ~(3 << pin * 2);
    port->pul1 |= pull << pin * 2;
  }
}

void gpio_irq_set_trigger(uint32_t port_addr, int pin, uint8_t mode)
{
	//set_pin_mode(port_addr + 0x200, pin, mode);
	volatile struct port_irq_registers *port = (volatile struct port_irq_registers *)(port_addr);

	if (pin < 8) {
		port->cfg0 &= ~(7 << ((pin - 0) * 4));
		port->cfg0 |= (mode << ((pin - 0) * 4));
	} else if (pin < 16) {
		port->cfg1 &= ~(7 << ((pin - 8) * 4));
		port->cfg1 |= (mode << ((pin - 8) * 4));
	} else if (pin < 24) {
		port->cfg2 &= ~(7 << ((pin - 16) * 4));
		port->cfg2 |= (mode << ((pin - 16) * 4));
	} else {
		port->cfg3 &= ~(7 << ((pin - 24) * 4));
		port->cfg3 |= (mode << ((pin - 24) * 4));
	}
}

void gpio_irq_enable(uint32_t port_addr, int pin, int enable)
{
  volatile struct port_irq_registers *port = (volatile struct port_irq_registers *)(port_addr);// + 0x200);
  if (enable)
    port->ctl |= BIT(pin);
  else
    port->ctl &= ~BIT(pin);
}

void gpio_irq_ack(uint32_t port_addr)
{
  volatile struct port_irq_registers *port = (volatile struct port_irq_registers *)(port_addr);// + 0x200);
  port->status = port->status;
}
