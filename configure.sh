#!/bin/bash

# configurable parameters
test -n "$STACK_PROT" && STACK_PROT_FLAGS="-fstack-protector-strong"
test -n "$UBSAN" && UBSAN_FLAGS="-fsanitize=object-size -fsanitize=null -fsanitize=bounds -fsanitize=alignment -fsanitize-address-use-after-scope"
test -n "$UBSAN_FULL" && UBSAN_FLAGS="-fsanitize=undefined -fno-sanitize=float-cast-overflow -fno-sanitize=pointer-overflow -fno-sanitize=vptr -fsanitize=bounds-strict"
test "$GDBSTUB" == "on" && GDB=1 || GDB=0
test "$LIBH3_MMC" == "off" && LIBH3_MMC=0 || LIBH3_MMC=1

test -z "$OSDIR" && OSDIR=`pwd`
test -z "$LIBH3DIR" && LIBH3DIR=$OSDIR/lib-h3
test -z "$LWIPDIR" && LWIPDIR=$OSDIR/lwip/src
test -z "$CROSS_COMPILE" && CROSS_COMPILE=~/x-tools/arm-unknown-eabihf/bin/arm-unknown-eabihf-
test -z "$OBJDIR" && OBJDIR=$OSDIR/build

test "$LIBH3_MMC" == 1 && LIBH3_MMC_FLAGS="-DLIBH3_MMC -DSD_WRITE_SUPPORT"
test "$GDB" == 1 && GDB_FLAGS="-DGDB"

OPT_FLAGS="$STACK_PROT_FLAGS $UBSAN_FLAGS $GDB_FLAGS $LIBH3_MMC_FLAGS"

test -z "$MAKE" && MAKE=make

test -z "$CC" && CC=${CROSS_COMPILE}gcc
test -z "$CXX" && CXX=${CROSS_COMPILE}g++
test -z "$OBJCOPY" && OBJCOPY=${CROSS_COMPILE}objcopy
test -z "$AR" && AR=${CROSS_COMPILE}ar

test -e build.ninja && ninja -t clean

cat <<EOT >build.ninja.common
aw_cc = $CC
aw_cxx = $CXX

aw_common_flags = -g -O2 -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 -ffreestanding \$
  -DREENTRANT_SYSCALLS_PROVIDED -D__DYNAMIC_REENT__ -DALLWINNER_BARE_METAL \$
  -I $LIBH3DIR/lib-h3/include -I $LIBH3DIR/lib-arm/include -I $LIBH3DIR/lib-hal/include -DORANGE_PI_ONE \$
  -I $LWIPDIR/include $OPT_FLAGS

aw_cflags = -T $OSDIR/linker.ld \$aw_common_flags -nostdlib -Wall -Wextra \$
  -I $OSDIR -I $OSDIR/tinyusb/src

aw_cxxflags = \$aw_common_flags -nostdlib -Wall -Wextra -I $OSDIR -I $OSDIR/tinyusb/src

rule cc
  depfile = \$out.d
  command = $CC -MD -MF \$out.d \$aw_cflags \$cflags -c \$in -o \$out
rule cxx
  depfile = \$out.d
  command = $CXX -MD -MF \$out.d \$aw_cxxflags \$cxxflags -c \$in -o \$out
rule link
  command = $CC \$aw_cflags \$cflags -o \$out \$in -Wl,--wrap,__stack_chk_fail -Wl,-wrap,__malloc_lock -Wl,-wrap,__malloc_unlock -lc \$
            -L$OSDIR -L$LIBH3DIR/lib-h3/lib_h3 -L$LIBH3DIR/lib-arm/lib_h3 \$libs -los -lh3 -larm -lc -lm -lgcc

rule bin
  command = $OBJCOPY -O binary --remove-section .uncached \$in \$out
rule uimg
  command = mkimage -A arm -O u-boot -T firmware -d \$in \$out -C none -a 0x40000000

rule sdimg
  command = dd if=/dev/zero of=\$out.fs bs=1M count=60 ; \$
            mkfs.vfat -F 32 \$out.fs ; \$
            dd if=/dev/zero of="\$out" bs=1M count=4 ; \$
            cat \$out.fs >>"\$out" ; \$
            rm -f \$out.fs ; \$
            parted -s "\$out" mklabel msdos ; \$
            parted -s "\$out" mkpart primary fat32 4 67 ; \$
            dd if=$OSDIR/sunxi-spl.bin of="\$out" conv=notrunc bs=1024 seek=8 ; \$
            dd if="\$in" of="\$out" conv=notrunc bs=1024 seek=40 ; \$
            test -e "\$initfs_dir" && mcopy -s -i \${out}@@4M \$initfs_dir/* ::

rule upload
  command = for i in 1 2 3 4 5 ; do sudo sunxi-fel -v -p spl $OSDIR/sunxi-spl.bin write 0x40000000 \$
            \$in exe 0x40000000 && break ; sleep .5 ; done
  pool = console
EOT

cat <<EOT >build.ninja
include build.ninja.common

rule libh3
  command = $MAKE $LIBH3DIR/lib-h3 -f Makefile.H3 PREFIX=$CROSS_COMPILE PLATFORM=ORANGE_PI_ONE
rule libarm
  command = $MAKE $LIBH3DIR/lib-arm -f Makefile.H3 PREFIX=$CROSS_COMPILE PLATFORM=ORANGE_PI_ONE

build libh3: libh3
build libarm: libarm

EOT

# convert LWIP file list from make to shell format
tr '()\t' '{} ' <$LWIPDIR/Filelists.mk |grep -v '#'|grep -v '^$'|sed 's, ,\\ ,g' >lwip.files
. lwip.files
rm lwip.files

SOURCES="boot.S startup.c uart.c ports.c mmu.c system.c display.c interrupts.c \
	usb.c fs.c audio_i2s.c exceptions.c cache.S display_filter.c \
	dma.c rtc.c smp.c spinlock.c ubsan.c tve.c \
	tinyusb/src/host/ohci/ohci1.c tinyusb/src/host/ohci/ohci2.c tinyusb/src/host/ohci/ohci3.c\
	tinyusb/src/host/usbh1.c tinyusb/src/host/usbh2.c tinyusb/src/host/usbh3.c \
	tinyusb/src/host/hub1.c tinyusb/src/host/hub2.c tinyusb/src/host/hub3.c \
	tinyusb/src/class/hid/hid_host1.c tinyusb/src/class/hid/hid_host2.c tinyusb/src/class/hid/hid_host3.c \
	tinyusb/src/common/tusb_fifo.c \
	tinyusb/src/tusb1.c tinyusb/src/tusb2.c tinyusb/src/tusb3.c \
	tinyusb/src/class/msc/msc_host1.c tinyusb/src/class/msc/msc_host2.c tinyusb/src/class/msc/msc_host3.c \
	tinyusb/lib/fatfs/diskio1.c tinyusb/lib/fatfs/diskio2.c tinyusb/lib/fatfs/diskio3.c \
	libc_io.c \
	fatfs/ff.c fatfs/ffunicode.c \
	network.c ${COREFILES} ${CORE4FILES} ${NETIFFILES} ${HTTPFILES} ${TFTPFILES} $LWIPDIR/api/err.c"

test "$GDB" == 1 && SOURCES="$SOURCES gdb/tzvecs.c gdb/gdbstub.c gdb/string.c gdb/printk.c"
if test "$LIBH3_MMC" == 1; then
  SOURCES="$SOURCES lib-h3/lib-hal/src/h3/sdcard/diskio.c"
else
  SOURCES="$SOURCES fatfs/mmc_sunxi.c"
fi


cat <<EOT >build.ninja
include build.ninja.common

rule link_lib
  command = bash -c "rm -f \$out; $AR rc \$out \$in"

EOT

{
echo -n "build libos.a: link_lib "
for s in $SOURCES ; do
  echo -n "$OBJDIR/${s%.*}.o "
done
echo

for s in $SOURCES ; do
  echo "build $OBJDIR/${s%.*}.o: cc $s"
done
} >>build.ninja
