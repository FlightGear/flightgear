noinst_LIBRARIES = libDEM.a

libDEM_a_SOURCES = dem.cxx dem.hxx

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib

# We can't build this with "-O2" (optimization) since this causes a seg fault
# I haven't found a way to strip this out of the CXXFLAGS, so I'm just
# setting it to "-g"
# CXXFLAGS = -g

