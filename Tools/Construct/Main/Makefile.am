bin_PROGRAMS = fgfs-construct fgfs-master

fgfs_construct_SOURCES = \
	construct_types.hxx \
	construct.cxx construct.hxx \
	main.cxx

fgfs_construct_LDADD = \
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

fgfs_master_SOURCES = master.cxx

fgfs_master_LDADD = \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Misc/libMisc.a

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Tools/Lib \
	-I$(top_builddir)/Tools/Construct
