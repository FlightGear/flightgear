#!/bin/sh

#
# Run this to generate a new configure script and such  :)
# 
#	-- Rob
#


(libtoolize --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have libtool installed to compile libiax";
	echo;
	exit;
}

libtoolize --copy --force
aclocal
autoconf
automake
