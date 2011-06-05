noinst_LIBRARIES = libNavaids.a

# noinst_PROGRAMS = testnavs

libNavaids_a_SOURCES = \
	navdb.hxx navdb.cxx \
	fix.hxx fixlist.hxx fixlist.cxx \
	navrecord.hxx navrecord.cxx \
	navlist.hxx navlist.cxx \
	positioned.hxx positioned.cxx \
	markerbeacon.hxx markerbeacon.cxx \
	routePath.hxx routePath.cxx \
	airways.hxx airways.cxx \
	route.hxx route.cxx \
	waypoint.hxx waypoint.cxx \
	procedure.hxx procedure.cxx \
	PositionedBinding.hxx PositionedBinding.cxx

#
# testnavs_SOURCES = testnavs.cxx
# testnavs_LDADD = \
# 	libNavaids.a \
# 	-lsgtiming -lsgmath -lsgmisc -lsgdebug -lsgmagvar -lsgxml \
# 	-lsgstructure $(base_LIBS) -lz

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
