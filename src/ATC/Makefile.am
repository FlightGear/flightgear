noinst_LIBRARIES = libATC.a

libATC_a_SOURCES = \
	ATC.hxx ATC.cxx \
	atis.hxx atis.cxx atislist.hxx atislist.cxx \
	tower.hxx tower.cxx towerlist.hxx towerlist.cxx \
	approach.hxx approach.cxx approachlist.hxx approachlist.cxx \
	ATCdisplay.hxx ATCdisplay.cxx \
	ATCmgr.hxx ATCmgr.cxx \
	ATCutils.hxx ATCutils.cxx \
	AIMgr.hxx AIMgr.cxx \
	AIEntity.hxx AIEntity.cxx \
	AILocalTraffic.hxx AILocalTraffic.cxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
