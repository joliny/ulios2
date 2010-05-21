######################
#	Makefile for basesrv
#	作者：孙亮
#	功能：基础服务组建脚本
#	最后修改日期：2010-03-09
######################

# Entry point of ulios basesrv
ENTRYPOINT	= 0x08048000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f aout
CFLAGS		= -c -O1 -Wall
LDFILE		= ../MkApi/app.ld
LDFLAGS		= -T $(LDFILE)
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
HEADS		= ../MkApi/ulimkapi.h basesrv.h
OBJS		= ../objs/athd.o ../objs/time.o ../objs/kbdmus.o ../objs/vesa.o
HEADOBJ		= ../objs/apphead.o

# All Phony Targets
.PHONY : all redo clean

# Default starting position

all : ../out/ULIOS/athd.bin ../out/ULIOS/time.bin ../out/ULIOS/kbdmus.bin ../out/ULIOS/vesa.bin

redo : clean all

clean :
	rm -f $(OBJS) $(HEADOBJ) $(BINS)

../out/ULIOS/athd.bin : ../objs/athd.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/athd.o

../out/ULIOS/time.bin : ../objs/time.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/time.o

../out/ULIOS/kbdmus.bin : ../objs/kbdmus.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/kbdmus.o

../out/ULIOS/vesa.bin : ../objs/vesa.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/vesa.o

../objs/apphead.o : ../MkApi/apphead.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

../objs/athd.o : athd.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/time.o : time.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/kbdmus.o : kbdmus.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/vesa.o : vesa.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<
