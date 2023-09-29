WIN32_CC ?= i686-w64-mingw32-gcc
CFLAGS ?= -g -O3 -static -ffunction-sections -fdata-sections -Wl,--gc-sections -lm -Wall -Wextra
PREFIX ?= /usr/local

all: hicolor

hicolor: cli.c hicolor.h vendor/cute_png.h
	$(CC) $< -o $@ $(CFLAGS)
hicolor.exe: cli.c hicolor.h vendor/cute_png.h
	$(WIN32_CC) $< -o $@ $(CFLAGS)
clean:
	-rm -f hicolor hicolor.exe

install: install-bin install-include
install-bin: hicolor
	install $< $(DESTDIR)$(PREFIX)/bin/hicolor
install-include: hicolor.h
	install -m 0644 $< $(DESTDIR)$(PREFIX)/include

release: clean test test-wine
	cp hicolor hicolor-v"$$(./hicolor version)"-"$$(uname | tr 'A-Z' 'a-z')"-"$$(uname -m)"
	cp hicolor.exe hicolor-v"$$(./hicolor version)"-win32.exe

test: all
	./tests/hicolor.test
test-wine: hicolor.exe
	HICOLOR_COMMAND='wine ../hicolor.exe' WINEDEBUG=-all ./tests/hicolor.test

.PHONY: all clean install-bin install-include test test-wine
