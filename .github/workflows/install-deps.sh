#! /bin/sh
set -e

if [ "$(uname)" = Linux ]; then
    apt-get install -y graphicsmagick pkgconf
fi

if [ "$(uname)" = FreeBSD ]; then
    pkg install -y gmake GraphicsMagick png pkgconf tcl86
    ln -s /usr/local/bin/tclsh8.6 /usr/local/bin/tclsh
fi

if [ "$(uname)" = NetBSD ]; then
        pkgin -y install gmake GraphicsMagick png pkgconf tcl zlib
fi

if [ "$(uname)" = OpenBSD ]; then
    pkg_add -I gmake GraphicsMagick png pkgconf tcl%8.6
    ln -s /usr/local/bin/tclsh8.6 /usr/local/bin/tclsh
fi
