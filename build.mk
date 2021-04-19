include $(OSDIR)/common.mk

all: $(TARGET).bin

OUT_OBJS = $(addprefix $(OBJDIR)/, $(OBJS))

$(TARGET).elf: $(OUT_OBJS) $(OSDIR)/libos.a $(LIBH3DIR)/lib-h3/lib_h3/libh3.a $(LIBH3DIR)/lib-arm/lib_h3/libarm.a $(OSDIR)/linker.ld
	$(CC) $(CFLAGS) -o $(TARGET).elf $(OUT_OBJS) -Wl,--wrap,__stack_chk_fail -Wl,-wrap,__malloc_lock -Wl,-wrap,__malloc_unlock -lc \
	  -L $(OSDIR) -L $(LIBH3DIR)/lib-h3/lib_h3 -L $(LIBH3DIR)/lib-arm/lib_h3 $(LIBS) -los -lh3 -larm -lc -lm -lgcc

install: $(TARGET).bin
	for i in 1 2 3 4 ; do sudo sunxi-fel -v -p spl $(OSDIR)/sunxi-spl.bin write 0x40000000 $(TARGET).bin exe 0x40000000 && break ; sleep .5 ; done

image: $(TARGET)_sd.img

$(TARGET).uimg: $(TARGET).bin
	mkimage -A arm -O u-boot -T firmware -d $< $@ -C none -a 0x40000000

$(TARGET)_sd.img: $(TARGET).uimg $(OSDIR)/sunxi-spl.bin
	dd if=/dev/zero of=$@.fs bs=1M count=60
	mkfs.vfat -F 32 $@.fs
	dd if=/dev/zero of="$@" bs=1M count=4
	cat $@.fs >>"$@"
	rm -f $@.fs
	parted -s "$@" mklabel msdos
	parted -s "$@" mkpart primary fat32 4 67
	dd if=$(OSDIR)/sunxi-spl.bin of="$@" conv=notrunc bs=1024 seek=8
	dd if=$(TARGET).uimg of="$@" conv=notrunc bs=1024 seek=40
	test -e "$(INIT_FS_DIR)" && mcopy -s -i $(TARGET)_sd.img@@4M $(INIT_FS_DIR)/* ::
