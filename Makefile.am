SUBDIRS = \
	tests \
	src

EXTRA_DIST = Thanks acsite.m4 acconfig.h irix-hack.sh \
	FlightGear.dsp FlightGear.dsw

dist-hook:
	(cd $(top_srcdir); $(HOME)/projects/FlightGear-0.7/admin/am2dsp.pl)
	(cd $(top_srcdir); tar --exclude docs-mini/CVS --exclude hints/CVS \
		-cf - docs-mini hints) | (cd $(distdir); tar xvf -)
	# tar cf - Simulator/FDM/JSBsim/aircraft Simulator/FDM/JSBsim/engine | (cd $(distdir); tar xvf -)


##########################################################################
# extra targets to do various things ...
#
# Use "make <target>"
##########################################################################


# make the base distribution with textures, sounds and a bit of
# scenery, and all the other associated files

fgfs-base: fgfs-base-tar fgfs-base-zip fgfs-base-patch fgfs-textures-high

fgfs-base-tar:
	(cd $(HOME); \
	tar --exclude=CVS \
	--exclude='*/c172/Instruments.high' \
	--exclude='*/Models/Aircraft/*' \
	--exclude='*/Textures/Unused' \
	--exclude='*/Textures.high/*.save' \
	-czvf fgfs-base-$(VERSION).tar.gz \
	FlightGear/A[A-su-z]* \
	FlightGear/[B-CE-R]* \
	FlightGear/Scenery/w130n30/w123n37 \
	FlightGear/Scenery/w130n30/w122n37 \
	FlightGear/Sounds \
	FlightGear/[T-Za]* FlightGear/[c-z]*)

fgfs-base-zip:
	(cd $(HOME); \
	zip -rv fgfs-base-$(VERSION).zip \
	FlightGear/A[A-su-z]* \
	FlightGear/[B-CE-R]* \
	FlightGear/Scenery/w130n30/w123n37 \
	FlightGear/Scenery/w130n30/w122n37 \
	FlightGear/Sounds \
	FlightGear/[T-Za]* FlightGear/[c-z]* \
	-x '*/CVS/*' -x '*/Textures/Unused/*' -x '*/Models/Aircraft/*' )

fgfs-base-patch:
	(cd $(HOME); \
	tar --newer 6/18/2001 -czvf fgfs-base-patch-$(VERSION)c.tar.gz \
	FlightGear/A[A-su-z]* \
	FlightGear/[B-CE-R]* \
	FlightGear/Scenery/w130n30/w123n37 \
	FlightGear/Scenery/w130n30/w122n37 \
	FlightGear/Sounds \
	FlightGear/[T-Za]* FlightGear/[c-z]*)

fgfs-textures-high:
	(cd $(HOME); \
	tar -czvf fgfs-textures-high-$(VERSION)b.tar.gz \
	FlightGear/Textures.high)

# make the mini JSBsim data distribution
jsbsim-data:
	(cd $(HOME); \
	tar czvf jsbsim-data-$(VERSION).tar.gz \
	FlightGear/Aircraft \
	FlightGear/Engine)

# make the documentation distribution

fgfs-docs: fgfs-docs-tar fgfs-manual-zip

fgfs-docs-tar:
	( cd $(HOME); \
	tar czvf fgfs-docs-$(VERSION).tar.gz FlightGear/Docs )

# fgfs-docs-zip:
# 	( cd $(HOME)/projects ; \
# 	zip -r fgfs-docs-$(VERSION).zip FlightGear-0.7/docs )

fgfs-manual-zip:
	( cd $(HOME) ; \
	zip -r fgfs-manual-$(VERSION).zip FlightGear/Docs/InstallGuide )

# make the win32-bin distribution

fgfs-win32-bin:
	(cd $(HOME); \
	zip -r fgfs-win32-bin-$(VERSION).zip FlightGear/bin )

# make the win32-libs distribution (depricated)
win32-libs:
	( zip -r ../win32-libs-$(VERSION).zip Win32/Makefile \
	Win32/README.glut Win32/cygwinb1.dll Win32/def \
	Win32/*.exp Win32/gen-def-from-errors.pl \
	Win32/glut.dll Win32/glut32.dll Win32/impdef.cpp \
	Win32/impdef.exe Win32/include \
	Win32/install.exe \
	Win32/lib Win32/zlib.lib )

# make the MacOS (MWCWP3) devel support archive
macos-devel:
	( cd ..; zip -r MWCWP3_Support.zip FlightGear/MWCWP3_Support )

# clean up dependencies
clean-deps:
	rm -rf .deps \
	src/.deps tests/.deps \
	src/*/.deps \
	src/*/*/.deps \
	tests/.deps

# clean up after libtool
clean-libs:
	rm -rf .libs src/.libs \
		src/*/.libs \
		src/*/*/.libs 
	rm -f src/*/so_locations \
		src/*/*/so_locations 

# clean up after winbloze spews random case for file names :-(
clean-winbloze: clean-deps clean-libs
	rm -f makefile src/Include/makefile src/makefile \
		src/*/makefile \
		src/*/*/makefile \
		tests/makefile
	rm -f src/Main/fg.exe
