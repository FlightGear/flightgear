SUBDIRS = Include Lib Simulator Tools Tests

EXTRA_DIST = Makefile.extra README.Unix README.Win32 README.autoconf \
	README.running Thanks acsite.m4

dist-hook:
	tar cf - Hints | (cd $(distdir); tar xvf -)
