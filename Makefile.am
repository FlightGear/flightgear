SUBDIRS = Simulator Tools

EXTRA_DIST = Makefile.extra README.Win32 README.autoconf README.running Thanks

dist-hook:
	tar cf - Hints | (cd $(distdir); tar xvf -)
