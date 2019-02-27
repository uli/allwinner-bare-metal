CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
CFLAGS=-T linker.ld -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 -fpic -ffreestanding -O3 -nostdlib -Wextra -std=gnu99

os.bin: os.elf
	$(OBJCOPY) -O binary --remove-section .uncached os.elf os.bin
os.elf: boot.o startup.o uart.o ports.o mmu.o system.o display.o interrupts.o spritelayers.o usb.o demo.o
	$(CC) $(CFLAGS) -o os.elf boot.o startup.o uart.o ports.o mmu.o system.o display.o interrupts.o spritelayers.o usb.o demo.o

boot.o: boot.s
	$(CC) $(CFLAGS) -c boot.s
startup.o: startup.c
	$(CC) $(CFLAGS) -c startup.c
uart.o: uart.c
	$(CC) $(CFLAGS) -c uart.c
ports.o: ports.c
	$(CC) $(CFLAGS) -c ports.c
mmu.o: mmu.c
	$(CC) $(CFLAGS) -c mmu.c
system.o: system.c
	$(CC) $(CFLAGS) -c system.c
display.o: display.c
	$(CC) $(CFLAGS) -c display.c
interrupts.o: interrupts.c
	$(CC) $(CFLAGS) -c interrupts.c
spritelayers.o: spritelayers.c
	$(CC) $(CFLAGS) -c spritelayers.c
usb.o: usb.c
	$(CC) $(CFLAGS) -c usb.c
demo.o: demo.c
	$(CC) $(CFLAGS) -c demo.c

clean:
	rm -f *.o os.*

install: os.bin
	sudo sunxi-fel spl ./sunxi-spl.bin write 0x4e000000 os.bin exe 0x4e000000
