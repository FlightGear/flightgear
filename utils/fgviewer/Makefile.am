
AM_CXXFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"

bin_PROGRAMS = fgviewer

fgviewer_SOURCES = fgviewer.cxx
fgviewer_LDADD = \
	-lsgtgdb -lsgmaterial -lsgmodel -lsgbvh -lsgutil -lsgio -lsgbucket \
	-lsgmath -lsgprops -lsgdebug -lsgmisc -lsgxml -lsgstructure \
	-lplibul $(OSG_LIBS) $(opengl_LIBS) $(THREAD_LIBS) -lz
