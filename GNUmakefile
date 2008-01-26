-include $(shell uname).mk
CFLAGS		+=	-I $(FLTKDIR) -I src/FLU
CPPFLAGS	+=	-I $(FLTKDIR) 
EXECUTABLE	=	virtualt
CLIENT		=	vt_client

FLTKCONFIG	=	$(FLTKDIR)/fltk-config
VPATH		=	src:obj

#CFLAGS		+=	`$(FLTKCONFIG) --cflags --use-images`
LDFLAGS		+=	-g `$(FLTKCONFIG) --ldstaticflags --use-images` -L /usr/X11R6/lib -L $(FLTKDIR)/lib

OBJECTS		=	$(SOURCES:.c=.o)
OBJECTSCPP	=	$(SOURCESCPP:.cpp=.o)
CLIENT_OBJS	=	$(CLIENT_SRC:.cpp=.o)
LIBFILES	=	-lstdc++ -lfltk -lfltk_images -lfltk_jpeg -lfltk_png -lfltk_z -lm -lc -lX11 -lpthread
#LIBFILES	=	/$(FLTKDIR)/lib/libfltk.a ../$(FLTKDIR)/lib/libfltk_images.a ../$(FLTKDIR)/lib/libfltk_jpeg.a ../$(FLTKDIR)/lib/libfltk_png.a \
#				/$(FLTKDIR)/lib/libfltk_z.a -lm -lc -lX11
OBJDIR		=   obj


# =============================
# Define all source files below
# =============================
SOURCES		=	m100emu.c doins.c genwrap.c serial.c intelhex.c memory.c m100rom.c \
				m200rom.c n8201rom.c romstrings.c sound.c io.c m10rom.c
SOURCESCPP	=	display.cpp setup.cpp periph.cpp disassemble.cpp file.cpp memedit.cpp cpuregs.cpp \
				a85parse.cpp assemble.cpp MString.cpp MStringArray.cpp rpn_eqn.cpp vtobj.cpp \
				Flu_DND.cpp flu_pixmaps.cpp Flu_Tree_Browser.cpp FluSimpleString.cpp ide.cpp \
				multiwin.cpp multiwin_icons.cpp	project.cpp multieditwin.cpp rememcfg.cpp \
				fl_usage_box.cpp remote.cpp socket.cpp serversocket.cpp
CLIENT_SRC	=	clientsocket.cpp vt_client_main.cpp socket.cpp

# ========================
# Rule for all files below
# ========================
all:				virtualt vt_client
virtualt:			$(SOURCES) $(SOURCESCPP)
vt_client:		$(CLIENT_SRC)

$(EXECUTABLE):	$(OBJECTS) $(OBJECTSCPP)
ifndef FLTKDIR
	@echo "FLTKDIR environment variable must be set first!"
else
	cd obj; gcc $(LDFLAGS) $(OBJECTS) $(OBJECTSCPP) $(LIBFILES) -o ../$@
	cd ..
endif

$(CLIENT):		$(CLIENT_OBJS)
ifndef FLTKDIR
	@echo "FLTKDIR environment variable must be set first!"
else
	cd obj; gcc $(LDFLAGS) $(CLIENT_OBJS) $(LIBFILES) -o ../$@
	cd ..
endif


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
$(CLIENT_OBJS):	socket.h

disassemble.o:	disassemble.h io.h cpu.h periph.h memedit.h romstrings.h
display.o:		display.h io.h file.h setup.h periph.h memory.h memedit.h
doins.o:		cpu.h io.h
file.o:			memory.h roms.h intelhex.h
io.o:			cpu.h gen_defs.h io.h serial.h display.h setup.h memory.h
intelhex.o:		intelhex.h
m100emu.o:		io.h cpu.h doins.h display.h genwrap.h filewrap.h roms.h \
				intelhex.h setup.h memory.h do_instruct.h
memedit.o:		memedit.h disassemble.h memory.h cpu.h
memory.o:		memory.h cpu.h io.h intelhex.h setup.h
periph.o:		periph.h serial.h setup.h display.h disassemble.h
serial.o:		serial.h setup.h display.h
rememcfg.o:		rememcfg.h setup.h display.h
fl_usage_box.o: fl_usage_box.h
setup.o:		setup.h io.h serial.h memory.h memedit.h
sound.c:		sound.h
m100rom.o m102rom.o m200rom.o n8201rom.o romstrings.o m10rom.o: roms.h romstrings.h
remote.o:		remote.cpp m100emu.h socket.h serversocket.h socketexception.h
socket.o:		socket.h
serversocket.o:	serversocket.h

# ==========
# asm files
# ==========
a85parse.o:		a85parse.h assemble.h
assemble.o:		assemble.h
MString.o:		MString.h
MStringArray.o:	MStringArray.h MString.h
rpn_eqn.o:		rpn_eqn.h
vtobj.o:		vtobj.h

# ==========
# ide files 
# ==========
Flu_DND.o:		FLU/Flu_DND.h
flu_pixmaps.o:	FLU/flu_pixmaps.h
Flu_Tree_Browser.o:	FLU/Flu_Tree_Browser.h
FluSimpleString.o:	FLU/FluSimpleString.h
ide.o:			ide.h vtobj.h multiwin.h multieditwin.h
multiwin.o:		multiwin.h vtobj.h
multiwin.o:		multieditwin.h multiwin.h vtobj.h
multiwin_icons.o:	multiwin_icons.h	
project.o:		project.h

# ==========
# client files 
# ==========
clientsocket.o:			clientsocket.h socket.h
vt_client_main.o:		clientsocket.h socket.h

# =============================
# Rule to clean all build files
# =============================
clean:
	cd obj; rm *.o; cd ..
	rm virtualt
	rm vt_client

