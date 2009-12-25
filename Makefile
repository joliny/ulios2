######################
#	Makefile for app
#	作者：孙亮
#	功能：应用程序组建脚本
#	最后修改日期：2009-07-02
######################

ENTRYPOINT	= 0x08048000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f elf
CFLAGS		= -c -O1 -Wall
LDFILE		= MkApi/app.ld
LDFLAGS		= -T $(LDFILE) -Map objs/appmap.txt
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
APPBIN		= out/ULIOS/test.bin
HEADS		= MkApi/ulimkapi.h
OBJS		= objs/testhead.o objs/test.o

# All Phony Targets
.PHONY : all redo run clean

# Default starting position

all : $(APPBIN)

redo : clean all

run : all
	bochs -q -f ulios.bxrc

clean :
	rm -f $(OBJS) $(APPBIN)

$(APPBIN) : $(OBJS) $(LDFILE)
	$(LD) $(LDFLAGS) -o $(APPBIN) $(OBJS)

objs/testhead.o : MkApi/apphead.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

objs/test.o : test.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<
