noinst_LIBRARIES = libBucket.a

libBucket_a_SOURCES = newbucket.cxx newbucket.hxx

bin_PROGRAMS = testbucket

testbucket_SOURCES = testbucket.cxx

testbucket_LDADD = $(top_builddir)/Lib/Bucket/libBucket.a

INCLUDES += -I$(top_builddir)
