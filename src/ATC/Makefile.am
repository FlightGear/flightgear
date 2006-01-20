noinst_LIBRARIES = libATC.a

if USE_FESTIVAL
VOICE_SRC = voice.cxx voice.hxx
else
VOICE_SRC =
endif

libATC_a_SOURCES = \
	ATC.hxx ATC.cxx \
	atis.hxx atis.cxx \
	tower.hxx tower.cxx \
	approach.hxx approach.cxx \
	ground.hxx ground.cxx \
	commlist.hxx commlist.cxx \
	ATCDialog.hxx ATCDialog.cxx \
	ATCdisplay.hxx ATCdisplay.cxx \
	ATCVoice.hxx ATCVoice.cxx \
	ATCmgr.hxx ATCmgr.cxx \
	ATCutils.hxx ATCutils.cxx \
	ATCProjection.hxx ATCProjection.cxx \
	AIMgr.hxx AIMgr.cxx \
	AIEntity.hxx AIEntity.cxx \
	AIPlane.hxx AIPlane.cxx \
	AILocalTraffic.hxx AILocalTraffic.cxx \
	AIGAVFRTraffic.hxx AIGAVFRTraffic.cxx \
	transmission.hxx transmission.cxx \
	transmissionlist.hxx transmissionlist.cxx \
	$(VOICE_SRC)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
