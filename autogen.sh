#!/bin/sh

echo "Running aclocal"
aclocal

echo "Running autoheader"
autoheader

echo "Running automake"
automake --add-missing

echo "Running autoconf"
autoconf

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
