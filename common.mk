LIBH3DIR ?= $(OSDIR)/lib-h3

PREFIX=~/x-tools/arm-unknown-eabihf/bin/arm-unknown-eabihf-
#PREFIX=arm-linux-gnueabihf-

CC=$(PREFIX)gcc
CXX=$(PREFIX)g++
OBJCOPY=$(PREFIX)objcopy
AR=$(PREFIX)ar

CFLAGS_COMMON = -MMD -g -O2 -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 -ffreestanding \
		-DREENTRANT_SYSCALLS_PROVIDED -D__DYNAMIC_REENT__ \
		-I $(LIBH3DIR)/lib-h3/include -I $(LIBH3DIR)/lib-arm/include -DORANGE_PI_ONE

# debugging: detect stack smashing
#CFLAGS_COMMON += -fstack-protector-strong

# debugging: sanitize important stuff
#CFLAGS_COMMON += -fsanitize=object-size -fsanitize=null -fsanitize=bounds -fsanitize=alignment -fsanitize-address-use-after-scope

# debugging: sanitize everything that is supported
#CFLAGS_COMMON += -fsanitize=undefined -fno-sanitize=float-cast-overflow -fno-sanitize=pointer-overflow -fno-sanitize=vptr -fsanitize=bounds-strict

# debugging: enable GDB stub
#GDB = 1

ifneq ($(GDB),)
CFLAGS_COMMON += -DGDBSTUB
endif

CFLAGS=-T $(OSDIR)/linker.ld $(CFLAGS_COMMON) -nostdlib -Wall -Wextra \
	-I $(OSDIR) -I $(OSDIR)/tinyusb/src
CXXFLAGS=$(CFLAGS_COMMON) -nostdlib -Wall -Wextra -I $(OSDIR) -I $(OSDIR)/tinyusb/src

%.o: %.s
	$(CC) $(CFLAGS) -o $@ -c $<

%.bin: %.elf
	$(OBJCOPY) -O binary --remove-section .uncached $< $@

