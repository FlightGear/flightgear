/**
 * SHPParser - parse ESRI ShapeFiles containing PolyLines */

// Written by James Turner, started 2013.
//
// Copyright (C) 2013 James Turner <zakalawe@mac.com>
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
     #include "config.h"
#endif

#include "SHPParser.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/io/lowlevel.hxx>

// http://www.esri.com/library/whitepapers/pdfs/shapefile.pdf table 1
const int SHP_FILE_MAGIC = 9994;
const int SHP_FILE_VERSION = 1000;

const int SHP_NULL_TYPE = 0;
const int SHP_POLYLINE_TYPE = 3;
const int SHP_POLYGON_TYPE = 5;

namespace
{

void sgReadIntBE ( gzFile fd, int& var )
{
    if ( gzread ( fd, &var, sizeof(int) ) != sizeof(int) ) {
        throw sg_io_exception("gzread failed");
    }

    if ( sgIsLittleEndian() ) {
        sgEndianSwap( (uint32_t *) &var);
    }
}

void sgReadIntLE ( gzFile fd, int& var )
{
    if ( gzread ( fd, &var, sizeof(int) ) != sizeof(int) ) {
        throw sg_io_exception("gzread failed");
    }

    if ( sgIsBigEndian() ) {
        sgEndianSwap( (uint32_t *) &var);
    }
}


void readSHPRecordHeader(gzFile fd, int &recordNumber, int& contentLength)
{
    sgReadIntBE(fd, recordNumber);
    sgReadIntBE(fd, contentLength);
}

void parseSHPPoints2D(gzFile fd, int numPoints, flightgear::SGGeodVec& aPoints)
{
    aPoints.reserve(numPoints);
    std::vector<double> ds;
    ds.resize(numPoints * 2);
    sgReadDouble(fd, numPoints * 2, ds.data());

    unsigned int index = 0;
    for (int i=0; i<numPoints; ++i, index += 2) {
        aPoints.push_back(SGGeod::fromDeg(ds[index], ds[index+1]));
    }
}

} // anonymous namespace

namespace flightgear
{

void SHPParser::parsePolyLines(const SGPath& aPath, PolyLine::Type aTy,
                               PolyLineList& aResult, bool aClosed)
{
#if defined(SG_WINDOWS)
	const auto ws = aPath.wstr();
	gzFile file = gzopen_w(ws.c_str(), "rb");
#else
    const auto s = aPath.utf8Str();
    gzFile file = gzopen(s.c_str(), "rb");
#endif
    if (!file) {
        throw sg_io_exception("couldn't open file:", aPath);
    }

    try {
        int header, fileLength, fileVersion, shapeType;
        sgReadIntBE(file, header);
        if (header != SHP_FILE_MAGIC) {
            throw sg_io_exception("bad SHP header value", aPath);
        }

        // skip 5 ints, then read the file length
        for (int i=0; i<6; ++i) {
            sgReadIntBE(file, fileLength);
        }

        sgReadIntLE(file, fileVersion);
        sgReadIntLE(file, shapeType);

        if (fileVersion != SHP_FILE_VERSION) {
            throw sg_io_exception("bad SHP file version", aPath);
        }

        if (aClosed && (shapeType != SHP_POLYGON_TYPE)) {
            throw sg_io_exception("SHP file does not contain Polygon data", aPath);
        }

       if (!aClosed && (shapeType != SHP_POLYLINE_TYPE)) {
            throw sg_io_exception("SHP file does not contain PolyLine data", aPath);
        }

        // we don't care about range values
        double range;
        for (int i=0; i<8; ++i) {
            sgReadDouble(file, &range);
        }

        int readLength = 100; // sizeof the header
        while (readLength < fileLength) {
            int recordNumber, contentLengthWords;
            readSHPRecordHeader(file, recordNumber, contentLengthWords);

            int recordShapeType;
            sgReadIntLE(file, recordShapeType);
            if (recordShapeType == SHP_NULL_TYPE) {
                continue; // nothing else to do
            }

            if (recordShapeType != shapeType) {
                // vesion 1000 requires files to have homogenous shape type
                throw sg_io_exception("SHP file shape-type mismatch", aPath);
            }
        // read PolyLine record from now on
            double box[4];
            for (int i=0; i<4; ++i) {
                sgReadDouble(file, &box[i]);
            }

            int numParts, numPoints;
            sgReadInt(file, &numParts);
            sgReadInt(file, &numPoints);

            std::vector<int> parts;
            parts.resize(numParts);
            sgReadInt(file, numParts, parts.data());

            SGGeodVec points;
            parseSHPPoints2D(file, numPoints, points);

            for (int part=0; part<numParts; ++part) {
                SGGeodVec partPoints;
                unsigned int startIndex = parts[part];
                unsigned int endIndex = ((part + 1) == numParts) ? numPoints : parts[part + 1];
                partPoints.insert(partPoints.begin(), points.begin() + startIndex, points.begin() + endIndex);

                if (aClosed) {
                    aResult.push_back(PolyLine::create(aTy, partPoints));
                } else {
                    PolyLineList lines = PolyLine::createChunked(aTy, partPoints);
                    aResult.insert(aResult.end(), lines.begin(), lines.end());
                }
            }

            // total record size if contentLenght + 4 words for the two record fields
            readLength += (contentLengthWords + 4);
        // partition
        } // of record parsing

    } catch (sg_exception& e) {
        aResult.clear();
        gzclose(file);
        throw e; // rethrow
    }
}

} // of namespace flightgear
