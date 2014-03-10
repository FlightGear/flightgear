
EXTRA_DIST = Makefile.mak

INCLUDES = -I$(top_srcdir)/include \
           -I$(top_srcdir)/flite/include \
           -I$(top_srcdir)/flite/lang/cmu_us_kal \
           -I$(top_srcdir)/flite/lang/cmulex \
           -I$(top_srcdir)/flite/lang/usenglish \
           -I$(HTS_ENGINE_HEADER_DIR)

bin_PROGRAMS = flite_hts_engine

flite_hts_engine_SOURCES = flite_hts_engine.c 

flite_hts_engine_LDADD = $(top_srcdir)/lib/libflhtse.a $(HTS_ENGINE_LIBRARY)

DISTCLEANFILES = *.log *.out *~
