bin_PROGRAMS = example

example_SOURCES = example.cxx

example_LDADD = \
	$(top_builddir)/Lib/Audio/src/libsl.la \
	$(top_builddir)/Lib/Audio/src/libsm.la

INCLUDES += -I$(top_builddir)/Lib/Audio/src

if ENABLE_WIN32_AUDIO
LIBS += -lwinmm
endif
