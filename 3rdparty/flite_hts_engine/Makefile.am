
EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL NEWS README Makefile.mak \
             flite/COPYING \
             flite/lang/cmulex/cmu_lex_data_raw.c \
             flite/lang/cmulex/cmu_lex_num_bytes.c \
             flite/lang/cmulex/cmu_lex_phones_huff_table.c \
             flite/lang/cmulex/cmu_lex_entries_huff_table.c

SUBDIRS = lib bin

include_HEADERS = $(top_srcdir)/include/flite_hts_engine.h

DISTCLEANFILES = *.log *.out *~ config/*~ include/*~ flite/*~ flite/*/*~ flite/*/*/*~

MAINTAINERCLEANFILES = aclocal.m4 configure Makefile.in config/compile \
                       config/config.guess config/config.sub config/depcomp \
                       config/install-sh config/missing
