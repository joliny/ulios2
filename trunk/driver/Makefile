######################
#	Makefile for basesrv
#	作者：孙亮
#	功能：基础服务组建脚本
#	最后修改日期：2010-03-09
######################

# Entry point of ulios basesrv
ENTRYPOINT	= 0x08000000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f aout
CFLAGS		= -c -O1 -Wall -m32 -fno-builtin -fleading-underscore -fno-stack-protector
LDFILE		= ../MkApi/app.ld
LDFLAGS		= -T $(LDFILE) -mi386go32
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
HEADS		= ../MkApi/ulimkapi.h basesrv.h
OBJS		= ../objs/athd.o ../objs/time.o ../objs/kbdmus.o ../objs/vesa.o ../objs/gdi.o ../objs/cui.o ../objs/rep.o ../objs/speaker.o ../objs/uart.o
HEADOBJ		= ../objs/apphead.o

# All Phony Targets
.PHONY : all redo clean

# Default starting position

all : ../out/ulios/athd.bin ../out/ulios/time.bin ../out/ulios/kbdmus.bin ../out/ulios/vesa.bin ../out/ulios/cui.bin ../out/ulios/rep.bin ../out/ulios/speaker.bin ../out/ulios/uart.bin

redo : clean all

clean :
	rm -f $(OBJS) $(HEADOBJ) $(BINS)

../out/ulios/athd.bin : ../objs/athd.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/athd.o

../out/ulios/time.bin : ../objs/time.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/time.o

../out/ulios/kbdmus.bin : ../objs/kbdmus.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/kbdmus.o

../out/ulios/vesa.bin : ../objs/vesa.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/vesa.o

../out/ulios/cui.bin : ../objs/gdi.o ../objs/cui.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/gdi.o ../objs/cui.o

../out/ulios/rep.bin : ../objs/rep.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/rep.o

../out/ulios/speaker.bin : ../objs/speaker.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/speaker.o

../out/ulios/uart.bin : ../objs/uart.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/uart.o

../objs/apphead.o : ../MkApi/apphead.c
	$(CC) $(CFLAGS) -o $@ $<

../objs/athd.o : athd.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/time.o : time.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/kbdmus.o : kbdmus.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/vesa.o : vesa.c $(HEADS) ../fs/fsapi.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/gdi.o : gdi.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/cui.o : cui.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/rep.o : rep.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/speaker.o : speaker.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/uart.o : uart.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<
