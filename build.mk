include $(OSDIR)/common.mk

all: $(TARGET).bin

$(TARGET).elf: $(OBJS) $(OSDIR)/libos.a $(OSDIR)/linker.ld
	$(CC) $(CFLAGS) -o $(TARGET).elf $(OBJS) -lc -L $(OSDIR) $(LIBS) -los -lc -lm -lgcc

install: $(TARGET).bin
	sudo sunxi-fel -v -p spl $(OSDIR)/sunxi-spl.bin write 0x5c000000 $(TARGET).bin exe 0x5c000000

image: $(TARGET)_sd.img

$(TARGET).uimg: $(TARGET).bin
	mkimage -A arm -O u-boot -T firmware -d $< $@ -C none -a 0x5c000000

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
	test -e "$(INIT_FS_DIR)" && mcopy -s -i basic_sd.img@@4M $(INIT_FS_DIR)/* ::
