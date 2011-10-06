
AM_CXXFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"

bin_PROGRAMS = fgviewer

if HAVE_FRAMEWORK_PLIB
fgviewer_LDFLAGS = $(plib_FRAMEWORK)
else
fgviewer_PLIB_LIBS = -lplibul
fgviewer_LDFLAGS =
endif

if HAVE_FRAMEWORK_OSG
fgviewer_LDFLAGS += $(osg_FRAMEWORKS) $(openthreads_FRAMEWORK)
else
fgviewer_LDFLAGS += $(osg_LIBS) $(openthreads_LIBS)
endif

fgviewer_SOURCES = fgviewer.cxx
fgviewer_LDADD = \
	-lsgtgdb -lsgmaterial -lsgmodel -lsgbvh -lsgutil -lsgio -lsgbucket \
	-lsgmath -lsgprops -lsgdebug -lsgmisc -lsgxml -lsgstructure -lsgthreads \
	$(fgviewer_PLIB_LIBS) $(OSG_LIBS) $(opengl_LIBS) $(thread_LIBS) -lz
