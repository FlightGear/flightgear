noinst_LIBRARIES = libNavaids.a

noinst_PROGRAMS = testnavs

libNavaids_a_SOURCES = \
	fix.hxx fixlist.hxx fixlist.cxx \
	ils.hxx ilslist.hxx ilslist.cxx \
	mkrbeacons.hxx mkrbeacons.cxx \
	nav.hxx navlist.hxx navlist.cxx

testnavs_SOURCES = testnavs.cxx
testnavs_LDADD = \
	libNavaids.a \
	-lsgtiming -lsgmath -lsgmisc -lsgdebug -lsgmagvar -lsgxml \
	-lz

INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
