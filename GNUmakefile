-include $(shell uname).mk
EXECUTABLE	:=	virtualt
VPATH		:=	src:obj

FLTKCONFIG	?=	fltk-config

CFLAGS		+=	`$(FLTKCONFIG) --cflags --use-images`
CPPFLAGS	+=	`$(FLTKCONFIG) --cflags --use-images`

LIBFILES	=	-lm -lc -lX11
OBJDIR		=   obj


# =============================
# Define all source files below
# =============================
SOURCES		=	m100emu.c doins.c genwrap.c serial.c intelhex.c memory.c m100rom.c \
				m200rom.c n8201rom.c romstrings.c sound.c io.c
SOURCESCPP	=	display.cpp setup.cpp periph.cpp disassemble.cpp file.cpp memedit.cpp cpuregs.cpp
OBJECTS		=	$(SOURCES:.c=.o)
OBJECTSCPP	=	$(SOURCESCPP:.cpp=.o)


# ========================
# Rule for all files below
# ========================
all:			$(SOURCES) $(SOURCESCPP) $(EXECUTABLE)

$(EXECUTABLE):	$(OBJECTS) $(OBJECTSCPP)
	cd obj; g++ $(LDFLAGS) $(OBJECTS) $(OBJECTSCPP) `$(FLTKCONFIG) --ldstaticflags --use-images` $(LIBFILES) -o ../$@
	cd ..


# ===============================
# Rule for compiling source files
# ===============================
.cpp.o:
	-mkdir -p obj; g++ $(CPPFLAGS) -c $< -o obj/$@

.c.o:
	-mkdir -p obj; gcc $(CFLAGS) -c $< -o obj/$@


# ==========================================
# Declare dependencies on header files below
# ==========================================
$(OBJECTS) $(OBJECTSCPP): m100emu.h GNUmakefile VirtualT.h

disassemble.o:	disassemble.h io.h cpu.h periph.h memedit.h romstrings.h
display.o:		display.h io.h file.h setup.h periph.h memory.h memedit.h
doins.o:		cpu.h io.h
file.o:			memory.h roms.h intelhex.h
io.o:			cpu.h gen_defs.h io.h serial.h display.h setup.h memory.h
intelhex.o:		intelhex.h
m100emu.o:		io.h cpu.h doins.h display.h genwrap.h filewrap.h roms.h \
				intelhex.h setup.h memory.h
memedit.o:		memedit.h disassemble.h memory.h cpu.h
memory.o:		memory.h cpu.h io.h intelhex.h setup.h
periph.o:		periph.h serial.h setup.h display.h disassemble.h cpuregs.h
serial.o:		serial.h setup.h display.h
setup.o:		setup.h io.h serial.h memory.h memedit.h
sound.c:		sound.h
cpuregs.cpp:	cpuregs.h disassemble.h periph.h memedit.h cpu.h
m100rom.o m102rom.o m200rom.o n8201rom.o romstrings.o: roms.h romstrings.h


# =============================
# Rule to clean all build files
# =============================
clean:
	cd obj; rm *.o; cd ..
	rm virtualt

