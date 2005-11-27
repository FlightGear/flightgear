noinst_LIBRARIES = libNavaids.a

# noinst_PROGRAMS = testnavs

libNavaids_a_SOURCES = \
	navdb.hxx navdb.cxx \
	fix.hxx fixlist.hxx fixlist.cxx \
	navrecord.hxx navlist.hxx navlist.cxx

#	ils.hxx ilslist.hxx ilslist.cxx \
#	mkrbeacons.hxx mkrbeacons.cxx 
#
# testnavs_SOURCES = testnavs.cxx
# testnavs_LDADD = \
# 	libNavaids.a \
# 	-lsgtiming -lsgmath -lsgmisc -lsgdebug -lsgmagvar -lsgxml \
# 	-lsgstructure $(base_LIBS) -lz

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
