#CFLAGS	:=	-O6 -march=pentium -mcpu=pentium
#CFLAGS	:=	-O -pg
#LDFLAGS :=	-pg
#CFLAGS	:=	-O2 -g
CFLAGS	+=	-O6

FLTKCONFIG	?=	fltk-config
CFLAGS	+=	`$(FLTKCONFIG) --cflags --use-images`
LDFLAGS +=	`$(FLTKCONFIG) --ldstaticflags --use-images`

m100emu: doins.o io.o genwrap.o display.o m100emu.o disassemble.o Makefile
	gcc -o m100emu doins.o genwrap.o io.o display.o m100emu.o disassemble.o $(LDFLAGS)

m100emu.o: m100emu.c cpu.h doins.h display.h genwrap.h do_instruct.h Makefile
	gcc $(CFLAGS) -c m100emu.c -o m100emu.o
	
doins.o: doins.c doins.h cpu.h io.h m100emu.h Makefile
	gcc $(CFLAGS) -c doins.c -o doins.o

genwrap.o: genwrap.c genwrap.h gen_defs.h Makefile
	gcc $(CFLAGS) -c genwrap.c -o genwrap.o

io.o: cpu.h gen_defs.h io.h io.c Makefile
	gcc $(CFLAGS) -c io.c -o io.o

display.o: display.cpp display.h io.h m100emu.h Makefile
	g++ $(CFLAGS) -c display.cpp -o display.o

disassemble.o: disassemble.cpp m100emu.h disassemble.h m100rom.h io.h Makefile
	g++ $(CFLAGS) -c disassemble.cpp -o disassemble.o

clean:
	rm *.o
	rm m100emu
