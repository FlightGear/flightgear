libdir  = ${exec_prefix}/lib

lib_LTLIBRARIES = libDEM.la

libDEM_la_SOURCES = dem.cxx dem.hxx leastsqs.cxx leastsqs.hxx

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib

# We can't build this with "-O2" (optimization) since this causes a seg fault
# I haven't found a way to strip this out of the CXXFLAGS, so I'm just
# setting it to "-g"
CXXFLAGS = -g

