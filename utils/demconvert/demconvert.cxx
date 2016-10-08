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
#include <simgear/scene/tgdb/dem.hxx>

std::string getTileName( int lon, int lat )
{
    std::stringstream ss;

    if ( lat >= 0 ) {
        ss << "N" << std::setw(2) << std::setfill('0') << lat;
    } else {
        ss << "S" << std::setw(2) << std::setfill('0') << -lat;
    }
    if ( lon >= 0 ) {
        ss << "E" << std::setw(3) << std::setfill('0') << lon;
    } else {
        ss << "W" << std::setw(3) << std::setfill('0') << -lon;
    }
    ss << ".hgt";
    
    return ss.str();
}

unsigned short getAlt( std::string path, double lon, double lat )
{
    double intPart;
    double fractLat = modf ( lat, &intPart);
    double fractLon = modf ( lon, &intPart);
    
    int l = (int)( (double)1200 - (1200*fractLat) );
    int p = (int)( (double)1200 * fractLon );
    
    int intLon = (int)lon;
    int intLat = (int)lat;
    
    unsigned short data;
    SGPath tileIn(path);
    tileIn.append(getTileName(intLon, intLat));
    std::ifstream demFile(tileIn.c_str());
    
    if (demFile.is_open()) {
        int fileOffset = (1201*l + p) * 2;
        demFile.seekg (fileOffset, demFile.beg);
        demFile.read ((char *)&data,2);
        demFile.close();
    
        if ( sgIsLittleEndian() ) {
            sgEndianSwap(&data);
        }

        if ( data > 10000 ) {
            SG_LOG(SG_GENERAL, SG_INFO, "getAlt for (" << lon << "," << lat << ") filename is " << path << " l is " << l << ", p is " << p << " alt is " << data );
            SG_LOG(SG_GENERAL, SG_INFO, "               fractLon is " << fractLon << " fractLat is " << fractLat << " fileOffset is " << fileOffset  );
            return 0;
        }

        return (unsigned short)data;
    } else {
        return 0;            
    }
}    


int main(int argc, char** argv) 
{
    std::string input;
    int tileWidth;
    int tileHeight;
    int resx, resy;
    SGDem dem;

    osg::ApplicationUsage* usage = new osg::ApplicationUsage();
    usage->setApplicationName("demconvert");
    usage->setCommandLineUsage(
        "Convert high resolution DEM to low res suitable for terrasync dl.");
    usage->addCommandLineOption("--input <dir>", "DEM root directory");
    usage->addCommandLineOption("--width <N>", "width (in degrees) of created tiles");
    usage->addCommandLineOption("--height <N>", "height (in degrees) of created tiles");
    usage->addCommandLineOption("--resx <N>", "resolution of created tiles");
    usage->addCommandLineOption("--resy <N>", "resolution of created tiles");

    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc, argv);
    arguments.setApplicationUsage(usage);

    sglog().setLogLevels( SG_TERRAIN, SG_INFO );
    
    if (!arguments.read("--input", input)) {
        arguments.reportError("--input argument required.");
    } else {
        SGPath s(input);
        if (!s.isDir()) {
            arguments.reportError(
                "--input directory does not exist or is not directory.");
        } else if (!s.canRead()) {
            arguments.reportError(
                "--input directory cannot be read. Check permissions.");
        } else if ( !dem.init(s) ) {
            arguments.reportError(
                "--input directory is not a DEM heiarchy");            
        }
    }
    
    if (!arguments.read("--width", tileWidth)) {
        arguments.reportError("--width argument required.");
    } else {
        if ( tileWidth < 2 || tileWidth > 60 ) 
            arguments.reportError(
                    "--width must be between 2 and 60");
    }

    if (!arguments.read("--height", tileHeight)) {
        arguments.reportError("--height argument required.");
    } else {
        if ( tileHeight < 2 || tileHeight > 60 ) 
            arguments.reportError(
                    "--height must be between 2 and 60");
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

    // create a new dem level
    int outLvl = dem.createLevel( tileWidth, tileHeight, resx, resy, ".tiff" );
    if ( outLvl >= 0 ) {
        // traverse the new tiles, 1 at a time
        for ( int tilex = MIN_X; tilex <= MAX_X; tilex += tileWidth ) {
            for ( int tiley = MAX_Y; tiley >= MIN_Y; tiley -= tileHeight ) {            
                // traverse rows from north to south, then columns west to east
                double lonmin = (double)tilex;
                double lonmax = lonmin + (double)tileWidth;
                double latmax = (double)tiley;
                double latmin = latmax - (double)tileHeight;

                // create a dem cache session for this tile - don't cache
                SGDemSession s = dem.openSession( SGGeod::fromDeg(lonmin, latmin), SGGeod::fromDeg(lonmax, latmax), false );

                // create a new dem tile for the new level
                SGDemTileRef tile = dem.createTile( outLvl, (int)lonmin, (int)latmin, s );

                s.close();
            }
        }
    } else {
        printf("SGDem::createLevel failed\n");
    }

    return EXIT_SUCCESS;
}
