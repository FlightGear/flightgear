noinst_LIBRARIES = libSound.a

libSound_a_SOURCES = \
	beacon.cxx beacon.hxx \
	fg_sound.cxx fg_sound.hxx \
	fg_fx.cxx fg_fx.hxx \
	morse.cxx morse.hxx \
	soundmgr.cxx soundmgr.hxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
