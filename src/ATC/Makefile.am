noinst_LIBRARIES = libATC.a

libATC_a_SOURCES = \
	atis.cxx atis.hxx atislist.hxx atislist.cxx \
	ATCdisplay.hxx ATCdisplay.cxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
