#include "ports.h"
#include "ccu.h"

void gpio_init() {
  BUS_CLK_GATING2 |= (1<<5);
  APB0_CLK_GATING |= (1<<0);
}

void set_pin_mode(uint32_t port_addr, uint32_t pin, uint32_t mode) {
  struct port_registers * port = (struct port_registers *)port_addr;
  if(pin < 8) {
    port->cfg0 &= ~(7 << ((pin - 0) * 4));
    port->cfg0 |= (mode << ((pin - 0) * 4));
  } else if(pin < 16) {
    port->cfg1 &= ~(7 << ((pin - 8) * 4));
    port->cfg1 |= (mode << ((pin - 8) * 4));
  } else if(pin < 24) {
    port->cfg2 &= ~(7 << ((pin - 16) * 4));
    port->cfg2 |= (mode << ((pin - 16) * 4));
  } else {
    port->cfg3 &= ~(7 << ((pin - 24) * 4));
    port->cfg3 |= (mode << ((pin - 24) * 4));
  }
}

void set_pin_data(uint32_t port_addr, uint32_t pin, uint32_t data) {
  struct port_registers * port = (struct port_registers *)port_addr;
  if(data) {
    port->data |= (1 << pin);
  } else {
    port->data &= ~(1 << pin);
  }
}

int get_pin_data(uint32_t port_addr, uint32_t pin) {
  struct port_registers * port = (struct port_registers *)port_addr;
  return !!(port->data & (1 << pin));
}
