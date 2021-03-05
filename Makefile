OSDIR = .

include common.mk

OBJS = boot.o startup.o uart.o ports.o mmu.o system.o display.o interrupts.o \
       usb.o fs.o audio_hdmi.o audio_i2s.o exceptions.o cache.o

USB_OBJS = tinyusb/src/host/ohci/ohci.o tinyusb/src/host/usbh.o tinyusb/src/host/hub.o \
	tinyusb/src/class/hid/hid_host.o tinyusb/src/common/tusb_fifo.o \
	tinyusb/src/tusb.o

LIBC_CSRC = libc_io.c
LIBC_OBJS = $(LIBC_CSRC:.c=.o)

SD_OBJS = fatfs/mmc_sunxi.o fatfs/ff.o fatfs/ffunicode.o

ALL_OBJS = $(OBJS) $(USB_OBJS) $(LIBC_OBJS) $(SD_OBJS)

libos.a: $(ALL_OBJS) Makefile
	rm -f $@
	$(AR) rc $@ $(ALL_OBJS)

clean:
	rm -f $(ALL_OBJS) $(ALL_OBJS:%.o=%.d) libos.a

-include $(ALL_OBJS:%.o=%.d)
