libdir	= ${exec_prefix}/lib

lib_LTLIBRARIES = libScenery.la

libScenery_la_SOURCES = \
	obj.c obj.h \
	scenery.c scenery.h \
	texload.c texload.h \
	tilecache.cxx tilecache.hxx \
	tilemgr.cxx tilemgr.hxx

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Simulator
