#CFLAGS	:=	-O6 -march=pentium -mcpu=pentium
#CFLAGS	:=	-O -pg
CFLAGS	:=	-O -g

CFLAGS	+=	-I/usr/local/include
LDFLAGS +=	-L/usr/local/lib -L/usr/X11R6/lib
LIBS	+=	-lfltk -lXext -lX11 -lm

m100emu: doins.o genwrap.o io.o display.o m100emu.o file.o memory.o m100rom.o intelhex.o disassemble.o Makefile
	gcc $(LDFLAGS) -o m100emu doins.o genwrap.o io.o display.o m100emu.o file.o memory.o m100rom.o intelhex.o disassemble.o $(LIBS)

m100emu.o: m100emu.c cpu.h doins.h display.h genwrap.h do_instruct.h Makefile
	gcc $(CFLAGS) -c m100emu.c -o m100emu.o
	
doins.o: doins.c doins.h cpu.h io.h m100emu.h do_instruct.h Makefile
	gcc $(CFLAGS) -c doins.c -o doins.o

genwrap.o: genwrap.c genwrap.h gen_defs.h Makefile
	gcc $(CFLAGS) -c genwrap.c -o genwrap.o

io.o: cpu.h gen_defs.h io.h io.c Makefile
	gcc $(CFLAGS) -c io.c -o io.o

display.o: display.cpp display.h Makefile
	gcc $(CFLAGS) -c display.cpp -o display.o

file.o: file.cpp memory.h roms.h intelhex.h m100emu.h Makefile
	gcc $(CFLAGS) -c file.cpp -o file.o

memory.o: memory.c genwrap.h Makefile
	gcc $(CFLAGS) -c memory.c -o memory.o

m100rom.o: m100rom.c roms.h Makefile
	gcc $(CFLAGS) -c m100rom.c -o m100rom.o

intelhex.o: intelhex.c intelhex.h Makefile
	gcc $(CFLAGS) -c intelhex.c -o intelhex.o  

disassemble.o: disassemble.cpp m100emu.h disassemble.h io.h Makefile
	gcc $(CFLAGS) -c disassemble.c -o disassemble.o


clean:
	rm *.o
	rm m100emu
