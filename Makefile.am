SUBDIRS = \
	tests \
	man \
        scripts \
	src \
        utils

EXTRA_DIST = \
	acinclude.m4 \
	autogen.sh \
	projects \
	README \
	README.OpenAL \
	README.plib \
	README.OSG \
	README.SimGear \
	Thanks

dist-hook:
	(cd $(top_srcdir); tar --exclude docs-mini/CVS --exclude hints/CVS \
		-cf - docs-mini ) | (cd $(distdir); tar xvf -)
	rm -rf `find $(distdir)/projects -name CVS`


##########################################################################
# extra targets to do various things ...
#
# Use "make <target>"
##########################################################################


# make the base distribution with textures, sounds and a bit of
# scenery, and all the other associated files

# note, fedora core tar does case insensitive wildcard matching for
# directories, but apparently not files ... some stupid reason. :-(

data-tar:
	(cd ..; \
	tar \
	--exclude=CVS \
	--exclude='*~' \
	--exclude='*.tex' \
	--exclude='*.xcf' \
	--exclude='*/c172/Instruments.high' \
	--exclude='*/Textures/Unused' \
	--exclude='*/Textures/*.orig' \
	--exclude='*/Textures.high/*.new' \
	--exclude='*/Textures.high/*.orig' \
	--exclude='*/Textures.high/*.save' \
	--exclude='*/data/Data' \
	--exclude='*/Docs/source' \
        --exclude='*/Models/MNUAV' \
        --exclude='*/Models/Airspace' \
	-cjvf FlightGear-data-$(VERSION).tar.bz2 \
		data/AI \
                data/Aircraft/Generic \
                data/Aircraft/Instruments \
                data/Aircraft/Instruments-3d \
                data/Aircraft/UIUC \
		data/Aircraft/777-200 \
		data/Aircraft/A6M2 \
		data/Aircraft/b1900d \
                data/Aircraft/bo105 \
                data/Aircraft/c172p \
                data/Aircraft/CitationX \
		data/Aircraft/Dragonfly \
		data/Aircraft/dhc2 \
		data/Aircraft/f-14b \
		data/Aircraft/Cub \
		data/Aircraft/SenecaII \
		data/Aircraft/sopwithCamel \
                data/Aircraft/ufo \
		data/Aircraft/ZLT-NT \
		data/Airports \
		data/Astro \
		data/ATC \
		data/AUTHORS \
		data/ChangeLog \
		data/cloudlayers.xml \
		data/COPYING \
		data/D* \
		data/Effects \
		data/Environment \
		data/Fonts \
		data/gui \
		data/Huds \
		data/Input \
		data/joysticks.xml \
		data/keyboard.xml \
		data/Lighting \
		data/materials.dtd \
		data/materials.xml \
		data/mice.xml \
		data/Models \
		data/N* \
		data/options.xml \
		data/preferences.xml \
		data/Protocol \
		data/README \
		data/Scenery/Airports \
		data/Scenery/Objects \
		data/Scenery/Terrain \
		data/Shaders \
		data/Sounds \
		data/T* \
		data/version \
	)

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
# 	( cd $(HOME)/Projects ; \
# 	zip -r fgfs-docs-$(VERSION).zip FlightGear-0.7/docs )

fgfs-manual-zip:
	( cd $(HOME) ; \
	zip -r fgfs-manual-$(VERSION).zip FlightGear/Docs/InstallGuide )

# make the win32-bin distribution

fgfs-win32-bin:
	(cd $(HOME); \
	zip -r fgfs-win32-bin-$(VERSION).zip FlightGear/runfgfs.bat \
        FlightGear/bin/Win32 )

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
