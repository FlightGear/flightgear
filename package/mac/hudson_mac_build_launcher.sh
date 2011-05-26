#!/bin/sh

SDK_PATH="/Developer/SDKs/MacOSX10.6.sdk"
OSX_TARGET="10.6"

svn co https://macflightgear.svn.sourceforge.net/svnroot/macflightgear/trunk/FlightGearOSX  macflightgear

pushd macflightgear

# compile the stub executable
gcc -o FlightGear -mmacosx-version-min=$OSX_TARGET -isysroot $SDK_PATH -arch i386 main.m \
    -framework Cocoa -framework RubyCocoa -framework Foundation -framework AppKit

popd

