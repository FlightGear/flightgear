if WITH_FGPANEL
AM_CXXFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"

bin_PROGRAMS = fgpanel

fgpanel_SOURCES = main.cxx \
		FGGLApplication.cxx FGGLApplication.hxx \
		FGPanelApplication.cxx FGPanelApplication.hxx \
                FGPNGTextureLoader.cxx FGPNGTextureLoader.hxx FGTextureLoaderInterface.hxx \
                FGRGBTextureLoader.cxx FGRGBTextureLoader.hxx \
		FGPanelProtocol.cxx \
		FGFontCache.cxx \
		panel.cxx panel.hxx \
		panel_io.cxx panel_io.hxx

#LIBS =

fgpanel_LDADD = \
	-lGLU -lglut -lsgmath -lsgprops -lsgio -lsgdebug -lsgmisc -lsgstructure -lsgxml -lsgtiming \
	-lplibpu -lplibfnt -lplibul \
	-lrt -lpng
endif
