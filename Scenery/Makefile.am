libdir	= ${exec_prefix}/lib

lib_LTLIBRARIES = libScenery.la

libScenery_la_SOURCES = \
	obj.c obj.h \
	scenery.c scenery.h \
	texload.c texload.h \
	tilecache.c tilecache.h \
	tilemgr.c tilemgr.h

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Simulator
