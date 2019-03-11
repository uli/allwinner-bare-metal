PREFIX=arm-none-eabi-
#PREFIX=arm-linux-gnueabihf-
CC=$(PREFIX)gcc
OBJCOPY=$(PREFIX)objcopy
CFLAGS=-MMD -T linker.ld -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 -fpic -ffreestanding -O3 -nostdlib -Wall -Wextra -std=gnu99 \
	-I tinyusb/src -I Baselibc/include -I Baselibc/src/templates

OBJS = boot.o startup.o uart.o ports.o mmu.o system.o display.o interrupts.o \
       spritelayers.o usb.o demo.o fs.o audio_hdmi.o audio_i2s.o

USB_OBJS = tinyusb/src/host/ohci/ohci.o tinyusb/src/host/usbh.o tinyusb/src/host/hub.o \
	tinyusb/src/class/hid/hid_host.o tinyusb/src/common/tusb_fifo.o \
	tinyusb/src/tusb.o

LIBC_CSRC = $(wildcard Baselibc/src/*.c) libc_io.c
LIBC_OBJS = $(LIBC_CSRC:.c=.o)

SD_OBJS = sdgpio/mmc_sunxi.o sdgpio/ff.o

ALL_OBJS = $(OBJS) $(USB_OBJS) $(LIBC_OBJS) $(SD_OBJS)

os.bin: os.elf
	$(OBJCOPY) -O binary --remove-section .uncached os.elf os.bin
os.elf: $(ALL_OBJS)
	$(CC) $(CFLAGS) -o os.elf $(ALL_OBJS)

%.o: %.s
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(ALL_OBJS) os.*

install: os.bin
	sudo sunxi-fel spl ./sunxi-spl.bin write 0x4e000000 os.bin exe 0x4e000000

-include $(ALL_OBJS:%.o=%.d)
