noinst_LIBRARIES = libModel.a

libModel_a_SOURCES = model.cxx model.hxx \
                     acmodel.cxx acmodel.hxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif
