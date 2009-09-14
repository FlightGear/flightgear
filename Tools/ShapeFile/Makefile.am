bin_PROGRAMS = shape-decode

shape_decode_SOURCES = main.cxx shape.cxx shape.hxx

shape_decode_LDADD = \
	$(top_builddir)/Tools/Lib/Polygon/libPolygon.a \
	$(top_builddir)/Lib/Debug/libDebug.a \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Misc/libMisc.a \
	$(top_builddir)/Lib/zlib/libz.a \
	-lgfc -lgpc


INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Tools/Lib
