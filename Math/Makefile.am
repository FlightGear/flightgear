libdir	= ${exec_prefix}/lib

lib_LTLIBRARIES = libMath.la
libMath_la_SOURCES = \
	MAT3geom.c \
	MAT3inv.c \
	MAT3mat.c \
	MAT3vec.c \
	fg_geodesy.c fg_geodesy.h \
	fg_random.c fg_random.h \
	interpolater.cxx interpolater.hxx \
	mat3.h mat3defs.h mat3err.h \
	polar3d.cxx polar3d.hxx \
	vector.cxx vector.hxx

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Simulator
