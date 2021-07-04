WIN32_CC ?= i686-w64-mingw32-gcc
CFLAGS ?= -static -Wall -Wextra
PREFIX ?= /usr/local

all: hicolor

hicolor: cli.c hicolor.h
	$(CC) $< -o $@ $(CFLAGS)
hicolor.exe: cli.c hicolor.h
	$(WIN32_CC) $< -o $@ $(CFLAGS)
clean:
	-rm -f hicolor hicolor.exe

install: install-bin install-include
install-bin: hicolor
	install $< $(DESTDIR)$(PREFIX)/bin/hicolor
install-include: picol.h
	install -m 0644 $< $(DESTDIR)$(PREFIX)/include

.PHONY: all clean install-bin install-include
