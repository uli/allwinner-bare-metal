OSDIR = .

include common.mk

OBJS = boot.o startup.o uart.o ports.o mmu.o system.o display.o interrupts.o \
       usb.o fs.o audio_i2s.o exceptions.o cache.o display_filter.o \
       dma.o rtc.o smp.o spinlock.o ubsan.o

USB_OBJS = tinyusb/src/host/ohci/ohci1.o tinyusb/src/host/ohci/ohci2.o \
	   tinyusb/src/host/usbh1.o tinyusb/src/host/usbh2.o \
	   tinyusb/src/host/hub1.o tinyusb/src/host/hub2.o \
	   tinyusb/src/class/hid/hid_host1.o tinyusb/src/class/hid/hid_host2.o \
	   tinyusb/src/common/tusb_fifo.o \
	   tinyusb/src/tusb1.o tinyusb/src/tusb2.o

ifneq ($(GDB),)
GDB_OBJS = gdb/tzvecs.o gdb/gdbstub.o gdb/string.o gdb/printk.o
endif

LIBC_CSRC = libc_io.c
LIBC_OBJS = $(LIBC_CSRC:.c=.o)

SD_OBJS = fatfs/mmc_sunxi.o fatfs/ff.o fatfs/ffunicode.o

ALL_OBJS = $(OBJS) $(GDB_OBJS) $(USB_OBJS) $(LIBC_OBJS) $(SD_OBJS)

libos.a: $(ALL_OBJS) Makefile $(LIBH3DIR)/lib-h3/lib_h3/libh3.a $(LIBH3DIR)/lib-arm/lib_h3/libarm.a
	rm -f $@
	$(AR) rc $@ $(ALL_OBJS)

$(LIBH3DIR)/lib-h3/lib_h3/libh3.a:
	$(MAKE) -C $(LIBH3DIR)/lib-h3 -f Makefile.H3 PREFIX=$(PREFIX) PLATFORM=ORANGE_PI_ONE
$(LIBH3DIR)/lib-arm/lib_h3/libarm.a:
	$(MAKE) -C $(LIBH3DIR)/lib-arm -f Makefile.H3 PREFIX=$(PREFIX) PLATFORM=ORANGE_PI_ONE

clean:
	rm -f $(ALL_OBJS) $(ALL_OBJS:%.o=%.d) libos.a
	$(MAKE) -C $(LIBH3DIR) -f Makefile.H3 clean

-include $(ALL_OBJS:%.o=%.d)
