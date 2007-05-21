#!/bin/sh

echo "Running aclocal"
aclocal

echo "Running autoheader"
autoheader
if [ ! -e src/Include/config.h.in ]; then
    echo "ERROR: autoheader didn't create simgear/simgear_config.h.in!"
    exit 1
fi    

echo "Running automake --add-missing"
automake --add-missing

echo "Running autoconf"
autoconf

if [ ! -e configure ]; then
    echo "ERROR: configure was not created!"
    exit 1
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
