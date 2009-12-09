######################
#	Makefile for ulios
#	作者：孙亮
#	功能：微内核组建脚本
#	最后修改日期：2009-07-02
######################

# Entry point of ulios MicroKernel
ENTRYPOINT	= 0x10000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f elf
CFLAGS		= -c -O1 -Wall
LDFILE		= MicroKernel/ulios.ld
LDFLAGS		= -T $(LDFILE) -Map map.txt
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
KERNELBIN	= uliknl.bin
FLOPPY		= ulios.img
HEADS		= MicroKernel/bootdata.h MicroKernel/cintr.h MicroKernel/debug.h MicroKernel/error.h MicroKernel/exec.h MicroKernel/ipc.h MicroKernel/kalloc.h MicroKernel/knldef.h MicroKernel/page.h MicroKernel/task.h MicroKernel/ulidef.h MicroKernel/x86cpu.h
OBJS		= objs/head.o objs/intr.o objs/cintr.o objs/debug.o objs/exec.o objs/global.o objs/ipc.o objs/kalloc.o objs/page.o objs/task.o objs/ulios.o

# All Phony Targets
.PHONY : all redo run floppy vm vpc app clean

# Default starting position

all : $(KERNELBIN)

redo : clean all

run : floppy
	bochs -q -f ulios.bxrc

floppy : $(KERNELBIN)
	mount $(FLOPPY) /mnt/floppy -o loop
	cp -f $(KERNELBIN) /mnt/floppy
	umount /mnt/floppy

vm : $(KERNELBIN)
	dd if=$(KERNELBIN) of=/mnt/hgfs/vm/ulios-flat.vmdk seek=129 conv=notrunc

vpc : $(KERNELBIN)
	dd if=$(KERNELBIN) of=/mnt/hgfs/vpc/ULIOS.VHD seek=36 conv=notrunc

app : MkApi/app.ld MkApi/apphead.asm MkApi/ulimkapi.h test.c
	$(ASM) $(ASMFLAGS) -o objs/apphead.o MkApi/apphead.asm
	$(CC) $(CFLAGS) -o objs/test.o test.c
	$(LD) -T MkApi/app.ld -Map appmap.txt -o test objs/apphead.o objs/test.o
	dd if=test of=/mnt/hgfs/vm/ulios-flat.vmdk seek=257 conv=notrunc

clean :
	rm -f $(OBJS) $(KERNELBIN)

$(KERNELBIN) : $(OBJS) $(LDFILE)
	$(LD) $(LDFLAGS) -o $(KERNELBIN) $(OBJS)

objs/head.o : MicroKernel/head.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

objs/intr.o : MicroKernel/intr.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

objs/cintr.o: MicroKernel/cintr.c  $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

objs/debug.o: MicroKernel/debug.c  $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

objs/exec.o: MicroKernel/exec.c  $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

objs/global.o: MicroKernel/global.c  $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

objs/ipc.o: MicroKernel/ipc.c  $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

objs/kalloc.o: MicroKernel/kalloc.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

objs/page.o: MicroKernel/page.c  $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

objs/task.o: MicroKernel/task.c  $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

objs/ulios.o: MicroKernel/ulios.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<
