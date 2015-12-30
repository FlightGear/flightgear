
EXTRA_DIST = Makefile.mak

AM_CPPFLAGS = -I @top_srcdir@/include

lib_LIBRARIES = libHTSEngine.a

libHTSEngine_a_SOURCES = HTS_audio.c  HTS_engine.c  HTS_hidden.h  HTS_misc.c  \
                         HTS_pstream.c  HTS_sstream.c  HTS_model.c  HTS_vocoder.c \
                         HTS_gstream.c HTS_label.c

DISTCLEANFILES = *.log *.out *~

MAINTAINERCLEANFILES = Makefile.in
