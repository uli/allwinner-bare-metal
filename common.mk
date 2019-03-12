PREFIX=arm-none-eabi-
#PREFIX=arm-linux-gnueabihf-
CC=$(PREFIX)gcc
CXX=$(PREFIX)g++
OBJCOPY=$(PREFIX)objcopy
AR=$(PREFIX)ar
CFLAGS_BIN=-O3 -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 -fpic -ffreestanding
CFLAGS_COMMON=$(CFLAGS_BIN) -std=gnu99
CFLAGS=-MMD -T $(OSDIR)/linker.ld $(CFLAGS_COMMON) -nostdlib -Wall -Wextra \
	-I$(OSDIR) -I tinyusb/src -I newlib/newlib/include
CXXFLAGS=$(CFLAGS_BIN) -nostdlib -Wall -Wextra -I tinyusb/src -I newlib/newlib/include

%.o: %.s
	$(CC) $(CFLAGS) -o $@ -c $<

%.bin: %.elf
	$(OBJCOPY) -O binary --remove-section .uncached $< $@

