# libdir	= ${exec_prefix}/lib
# lib_LTLIBRARIES = libWeather.la
# libWeather_la_SOURCES = weather.c weather.h

noinst_LIBRARIES = libWeather.a

libWeather_a_SOURCES = weather.cxx weather.hxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
