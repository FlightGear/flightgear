noinst_HEADERS = xml_cache.h xml.h

noinst_LIBRARIES = libxmlconf.a

libxmlconf_a_SOURCES = xml_cache.c xml.c

noinst_PROGRAMS = \
	testxml \
	printxml \
	xmlgrep \
	printtree \
	generic_config_analyse

testxml_SOURCES = testxml.c
testxml_LDADD = libxmlconf.a

printxml_SOURCES = printxml.c
printxml_LDADD = libxmlconf.a

printtree_SOURCES = printtree.c
printtree_LDADD = libxmlconf.a

xmlgrep_SOURCES = xmlgrep.c
xmlgrep_LDADD = libxmlconf.a

generic_config_analyse_SOURCES = generic-config-analyse.c
generic_config_analyse_LDADD = libxmlconf.a

INCLUDES = -DSRC_DIR=\"$(top_srcdir)/utils/xmlgrep\" -DXML_USE_NODECACHE
