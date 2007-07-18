SRC= \
	thirdparty/aes.c \
	thirdparty/sha2.c \
	xremote.cpp \
    xrscreen.cpp \
	xrdisplay.cpp \
	xrevents.cpp \
	xrwindow.cpp \
	xrnet.cpp \
	xrclient.cpp \
	xrservercon.cpp \
	xrserver.cpp \
	xrservertest.cpp

CC=g++
CC_FLAGS=-O3 -g -Wall -L /usr/X11R6/lib -lX11 -lXtst -lpthread #-lXext
OUT=xremote
PREFIX=/usr/local

all: $(OUT)
	@chmod 755 ${OUT}

$(OUT): $(SRC)
	@echo "Building ${OUT}..."
	$(CC) $(CC_FLAGS) -o $(OUT) $(SRC)
	@echo "Done."

clean:
	@rm -f ${OUT}

install: all
	@cp -i ${OUT} ${PREFIX}/bin/${OUT}

