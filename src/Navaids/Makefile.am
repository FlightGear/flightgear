noinst_LIBRARIES = libNavAids.a

noinst_PROGRAMS = testnavs

libNavAids_a_SOURCES = nav.hxx navlist.hxx navlist.cxx

testnavs_SOURCES = testnavs.cxx
testnavs_LDADD = libNavAids.a -lsgdebug -lsgmath -lsgmisc -lz

INCLUDES += -I$(top_builddir) -I$(top_builddir)/src
