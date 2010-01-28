EXTRA_DIST = fgadmin.fl

bin_PROGRAMS = fgadmin

fgadmin_SOURCES = \
	fgadmin.cxx fgadmin.h \
	fgadmin_funcs.cxx \
	main.cxx \
	untarka.c untarka.h

fgadmin_LDADD = -lsgmisc -lsgdebug -lplibul -lz

fgadmin.cxx fgadmin.h: fgadmin.fl
	fluid -c fgadmin.fl
