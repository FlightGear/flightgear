libdir	= ${exec_prefix}/lib

lib_LTLIBRARIES = libScenery.la

libScenery_la_SOURCES = \
	obj.cxx obj.hxx \
	scenery.cxx scenery.hxx \
	texload.c texload.h \
	tile.cxx tile.hxx \
	tilecache.cxx tilecache.hxx \
	tilemgr.cxx tilemgr.hxx

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Simulator
