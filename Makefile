#-include $(shell uname).mk
CFLAGS	+=	-O6
#CFLAGS	+=	-g

FLTKCONFIG	?=	fltk-config
CFLAGS	+=	`$(FLTKCONFIG) --cflags --use-images`
LDFLAGS +=	`$(FLTKCONFIG) --ldstaticflags --use-images`

m100emu: doins.o io.o genwrap.o display.o m100emu.o disassemble.o file.o memory.o m100rom.o intelhex.o setup.o serial.o periph.o GNUmakefile
	gcc -o m100emu doins.o genwrap.o io.o display.o m100emu.o disassemble.o file.o memory.o m100rom.o intelhex.o setup.o serial.o periph.o $(LDFLAGS)

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

file.o: file.cpp memory.h roms.h intelhex.h m100emu.h GNUmakefile
	gcc $(CFLAGS) -c file.cpp -o file.o

memory.o: memory.c genwrap.h GNUmakefile
	gcc $(CFLAGS) -c memory.c -o memory.o

m100rom.o: m100rom.c roms.h GNUmakefile
	gcc $(CFLAGS) -c m100rom.c -o m100rom.o

intelhex.o: intelhex.c intelhex.h GNUmakefile
	gcc $(CFLAGS) -c intelhex.c -o intelhex.o  

setup.o: setup.cpp setup.h m100emu.h io.h serial.h GNUmakefile
	gcc $(CFLAGS) -c setup.cpp -o setup.o  

serial.o: serial.c serial.h setup.h display.h m100emu.h GNUmakefile
	gcc $(CFLAGS) -c serial.c -o serial.o  

periph.o: periph.cpp periph.h serial.h setup.h display.h m100emu.h disassemble.h GNUmakefile
	gcc $(CFLAGS) -c periph.cpp -o periph.o  

clean:
	rm *.o
	rm m100emu
