bin_PROGRAMS = example

example_SOURCES = example.cxx

example_LDADD = \
	$(top_builddir)/Lib/Audio/src/libsl.a \
	$(top_builddir)/Lib/Audio/src/libsm.a

INCLUDES += -I$(top_builddir)/Lib/Audio/src

if ENABLE_IRIX_AUDIO
LIBS += -laudio
endif

if ENABLE_WIN32_AUDIO
LIBS += -lwinmm
endif
