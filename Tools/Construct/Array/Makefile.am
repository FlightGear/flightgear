noinst_LIBRARIES = libArray.a

libArray_a_SOURCES = array.cxx array.hxx

bin_PROGRAMS = testarray

testarray_SOURCES = testarray.cxx

testarray_LDADD = \
	$(top_builddir)/Tools/Construct/Array/libArray.a \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Math/libMath.a \
	$(top_builddir)/Lib/Misc/libMisc.a \
	$(top_builddir)/Lib/zlib/libz.a

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Tools/Construct

# We can't build this with "-O2" (optimization) since this causes a seg fault
# I haven't found a way to strip this out of the CXXFLAGS, so I'm just
# setting it to "-g"
# CXXFLAGS = -g

