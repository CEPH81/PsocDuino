CC ?= cc
CFLAGS ?= -O2
CFLAGS += -std=c99 -D_DEFAULT_SOURCE -Wall -Wextra -Wno-format-truncation
psocbuild: psocbuild.c
	$(CC) $(CFLAGS) -o $@ psocbuild.c
clean:
	rm -f psocbuild psocbuild.exe
