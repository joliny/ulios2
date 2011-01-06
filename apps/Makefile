######################
#	Makefile for application
#	���ߣ�����
#	���ܣ�Ӧ�ó����齨�ű�
#	����޸����ڣ�2009-07-02
######################

# Entry point of ulios application
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
HEADS		= ../MkApi/ulimkapi.h ../driver/basesrv.h
OBJS		= ../objs/gdi.o ../objs/loader.o ../objs/cmd.o ../objs/3demo.o ../objs/3dline.o ../objs/workout.o ../objs/jpg.o ../objs/clock.o
HEADOBJ		= ../objs/apphead.o

# All Phony Targets
.PHONY : all redo clean

# Default starting position

all : ../out/ulios/loader.bin ../out/ulios/cmd.bin ../out/ulios/3demo.bin ../out/ulios/3dline.bin ../out/ulios/workout.bin ../out/ulios/jpg.bin ../out/ulios/clock.bin

redo : clean all

clean :
	rm -f $(OBJS) $(HEADOBJ) $(BINS)

../out/ulios/loader.bin : ../objs/loader.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/loader.o

../out/ulios/cmd.bin : ../objs/cmd.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/cmd.o

../out/ulios/3demo.bin : ../objs/3demo.o ../objs/gdi.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/3demo.o ../objs/gdi.o

../out/ulios/3dline.bin : ../objs/3dline.o ../objs/gdi.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/3dline.o ../objs/gdi.o

../out/ulios/workout.bin : ../objs/workout.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/workout.o

../out/ulios/jpg.bin : ../objs/jpg.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/jpg.o ../objs/gdi.o

../out/ulios/clock.bin : ../objs/clock.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/clock.o ../objs/gdi.o

../objs/apphead.o : ../MkApi/apphead.c
	$(CC) $(CFLAGS) -o $@ $<

../objs/gdi.o : ../driver/gdi.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/loader.o : loader.c $(HEADS) ../fs/fsapi.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/cmd.o : cmd.c $(HEADS) ../fs/fsapi.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/3demo.o : 3demo.c $(HEADS) math.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/3dline.o : 3dline.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/workout.o : workout.c $(HEADS) math.h
	$(CC) $(CFLAGS) -o $@ $<

../objs/jpg.o : jpg.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/clock.o : clock.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<