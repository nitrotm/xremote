CC:=g++

CFLAGS:=-pipe -c #-g
INCLUDES:=-I/usr/include -Iinclude -Ithirdparty

LDFLAGS:=-pipe -L/usr/lib -L/usr/X11R6/lib -lX11 -lXtst -lpthread

OUTPUT:=xremote
SRC_C:= \
	thirdparty/aes.c \
	thirdparty/sha2.c
SRC_CPP:= \
	src/xrclient.cpp \
	src/xrdisplay.cpp \
	src/xrnet.cpp \
	src/xrnetudp.cpp \
	src/xremote.cpp \
	src/xrscreen.cpp \
	src/xrserver.cpp \
	src/xrwindow.cpp
OBJ_C:=$(SRC_C:.c=.o)
OBJ_CPP:=$(SRC_CPP:.cpp=.oxx)


all: $(OUTPUT)
	@chmod 755 $(OUTPUT)

compile:
	@echo "compiling..."

clean:
	@echo "cleaning..."
	@echo " *.o"
	@rm -f $(OBJ_C) $(OBJ_CPP) tests/*.o*

distclean: clean
	@echo "cleaning binary..."
	@echo " $(OUTPUT) test-*"
	@rm -f $(OUTPUT) test-*


$(OUTPUT): compile $(OBJ_C) $(OBJ_CPP)
	@echo "building $(OUTPUT)..."
	@$(CC) $(LDFLAGS) -o $(OUTPUT) $(OBJ_C) $(OBJ_CPP)
	@echo "done."

test-streams: compile $(OBJ_C) $(OBJ_CPP) tests/streams.oxx
	@echo "building test-streams..."
	@$(CC) $(LDFLAGS) -o test-streams $(OBJ_C) $(OBJ_CPP) tests/streams.oxx
	@echo "done."

%.o: %.c
	@echo " $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

%.oxx: %.cpp
	@echo " $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<


.PHONY: all compile clean distclean
