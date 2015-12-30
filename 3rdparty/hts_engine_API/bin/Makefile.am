
EXTRA_DIST = Makefile.mak

AM_CPPFLAGS = -I @top_srcdir@/include

bin_PROGRAMS = hts_engine

hts_engine_SOURCES = hts_engine.c 

hts_engine_LDADD = ../lib/libHTSEngine.a

DISTCLEANFILES = *.log *.out *~

MAINTAINERCLEANFILES = Makefile.in
