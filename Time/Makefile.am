if HAVE_DAYLIGHT
DEFS += -DHAVE_DAYLIGHT
endif

if HAVE_TIMEZONE
DEFS += -DHAVE_TIMEZONE
endif

noinst_LIBRARIES = libTime.a

libTime_a_SOURCES = \
	event.cxx event.hxx \
	fg_time.cxx fg_time.hxx \
	fg_timer.cxx fg_timer.hxx \
	light.cxx light.hxx \
	sunpos.cxx sunpos.hxx \
	timestamp.hxx

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Simulator
