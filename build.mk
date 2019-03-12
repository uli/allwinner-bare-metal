include $(OSDIR)/common.mk

$(TARGET).elf: $(OBJS) $(OSDIR)/libos.a $(OSDIR)/linker.ld
	$(CC) $(CFLAGS) -o $(TARGET).elf $(OBJS) -L$(OSDIR) -los -L$(OSDIR)/newlib/newlib -lc -lm

install: $(TARGET).bin
	sudo sunxi-fel spl ./sunxi-spl.bin write 0x4e000000 $(TARGET).bin exe 0x4e000000

