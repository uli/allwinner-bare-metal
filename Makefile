PREFIX=arm-none-eabi-
#PREFIX=arm-linux-gnueabihf-
CC=$(PREFIX)gcc
OBJCOPY=$(PREFIX)objcopy
CFLAGS=-T linker.ld -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 -fpic -ffreestanding -O3 -nostdlib -Wextra -std=gnu99 \
	-I tinyusb/src

OBJS = boot.o startup.o uart.o ports.o mmu.o system.o display.o interrupts.o spritelayers.o usb.o demo.o \
	tinyusb/src/host/ohci/ohci.o tinyusb/src/host/usbh.o tinyusb/src/host/hub.o \
	tinyusb/src/class/hid/hid_host.o tinyusb/src/common/tusb_fifo.o \
	tinyusb/src/tusb.o

os.bin: os.elf
	$(OBJCOPY) -O binary --remove-section .uncached os.elf os.bin
os.elf: $(OBJS)
	$(CC) $(CFLAGS) -o os.elf $(OBJS)

%.o: %.s
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) os.*

install: os.bin
	sudo sunxi-fel spl ./sunxi-spl.bin write 0x4e000000 os.bin exe 0x4e000000
