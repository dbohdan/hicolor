WIN32_CC ?= i686-w64-mingw32-gcc
CFLAGS ?= -O3 -static -Wall -Wextra
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

release: clean test test-win32
	strip hicolor hicolor.exe
	cp hicolor hicolor-v"$$(./hicolor version)"-linux-x86_64
	cp hicolor.exe hicolor-v"$$(./hicolor version)"-win32.exe

test: all
	./tests/hicolor.test
test-win32: hicolor.exe
	HICOLOR_COMMAND='wine ../hicolor.exe' WINEDEBUG=-all ./tests/hicolor.test

.PHONY: all clean install-bin install-include test test-win32
