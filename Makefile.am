SUBDIRS = Include Lib Simulator Tools Tests

EXTRA_DIST = README.Unix README.Linux README.Win32 \
	README.autoconf README.running Thanks VERSION acsite.m4

dist-hook:
	tar cf - Hints | (cd $(distdir); tar xvf -)


##########################################################################
# extra targets to do various things ...
#
# Use "make <target>"
##########################################################################


# make the base distribution with textures, sounds and a bit of
# scenery, and all the other associated files

fgfs-base: fgfs-base-tar fgfs-base-zip

fgfs-base-tar:
	(cd $(HOME); \
	tar czvf fgfs-base-$(VERSION).tar.gz \
	FlightGear/[A-R]* \
	FlightGear/Scenery/w120n030/w111n033 \
	FlightGear/Sounds \
	FlightGear/Textures FlightGear/Thanks \
	FlightGear/[m-r]* )

fgfs-base-zip:
	(cd $(HOME); \
	zip -r fgfs-base-$(VERSION).zip \
	FlightGear/[A-R]* \
	FlightGear/Scenery/w120n030/w111n033 \
	FlightGear/Sounds \
	FlightGear/Textures FlightGear/Thanks \
	FlightGear/[m-r]* )


# make the documentation distribution

fgfs-docs: fgfs-docs-tar fgfs-docs-zip

fgfs-docs-tar:
	( cd .. ; \
	tar czvf fgfs-docs-$(VERSION).tar.gz FlightGear/Docs )

fgfs-docs-zip:
	( cd .. ; \
	zip -r fgfs-docs-$(VERSION).zip FlightGear/Docs )


# make the win32-bin distribution

fgfs-win32-bin:
	(cd $(HOME); \
	zip -r fgfs-win32-bin-$(VERSION).zip FlightGear/bin )

# make the win32-libs distribution
win32-libs:
	( zip -r ../win32-libs-$(VERSION).zip Win32/Makefile \
	Win32/README.glut Win32/cygwinb1.dll Win32/def \
	Win32/*.exp Win32/gen-def-from-errors.pl \
	Win32/glut.dll Win32/glut32.dll Win32/impdef.cpp \
	Win32/impdef.exe Win32/include \
	Win32/install.exe \
	Win32/lib Win32/zlib.lib )

# clean up dependencies
clean-deps:
	rm -rf .deps \
	Lib/.deps Simulator/.deps Tools/.deps Test/.deps \
	Lib/*/.deps Simulator/*/.deps Tools/*/.deps \
	Lib/*/*/.deps Simulator/*/*/.deps Tools/*/*/.deps \
	Tests/.deps

# clean up after libtool
clean-libs:
	rm -rf .libs Lib/.libs Simulator/.libs Tools/.libs \
		Lib/*/.libs Simulator/*/.libs Tools/*/.libs \
		Lib/*/*/.libs Simulator/*/*/.libs Tools/*/*/.libs
	rm -f Simulator/*/so_locations Lib/*/so_locations \
		Simulator/*/*/so_locations Lib/*/*/so_locations

# clean up after winbloze spews random case for file names :-(
clean-winbloze: clean-deps clean-libs
	rm -f makefile Include/makefile Lib/makefile Simulator/makefile \
		Tools/makefile \
		Lib/*/makefile Simulator/*/makefile Tools/*/makefile \
		Lib/*/*/makefile Simulator/*/*/makefile Tools/*/*/makefile \
		Tests/makefile
	rm -f Src/Main/fg.exe Tools/*/*.exe
