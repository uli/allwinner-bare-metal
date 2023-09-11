LIBH3DIR ?= $(OSDIR)/lib-h3
LWIPDIR ?= $(OSDIR)/lwip/src

PREFIX=~/x-tools/arm-unknown-eabihf/bin/arm-unknown-eabihf-
#PREFIX=arm-linux-gnueabihf-

OBJDIR ?= build

CC=$(PREFIX)gcc
CXX=$(PREFIX)g++
OBJCOPY=$(PREFIX)objcopy
AR=$(PREFIX)ar

CFLAGS_COMMON = -MMD -g -O2 -mfpu=neon -mfloat-abi=hard -mtune=cortex-a7 -ffreestanding -Wa,-mimplicit-it=thumb \
		-DREENTRANT_SYSCALLS_PROVIDED -D__DYNAMIC_REENT__ -DALLWINNER_BARE_METAL \
		-I $(LIBH3DIR)/lib-h3/include -I $(LIBH3DIR)/lib-arm/include -I $(LIBH3DIR)/lib-hal/include -DNANOPI_NEO_AIR \
		-I $(LWIPDIR)/include

# debugging: detect stack smashing
#CFLAGS_COMMON += -fstack-protector-strong

# debugging: sanitize important stuff
#CFLAGS_COMMON += -fsanitize=object-size -fsanitize=null -fsanitize=bounds -fsanitize=alignment -fsanitize-address-use-after-scope

# debugging: sanitize everything that is supported
#CFLAGS_COMMON += -fsanitize=undefined -fno-sanitize=float-cast-overflow -fno-sanitize=pointer-overflow -fno-sanitize=vptr -fsanitize=bounds-strict

# debugging: enable GDB stub
#GDB = 1

# use lib-h3 MMC driver
LIBH3_MMC = 1

ifneq ($(GDB),)
CFLAGS_COMMON += -DGDBSTUB
endif

ifneq ($(LIBH3_MMC),)
CFLAGS_COMMON += -DLIBH3_MMC -DSD_WRITE_SUPPORT
endif

CFLAGS=-T $(OSDIR)/linker.ld $(CFLAGS_COMMON) -nostdlib -Wall -Wextra \
	-I $(OSDIR) -I $(OSDIR)/tinyusb/src
CXXFLAGS=$(CFLAGS_COMMON) -nostdlib -Wall -Wextra -I $(OSDIR) -I $(OSDIR)/tinyusb/src

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

%.bin: %.elf
	$(OBJCOPY) -O binary --remove-section .uncached $< $@
