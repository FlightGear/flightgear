noinst_LIBRARIES = libClipper.a
bin_PROGRAMS = clipper

libClipper_a_SOURCES = clipper.cxx clipper.hxx
clipper_SOURCES = main.cxx

clipper_LDADD = $(top_builddir)/Tools/Clipper/libClipper.a \
	$(top_builddir)/Tools/Lib/Polygon/libPolygon.a \
	$(top_builddir)/Lib/Debug/libDebug.a \
	$(top_builddir)/Lib/Misc/libMisc.a \
	$(top_builddir)/Lib/zlib/libz.a \
	-lgfc -lgpc

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Tools/Lib
