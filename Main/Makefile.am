bin_PROGRAMS = construct

construct_SOURCES = construct.cxx

construct_LDADD = \
	$(top_builddir)/Tools/Construct/Array/libArray.a \
	$(top_builddir)/Tools/Construct/Clipper/libClipper.a \
	$(top_builddir)/Tools/Construct/Triangulate/libTriangulate.a \
	$(top_builddir)/Tools/Lib/Polygon/libPolygon.a \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Math/libMath.a \
	$(top_builddir)/Lib/Misc/libMisc.a \
	$(top_builddir)/Lib/Debug/libDebug.a \
	$(top_builddir)/Lib/zlib/libz.a \
	-lgpc -lgfc

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Tools/Lib \
	-I$(top_builddir)/Tools/Construct
