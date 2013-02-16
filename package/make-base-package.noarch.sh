#!/bin/sh

echo $1 $2

if [ "x$1" != "x" ]; then
    BASE="$1"
else
    BASE="/home/curt/Projects/FlightGear/"
fi

if [ "x$2" != "x" ]; then
    VERSION="$2"
else
    VERSION="2.7"
fi

echo base dir = $BASE, version = $VERSION

cd $BASE

tar \
	--exclude=CVS \
	--exclude='*~' \
	--exclude='*.bak' \
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
	-cjvf FlightGear-data-${VERSION}.tar.bz2 \
		data/AI \
		data/Aircraft/Generic \
		data/Aircraft/Instruments \
		data/Aircraft/Instruments-3d \
		data/Aircraft/UIUC \
		data/Aircraft/777 \
		data/Aircraft/777-200 \
		data/Aircraft/A6M2 \
		data/Aircraft/ASK13 \
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
		data/COPYING \
		data/D* \
		data/Effects \
		data/Environment \
		data/Fonts \
		data/gui \
		data/HLA \
		data/Huds \
		data/Input \
		data/joysticks.xml \
		data/keyboard.xml \
		data/Lighting \
		data/mice.xml \
		data/Materials \
		data/Models \
		data/MP \
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
		data/version
