#!/bin/sh

OSTYPE=`uname -s`
MACHINE=`uname -m`
AUTO_MAKE_VERSION=`automake --version | head -1 | awk '{print $4}' | sed -e 's/\.\([0-9]*\).*/\1/'`
if test $AUTO_MAKE_VERSION -lt 15; then
    echo ""
    echo "You need to upgrade to automake version 1.5 or greater."
    echo "Most distributions have packages available to install or you can"
    echo "find the source for the most recent version at"
    echo "ftp://ftp.gnu.org/gnu/automake"
    exit 1
fi

echo "Host info: $OSTYPE $MACHINE"
echo -n " automake: `automake --version | head -1 | awk '{print $4}'`"
echo " ($AUTO_MAKE_VERSION)"
echo ""

ACLOCAL_OPTS=""
if [ $AUTO_MAKE_VERSION -ge 14 ]; then
    if [ $OSTYPE = "IRIX" -o $OSTYPE = "IRIX64" ]; then    echo " -I ."
        ACLOCAL_OPTS="-I ."
    fi
fi
echo "Running aclocal $ACLOCAL_OPTS"
aclocal $ACLOCAL_OPTS

echo "Running autoheader"
autoheader
if [ ! -e src/Include/config.h.in ]; then
    echo "ERROR: autoheader didn't create simgear/simgear_config.h.in!"
    exit 1
fi    

echo -n "Running automake"
if [ $OSTYPE = "IRIX" -o $OSTYPE = "IRIX64" ]; then
    echo " --add-missing --include-deps"
    automake --add-missing --include-deps
else
    echo " --add-missing"
    automake --add-missing
fi

echo "Running autoconf"
autoconf

if [ ! -e configure ]; then
    echo "ERROR: configure was not created!"
    exit 1
fi

# fixup Makefiles for Irix
if test "$OSTYPE" = "IRIX" -o "$OSTYPE" = "IRIX64"; then
    echo "Fixing Makefiles for Irix"
    for n in `find . -name Makefile.in`; do \
        mv -f $n $n.ar-new; \
        sed 's/$(AR) cru/$(AR) -o/g' $n.ar-new > $n; \
        rm -f $n.ar-new; \
    done;
fi

echo ""
echo "======================================"

if [ -f config.cache ]; then
    echo "config.cache exists.  Removing the config.cache file will force"
    echo "the ./configure script to rerun all it's tests rather than using"
    echo "the previously cached values."
    echo ""
fi

echo "Now you are ready to run './configure'"
echo "======================================"
