-include $(shell uname).mk
CFLAGS	+=	-O6
#CFLAGS	+=	-g

CFLAGS	+=	`fltk-config --cflags --use-images`
LDFLAGS +=	`fltk-config --ldstaticflags --use-images`

m100emu: doins.o io.o genwrap.o display.o m100emu.o disassemble.o GNUmakefile
	gcc -o m100emu doins.o genwrap.o io.o display.o m100emu.o disassemble.o $(LDFLAGS)

m100emu.o: m100emu.c cpu.h doins.h display.h genwrap.h do_instruct.h GNUmakefile
	gcc $(CFLAGS) -c m100emu.c -o m100emu.o
	
doins.o: doins.c doins.h cpu.h io.h m100emu.h GNUmakefile
	gcc $(CFLAGS) -c doins.c -o doins.o

genwrap.o: genwrap.c genwrap.h gen_defs.h GNUmakefile
	gcc $(CFLAGS) -c genwrap.c -o genwrap.o

io.o: cpu.h gen_defs.h io.h io.c GNUmakefile
	gcc $(CFLAGS) -c io.c -o io.o

display.o: display.cpp display.h io.h m100emu.h GNUmakefile
	g++ $(CFLAGS) -c display.cpp -o display.o

disassemble.o: disassemble.cpp m100emu.h disassemble.h m100rom.h io.h GNUmakefile
	g++ $(CFLAGS) -c disassemble.cpp -o disassemble.o

clean:
	rm *.o
	rm m100emu
