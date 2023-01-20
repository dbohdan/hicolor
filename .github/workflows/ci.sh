#! /bin/sh
set -e

if [ "$(uname)" = Linux ]; then
    sudo apt install -y graphicsmagick
fi
if [ "$(uname)" = FreeBSD ]; then
    sudo pkg install gmake graphicsmagick tcl86
    ln -s /usr/local/bin/tclsh8.6 /usr/local/bin/tclsh
fi
if [ "$(uname)" = NetBSD ]; then
    sudo pkgin -y install gmake GraphicsMagick tcl
fi
if [ "$(uname)" = OpenBSD ]; then
    # No doas(1) on the VM?
    sudo pkg_add gmake GraphicsMagick tcl-8.6.12
fi

gmake test
