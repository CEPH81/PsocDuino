# psocprog - build:  make            (Linux/macOS, needs libusb-1.0)
#                    make NO_LIBUSB=1 (no USB backend; sim only, for tests)
#                    make windows     (MinGW i686: uses CH341DLL.DLL)
CC ?= cc
CFLAGS ?= -O2
CFLAGS += -std=c99 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Wall -Wextra
SRC = src/main.c src/issp.c src/ihex.c src/vectors.c src/ch341_proto.c \
      src/ch341_libusb.c src/ch341_dll.c src/ch341_sim.c
ifndef NO_LIBUSB
  USB_CFLAGS ?= $(shell pkg-config --cflags libusb-1.0)
  USB_LIBS ?= $(shell pkg-config --libs libusb-1.0)
  CFLAGS += -DHAVE_LIBUSB $(USB_CFLAGS)
  LIBS += $(USB_LIBS)
endif

psocprog: $(SRC) src/*.h
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LIBS)

windows: $(SRC) src/*.h
	$(MINGW)gcc -O2 -std=c99 -Wall -static -o psocprog.exe $(SRC)

test: 
	$(MAKE) NO_LIBUSB=1 psocprog
	sh test/run_tests.sh ./psocprog

clean:
	rm -f psocprog psocprog.exe
.PHONY: windows test clean
