#!/bin/sh

# Simple script to bake a windows obdsim build
# Uses mingw on linux. Please read cmakemodules/Toolchain-mingw32.cmake

# Run from top level obdgpslogger checkout with
#   "sh cmakemodules/build-obdsim-windows.sh"

TOPLEVEL="`basename $0`/../"
BUILDDIR="build-mingw"

cd $TOPLEVEL

rm -rf "$BUILDDIR"
mkdir -p "$BUILDDIR"

cd "$BUILDDIR"

cmake -DCMAKE_TOOLCHAIN_FILE=../cmakemodules/Toolchain-mingw32.cmake ..
make obdsim

mkdir obdsimwindows
cp ../bin/obdsim.exe ./obdsimwindows/
cp ../doc/obdsim-on-windows.txt ./obdsimwindows/
MANWIDTH=80; export MANWIDTH
man -M ../man obdsim | col -b > ./obdsimwindows/obdsim-manpage.txt

ZIPFILE="obdsimwindows-`date +%F`.zip"

zip -r9 "$ZIPFILE" obdsimwindows

scp "$ZIPFILE" i.o:/webspace/projects/obdgpslogger/downloads

ssh i.o "cd /webspace/projects/obdgpslogger/downloads/; rm -f obdsimwindows-latest.zip; ln -s \"$ZIPFILE\" obdsimwindows-latest.zip"

echo "http://icculus.org/obdgpslogger/downloads/$ZIPFILE"

