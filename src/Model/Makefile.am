noinst_LIBRARIES = libModel.a

libModel_a_SOURCES = loader.cxx loader.hxx \
	             model.cxx model.hxx \
                     modelmgr.cxx modelmgr.hxx \
                     acmodel.cxx acmodel.hxx \
                     panelnode.cxx panelnode.hxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
