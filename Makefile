PREFIX=arm-none-eabi-
#PREFIX=arm-linux-gnueabihf-
CC=$(PREFIX)gcc
CXX=$(PREFIX)g++
OBJCOPY=$(PREFIX)objcopy
CFLAGS_BIN=-O3 -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 -fpic -ffreestanding
CFLAGS_COMMON=$(CFLAGS_BIN) -std=gnu99
CFLAGS=-MMD -T linker.ld $(CFLAGS_COMMON) -nostdlib -Wall -Wextra \
	-I tinyusb/src -I newlib/newlib/include
CXXFLAGS=$(CFLAGS_BIN) -nostdlib -Wall -Wextra -I tinyusb/src -I newlib/newlib/include

OBJS = boot.o startup.o uart.o ports.o mmu.o system.o display.o interrupts.o \
       spritelayers.o usb.o demo.o fs.o audio_hdmi.o audio_i2s.o exceptions.o

USB_OBJS = tinyusb/src/host/ohci/ohci.o tinyusb/src/host/usbh.o tinyusb/src/host/hub.o \
	tinyusb/src/class/hid/hid_host.o tinyusb/src/common/tusb_fifo.o \
	tinyusb/src/tusb.o

LIBC_CSRC = libc_io.c
LIBC_OBJS = $(LIBC_CSRC:.c=.o)

SD_OBJS = sdgpio/mmc_sunxi.o sdgpio/ff.o

ALL_OBJS = $(OBJS) $(USB_OBJS) $(LIBC_OBJS) $(SD_OBJS)

os.bin: os.elf
	$(OBJCOPY) -O binary --remove-section .uncached os.elf os.bin
os.elf: $(ALL_OBJS) newlib/newlib/libc.a
	$(CC) $(CFLAGS) -o os.elf $(ALL_OBJS) -Lnewlib/newlib -lc

%.o: %.s
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(ALL_OBJS) os.*

install: os.bin
	sudo sunxi-fel spl ./sunxi-spl.bin write 0x4e000000 os.bin exe 0x4e000000

newlib/newlib/libc.a: newlib/newlib/Makefile
	make -C newlib/newlib

newlib/newlib/Makefile:
	cd newlib/newlib ; CC=$(CC) CFLAGS="$(CFLAGS_COMMON)" ./configure \
	  --host=arm-none-eabi --disable-newlib-supplied-syscalls

-include $(ALL_OBJS:%.o=%.d)
