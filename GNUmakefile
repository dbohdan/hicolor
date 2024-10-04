PLATFORM ?= $(shell uname)

ifneq ($(PLATFORM), Darwin)
    PLATFORM_CFLAGS ?= -static -Wl,--gc-sections
endif

LIBPNG_CFLAGS ?= $(shell pkg-config --cflags libpng)
LIBPNG_LIBS ?= $(shell pkg-config --libs libpng)
ZLIB_CFLAGS ?= $(shell pkg-config --cflags zlib)
ZLIB_LIBS ?= $(shell pkg-config --libs zlib)

CFLAGS ?= -g -O3 $(PLATFORM_CFLAGS) -ffunction-sections -fdata-sections -Wall -Wextra $(LIBPNG_CFLAGS) $(ZLIB_CFLAGS)
LIBS ?= $(LIBPNG_LIBS) $(ZLIB_LIBS) -lm
PREFIX ?= /usr/local

all: hicolor

hicolor: cli.c hicolor.h
	$(CC) $< -o $@ $(CFLAGS) $(LIBS)
clean: clean-no-ext clean-exe
clean-exe:
	-rm -f hicolor.exe
clean-no-ext:
	-rm -f hicolor

install: install-bin install-include
install-bin: hicolor
	install $< $(DESTDIR)$(PREFIX)/bin/hicolor
install-include: hicolor.h
	install -m 0644 $< $(DESTDIR)$(PREFIX)/include

uninstall: uninstall-bin uninstall-include
uninstall-bin:
	-rm $(DESTDIR)$(PREFIX)/bin/hicolor
uninstall-include:
	-rm $(DESTDIR)$(PREFIX)/include/hicolor.h

release: clean-no-ext test
	cp hicolor hicolor-v"$$(./hicolor version | head -n 1 | awk '{ print $$2 }')"-"$$(uname | tr 'A-Z' 'a-z')"-"$$(uname -m)"

test: all
	tests/hicolor.test

.PHONY: all clean clean-exe clean-no-ext install install-bin install-include release test uninstall uninstall-bin uninstall-include
