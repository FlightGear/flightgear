# libdir	= ${exec_prefix}/lib
# lib_LTLIBRARIES = libEnvironment.la
# libEnvironment_la_SOURCES = environment.c environment.h

noinst_LIBRARIES = libEnvironment.a

libEnvironment_a_SOURCES = environment.cxx environment.hxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
