noinst_LIBRARIES = libTime.a

libTime_a_SOURCES = \
	event.cxx event.hxx \
	fg_timer.cxx fg_timer.hxx \
	light.cxx light.hxx \
	moonpos.cxx moonpos.hxx \
	sunpos.cxx sunpos.hxx \
	tmp.cxx tmp.hxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
