noinst_LIBRARIES = libATC.a

libATC_a_SOURCES = \
	ATC.hxx ATC.cxx \
	ATCdisplay.hxx ATCdisplay.cxx \
	ATCmgr.hxx ATCmgr.cxx \
	atis.cxx atis.hxx \
	atislist.hxx atislist.cxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
