noinst_LIBRARIES = libClipper.a

libClipper_a_SOURCES = clipper.cxx clipper.hxx

bin_PROGRAMS = testclipper

testclipper_SOURCES = testclipper.cxx

testclipper_LDADD = $(top_builddir)/Tools/Construct/Clipper/libClipper.a \
	$(top_builddir)/Tools/Lib/Polygon/libPolygon.a \
	$(top_builddir)/Lib/Debug/libDebug.a \
	$(top_builddir)/Lib/Misc/libMisc.a \
	$(top_builddir)/Lib/zlib/libz.a \
	-lgfc -lgpc

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Tools/Lib
