#! /bin/sh
set -e

if [ "$(uname)" = Linux ]; then
    sudo apt install -y graphicsmagick
fi

if [ "$(uname)" = FreeBSD ]; then
    sudo pkg install -y gmake GraphicsMagick tcl86
    sudo ln -s /usr/local/bin/tclsh8.6 /usr/local/bin/tclsh
fi

if [ "$(uname)" = NetBSD ]; then
    sudo pkgin -y install gmake GraphicsMagick tcl
fi

if [ "$(uname)" = OpenBSD ]; then
    # doas(1) isn't configured.
    # See https://github.com/cross-platform-actions/action/issues/75
    sudo pkg_add -I gmake GraphicsMagick tcl%8.6
    sudo ln -s /usr/local/bin/tclsh8.6 /usr/local/bin/tclsh
fi

gmake test
