noinst_LIBRARIES = libObjects.a

libObjects_a_SOURCES = \
	apt_signs.cxx apt_signs.hxx \
	dir_lights.cxx dir_lights.hxx \
	newmat.cxx newmat.hxx \
	matlib.cxx matlib.hxx \
	obj.cxx obj.hxx \
	texload.c texload.h colours.h

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
