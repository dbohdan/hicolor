#! /bin/sh
set -e

if [ "$(uname)" = Linux ]; then
    sudo apt install -y graphicsmagick
fi
if [ "$(uname)" = NetBSD ]; then
    sudo pkgin -y install gmake tcl
fi

gmake test
