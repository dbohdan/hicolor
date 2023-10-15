WIN32_CC ?= i686-w64-mingw32-gcc
CFLAGS ?= -g -O3 -static -ffunction-sections -fdata-sections -Wl,--gc-sections -lm -Wall -Wextra
PREFIX ?= /usr/local

all: hicolor

hicolor: cli.c hicolor.h vendor/cute_png.h
	$(CC) $< -o $@ $(CFLAGS)
hicolor.exe: cli.c hicolor.h vendor/cute_png.h
	$(WIN32_CC) $< -o $@ $(CFLAGS)
clean: clean-no-ext clean-exe
clean-no-ext:
	-rm -f hicolor
clean-exe:
	-rm -f hicolor.exe

install: install-bin install-include
install-bin: hicolor
	install $< $(DESTDIR)$(PREFIX)/bin/hicolor
install-include: hicolor.h
	install -m 0644 $< $(DESTDIR)$(PREFIX)/include

release: clean-no-ext test
	cp hicolor hicolor-v"$$(./hicolor version)"-"$$(uname | tr 'A-Z' 'a-z')"-"$$(uname -m)"

test: all
	tests/hicolor.test
test-wine: hicolor.exe
	HICOLOR_COMMAND='wine ../hicolor.exe' WINEDEBUG=-all tests/hicolor.test

.PHONY: all clean install-bin install-include test test-wine
