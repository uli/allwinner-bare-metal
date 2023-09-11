OSDIR = .

include common.mk

OBJS = boot.o startup.o uart.o ports.o mmu.o system.o display.o interrupts.o \
       usb.o fs.o audio_i2s.o audio_hdmi.o exceptions.o cache.o display_filter.o \
       dma.o rtc.o smp.o spinlock.o ubsan.o tve.o \
       lib-h3/lib-h3/src/h3.o lib-h3/lib-h3/src/h3_cpu.o lib-h3/lib-h3/src/h3_smp.o

USB_OBJS = tinyusb/src/host/ohci/ohci1.o tinyusb/src/host/ohci/ohci2.o tinyusb/src/host/ohci/ohci3.o\
	   tinyusb/src/host/usbh1.o tinyusb/src/host/usbh2.o tinyusb/src/host/usbh3.o \
	   tinyusb/src/host/hub1.o tinyusb/src/host/hub2.o tinyusb/src/host/hub3.o \
	   tinyusb/src/class/hid/hid_host1.o tinyusb/src/class/hid/hid_host2.o tinyusb/src/class/hid/hid_host3.o \
	   tinyusb/src/common/tusb_fifo.o \
	   tinyusb/src/tusb1.o tinyusb/src/tusb2.o tinyusb/src/tusb3.o \
	   tinyusb/src/class/msc/msc_host1.o tinyusb/src/class/msc/msc_host2.o tinyusb/src/class/msc/msc_host3.o \
	   tinyusb/lib/fatfs/diskio1.o tinyusb/lib/fatfs/diskio2.o tinyusb/lib/fatfs/diskio3.o

ifneq ($(GDB),)
GDB_OBJS = gdb/tzvecs.o gdb/gdbstub.o gdb/string.o gdb/printk.o
endif

LIBC_CSRC = libc_io.c
LIBC_OBJS = $(LIBC_CSRC:.c=.o)

ifneq ($(LIBH3_MMC),)
SD_OBJS = lib-h3/lib-hal/src/h3/sdcard/diskio.o
else
SD_OBJS = fatfs/mmc_sunxi.o
endif

SD_OBJS += fatfs/ff.o fatfs/ffunicode.o

LWIPDIR = $(OSDIR)/lwip/src
-include $(LWIPDIR)/Filelists.mk
NET_CSRC = network.c $(COREFILES) $(CORE4FILES) $(NETIFFILES) $(HTTPFILES) $(TFTPFILES) $(LWIPDIR)/api/err.c
NET_OBJS = $(NET_CSRC:.c=.o)

ALL_OBJS = $(OBJS) $(GDB_OBJS) $(USB_OBJS) $(LIBC_OBJS) $(SD_OBJS) $(NET_OBJS)
OUT_ALL_OBJS = $(addprefix $(OBJDIR)/, $(ALL_OBJS))

all:	libos.a libh3 libarm

libos.a: $(OUT_ALL_OBJS) Makefile
	rm -f $@
	$(AR) rc $@ $(OUT_ALL_OBJS)

libh3:
	$(MAKE) -C $(LIBH3DIR)/lib-h3 -f Makefile.H3 PREFIX=$(PREFIX) PLATFORM=NANOPI_NEO_AIR
libarm:
	$(MAKE) -C $(LIBH3DIR)/lib-arm -f Makefile.H3 PREFIX=$(PREFIX) PLATFORM=NANOPI_NEO_AIR

clean:
	rm -fr build libos.a
	$(MAKE) -C $(LIBH3DIR)/lib-h3 -f Makefile.H3 clean
	$(MAKE) -C $(LIBH3DIR)/lib-arm -f Makefile.H3 clean

-include $(OUT_ALL_OBJS:%.o=%.d)
