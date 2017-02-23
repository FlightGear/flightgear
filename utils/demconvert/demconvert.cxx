// demconvert.cxx -- convert dem into lower resolutions
//
// Copyright (C) 2016 Peter Sadrozinski
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>

#include <osg/ArgumentParser>

#include <simgear/misc/stdint.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>

#include <simgear/scene/dem/SGDem.hxx>
#include <simgear/scene/dem/SGDemSession.hxx>

int main(int argc, char** argv) 
{
    std::string demroot;
    std::string inputvfp;
    int tileWidth;
    int tileHeight;
    int resx, resy;
    int overlap = 0;
    SGDem dem;

    osg::ApplicationUsage* usage = new osg::ApplicationUsage();
    usage->setApplicationName("demconvert");
    usage->setCommandLineUsage(
        "Convert high resolution DEM to low res suitable for terrasync dl.");
    usage->addCommandLineOption("--inputvfp <dir>", "input VFP root directory");
    usage->addCommandLineOption("--demroot <dir>", "input/ouput DEM root directory");
    usage->addCommandLineOption("--width <N>", "width (in degrees) of created tiles");
    usage->addCommandLineOption("--height <N>", "height (in degrees) of created tiles");
    usage->addCommandLineOption("--resx <N>", "resolution of created tiles (w/o overlap)");
    usage->addCommandLineOption("--resy <N>", "resolution of created tiles (w/o overlap)");
    usage->addCommandLineOption("--overlap <N>", "number of pixels of overlap");

    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc, argv);
    arguments.setApplicationUsage(usage);

    sglog().setLogLevels( SG_TERRAIN, SG_INFO );

    arguments.read("--inputvfp", inputvfp);
    printf( "--inputvfp is %s\n", inputvfp.c_str() );
    
    arguments.read("--demroot", demroot);
    if ( inputvfp.empty() && demroot.empty() ) {
        arguments.reportError("--inputvfp or --demroot argument required.");
    } else if ( !demroot.empty() ) {
        SGPath s(demroot);
        if (!s.isDir()) {
            arguments.reportError(
                "--demroot directory does not exist or is not directory.");
        } else if (!s.canRead()) {
            arguments.reportError(
                "--demroot directory cannot be read. Check permissions.");
        } else if (!s.canWrite()) {
            arguments.reportError(
                "--demroot directory cannot be written. Check permissions.");
        } else if ( !dem.addRoot(s) ) {
            // see if we specified input as raw directory
            if ( inputvfp.empty() ) {
                arguments.reportError(
                    "--demroot directory is not a DEM heiarchy");
            } else {
                // create a new dem heiarchy
                printf("Creating new dem heiarchy at %s\n", s.c_str() );
                dem.createRoot(s);
            }
        }
    } else {
        SGPath s(inputvfp);
        if (!s.isDir()) {
            arguments.reportError(
                "--inputvfp directory does not exist or is not directory.");
        } else if (!s.canRead()) {
            arguments.reportError(
                "--inputvfp directory cannot be read. Check permissions.");
        }
    }

    if (!arguments.read("--width", tileWidth)) {
        arguments.reportError("--width argument required.");
    } else {
        if ( tileWidth < 1 || tileWidth > 60 ) 
            arguments.reportError(
                    "--width must be between 1 and 60");
    }

    if (!arguments.read("--height", tileHeight)) {
        arguments.reportError("--height argument required.");
    } else {
        if ( tileHeight < 1 || tileHeight > 60 ) 
            arguments.reportError(
                    "--height must be between 1 and 60");
    }

    if (!arguments.read("--resx", resx)) {
        arguments.reportError("--resx argument required.");
    } else {
        if ( resx < 2 ) 
            arguments.reportError(
                    "--resx must be between greater than 2");
    }

    if (!arguments.read("--resy", resy)) {
        arguments.reportError("--resy argument required.");
    } else {
        if ( resy < 2 ) 
            arguments.reportError(
                    "--resy must be between greater than 2");
    }

    if (arguments.read("--overlap", overlap)) {
        if ( overlap > (resx/2)  || overlap > (resy/2) )
            arguments.reportError(
                    "--overlap is greater than half tile");
    }

    if (arguments.errors()) {
        arguments.writeErrorMessages(std::cout);
        arguments.getApplicationUsage()->write(std::cout,
                osg::ApplicationUsage::COMMAND_LINE_OPTION | 
                osg::ApplicationUsage::ENVIRONMENTAL_VARIABLE, 80, true);
        return EXIT_FAILURE;
    }

    double lat_dec = (double)tileHeight / (double)resy;
    double lon_inc = (double)tileWidth  / (double)resx;

    SG_LOG( SG_TERRAIN, SG_INFO, "tileWidth: " << tileWidth);
    SG_LOG( SG_TERRAIN, SG_INFO, "tileHeight: " << tileHeight);
    SG_LOG( SG_TERRAIN, SG_INFO, "lon_inc: " << lon_inc);
    SG_LOG( SG_TERRAIN, SG_INFO, "lat_dec: " << lat_dec);

#define MIN_X   -180
#define MIN_Y   -90
#define MAX_X   180
#define MAX_Y   90

    // create a new dem level in demRoot
    SGDemRoot* root = dem.getRoot(0);
    if ( root ) {
        int outLvl = root->createLevel( tileWidth, tileHeight, resx, resy, overlap, ".tiff" );
        if ( outLvl >= 0 ) {
            printf("SGDem::createLevel success\n");

            // traverse the new tiles, 1 at a time
            for ( int tilex = MIN_X; tilex < MAX_X; tilex += tileWidth ) {
                for ( int tiley = MAX_Y; tiley > MIN_Y; tiley -= tileHeight ) {
                    // traverse rows from north to south, then columns west to east
                    double lonmin = (double)tilex;
                    double lonmax = lonmin + (double)tileWidth;
                    double latmax = (double)tiley;
                    double latmin = latmax - (double)tileHeight;

                    unsigned wo = SGDem::longitudeDegToOffset(lonmin);
                    unsigned eo = SGDem::longitudeDegToOffset(lonmax);
                    unsigned so = SGDem::latitudeDegToOffset(latmin);
                    unsigned no = SGDem::latitudeDegToOffset(latmax);

                    if ( !inputvfp.empty() ) {
                        // read from vfp files
                        printf("open session from raw directory\n");
                        SGDemSession s = dem.openSession( SGGeod::fromDeg(lonmin, latmin), SGGeod::fromDeg(lonmax, latmax), SGPath(inputvfp) );
                        printf("opened session from raw directory\n");

                        // create a new dem tile for the new level
                        SGDemTileRef tile = root->createTile( outLvl, (int)lonmin, (int)latmin, overlap, s );

                        s.close();
                    } else {
                        // read session from DEM root - don't cache - include adjacent tiles
                        fprintf( stderr, "open session from DEM level %d\n", outLvl-1);
                        SGDemSession s = dem.openSession( wo, so, eo, no, outLvl-1, false );
                        fprintf( stderr, "session has %d tiles\n", s.size() );

                        // create a new dem tile for the new level
                        SGDemTileRef tile = root->createTile( outLvl, (int)lonmin, (int)latmin, overlap, s );

                        s.close();
                    }
                }
            }

            printf("SGDem::close Level \n");

            root->closeLevel( outLvl );
        } else {
            printf("SGDem::createLevel failed\n");
        }
    } else {
        printf("SGDem::getRoot failed\n");
    }

    return EXIT_SUCCESS;
}
