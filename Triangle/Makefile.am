# DEFS is a list of definitions used to compile an object code version
#   of Triangle (triangle.o) to be called by another program.  The file
#   "triangle.h" contains detailed information on how to call triangle.o.
#
# The -DTRILIBRARY should always be used when compiling Triangle into an
#   object file.
#
# An example DEFS line is:
#
#   DEFS = -DTRILIBRARY -DREDUCED -DCDT_ONLY

DEFS += 

bin_PROGRAMS = triangle # showme

triangle_SOURCES = triangle.c triangle.h

triangle_LDADD = $(base_LIBS)

# showme_SOURCES = showme.c
