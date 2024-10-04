#! /bin/sh
set -e

if [ "$(uname)" = Darwin ]; then
    brew install dylibbundler tcl-tk
fi

if [ "$(uname)" = Linux ]; then
    apt-get install -y graphicsmagick pkgconf
fi

if [ "$(uname)" = FreeBSD ]; then
    pkg install -y gmake GraphicsMagick pkgconf png tcl86
    ln -s /usr/local/bin/tclsh8.6 /usr/local/bin/tclsh
fi

if [ "$(uname)" = NetBSD ]; then
    pkgin -y install gmake GraphicsMagick pkgconf png tcl zlib
fi

if [ "$(uname)" = OpenBSD ]; then
    pkg_add -I gmake GraphicsMagick pkgconf png tcl%8.6
    ln -s /usr/local/bin/tclsh8.6 /usr/local/bin/tclsh
fi
