include $(OSDIR)/common.mk

all: $(TARGET).bin

$(TARGET).elf: $(OBJS) $(OSDIR)/libos.a $(OSDIR)/linker.ld
	$(CC) $(CFLAGS) -o $(TARGET).elf $(OBJS) -lc -L $(OSDIR) $(LIBS) -los -lc -lm -lgcc

install: $(TARGET).bin
	sudo sunxi-fel spl $(OSDIR)/sunxi-spl.bin write 0x5c000000 $(TARGET).bin exe 0x5c000000
