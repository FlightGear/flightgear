
AM_CXXFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"

bin_PROGRAMS = fgviewer

fgviewer_SOURCES = fgviewer.cxx
fgviewer_LDADD = \
	-lsgroute -lsgsky -lsgsound -lsgephem -lsgtgdb -lsgmaterial -lsgmodel \
	-lsgbvh -lsgutil -lsgtiming -lsgio -lsgscreen -lsgmath -lsgbucket \
	-lsgprops -lsgdebug -lsgmagvar -lsgmisc -lsgnasal -lsgxml -lsgsound \
	-lsgserial -lsgstructure -lsgenvironment \
	-lplibul \
	$(OSG_LIBS) \
	$(THREAD_LIBS) \
	$(network_LIBS) \
	-lz \
	$(opengl_LIBS) \
	$(openal_LIBS)
