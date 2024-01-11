#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>

#define BIT(n) (1UL << (n))

#define MEM(a) (*(volatile uint32_t *)(a))
#define MEM8(a) (*(volatile uint8_t *)(a))

#endif
