bin_PROGRAMS = construct master

construct_SOURCES = construct.cxx construct.hxx main.cxx construct_types.hxx

construct_LDADD = \
	$(top_builddir)/Tools/Construct/Array/libArray.a \
	$(top_builddir)/Tools/Construct/Clipper/libClipper.a \
	$(top_builddir)/Tools/Construct/GenOutput/libGenOutput.a \
	$(top_builddir)/Tools/Construct/Combine/libCombine.a \
	$(top_builddir)/Tools/Construct/Match/libMatch.a \
	$(top_builddir)/Tools/Construct/Triangulate/libTriangulate.a \
	$(top_builddir)/Tools/Lib/Polygon/libPolygon.a \
	$(top_builddir)/Tools/Lib/Triangle/libTriangle.a \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Math/libMath.a \
	$(top_builddir)/Lib/Misc/libMisc.a \
	$(top_builddir)/Lib/Debug/libDebug.a \
	$(top_builddir)/Lib/zlib/libz.a \
	-lgpc -lgfc

master_SOURCES = master.cxx

master_LDADD = \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Misc/libMisc.a

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Tools/Lib \
	-I$(top_builddir)/Tools/Construct
