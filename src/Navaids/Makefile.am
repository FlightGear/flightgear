noinst_LIBRARIES = libNavAids.a

noinst_PROGRAMS = testnavs

libNavAids_a_SOURCES = navaid.hxx navaids.hxx navaids.cxx

testnavs_SOURCES = testnavs.cxx
testnavs_LDADD = libNavAids.a -lsgdebug -lsgmisc -lz

INCLUDES += -I$(top_builddir) -I$(top_builddir)/src
