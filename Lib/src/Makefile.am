noinst_LIBRARIES = libsl.a libsm.a

libsl_a_SOURCES = \
	sl.h slPortability.h \
	slDSP.cxx slSample.cxx slEnvelope.cxx \
	slSamplePlayer.cxx slScheduler.cxx

libsm_a_SOURCES = sm.h slPortability.h smMixer.cxx

INCLUDES += -I$(top_builddir) -I.
