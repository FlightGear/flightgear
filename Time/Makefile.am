libdir	= ${exec_prefix}/lib

lib_LTLIBRARIES = libTime.la

libTime_la_SOURCES = \
	event.c event.h \
	fg_time.c fg_time.h \
	fg_timer.c fg_timer.h \
	light.cxx light.hxx \
	sunpos.cxx sunpos.hxx

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Simulator
