PREFIX=~/x-tools/arm-unknown-eabihf/bin/arm-unknown-eabihf-
#PREFIX=arm-linux-gnueabihf-

CC=$(PREFIX)gcc
CXX=$(PREFIX)g++
OBJCOPY=$(PREFIX)objcopy
AR=$(PREFIX)ar

CFLAGS_COMMON = -MMD -O3 -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 -ffreestanding -DREENTRANT_SYSCALLS_PROVIDED -D__DYNAMIC_REENT__
#CFLAGS_COMMON += -fstack-protector-all

CFLAGS=-T $(OSDIR)/linker.ld $(CFLAGS_COMMON) -nostdlib -Wall -Wextra \
	-I $(OSDIR) -I $(OSDIR)/tinyusb/src
CXXFLAGS=$(CFLAGS_COMMON) -nostdlib -Wall -Wextra -I $(OSDIR) -I $(OSDIR)/tinyusb/src

%.o: %.s
	$(CC) $(CFLAGS) -o $@ -c $<

%.bin: %.elf
	$(OBJCOPY) -O binary --remove-section .uncached $< $@

