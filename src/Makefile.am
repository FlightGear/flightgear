libdir	= ${exec_prefix}/lib

lib_LTLIBRARIES = libsl.la libsm.la

libsl_la_SOURCES = \
	sl.h slPortability.h \
	slDSP.cxx slSample.cxx slEnvelope.cxx \
	slSamplePlayer.cxx slScheduler.cxx

libsm_la_SOURCES = sm.h slPortability.h smMixer.cxx

INCLUDES += -I$(top_builddir) -I.
