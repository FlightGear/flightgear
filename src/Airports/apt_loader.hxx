// apt_loader.hxx -- a front end loader of the apt.dat file.  This loader
//                   populates the runway and basic classes.
//
// Written by Curtis Olson, started December 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//
// $Id$


#ifndef _FG_APT_LOADER_HXX
#define _FG_APT_LOADER_HXX

#include <string>
#include <vector>
#include <unordered_map>
#include "airport.hxx"

#include <simgear/compiler.h>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGGeod.hxx>
#include <simgear/misc/sg_path.hxx>
#include <Navaids/positioned.hxx>

class NavDataCache;
class sg_gzifstream;
class FGPavement;

namespace flightgear
{

class APTLoader
{
public:
  APTLoader();
  ~APTLoader();

  // Read the specified apt.dat file into 'airportInfoMap'.
  // 'bytesReadSoFar' and 'totalSizeOfAllAptDatFiles' are used for progress
  // information.
  void readAptDatFile(const SGPath& aptdb_file, std::size_t bytesReadSoFar,
                      std::size_t totalSizeOfAllAptDatFiles);
  // Read all airports gathered in 'airportInfoMap' and load them into the
  // navdata cache (even in case of overlapping apt.dat files,
  // 'airportInfoMap' has only one entry per airport).
  void loadAirports();

  // Load a specific airport defined in aptdb_file, and return a "rich" view
  // of the airport including taxiways, pavement and line features.
  const FGAirport* loadAirportFromFile(std::string id, const SGPath& aptdb_file);

private:
  struct Line
  {
    Line(unsigned int number_, unsigned int rowCode_, std::string str_)
      : number(number_), rowCode(rowCode_), str(str_) { }

    unsigned int number;
    unsigned int rowCode;         // Terminology of the apt.dat spec
    std::string str;
  };

  typedef std::vector<Line> LinesList;

  struct RawAirportInfo
  {
    // apt.dat file where the airport was defined
    SGPath file;
    // Row code for the airport (1, 16 or 17)
    unsigned int rowCode;
    // Line number in the apt.dat file where the airport definition starts
    unsigned int firstLineNum;
    // The whitespace-separated strings comprising the first line of the airport
    // definition
    std::vector<std::string> firstLineTokens;
    // Subsequent lines of the airport definition (one element per line)
    LinesList otherLines;
  };

  typedef std::unordered_map<std::string, RawAirportInfo> AirportInfoMapType;
  typedef SGSharedPtr<FGPavement> FGPavementPtr;
  typedef std::vector<FGPavementPtr> NodeList;

  APTLoader(const APTLoader&);            // disable copy constructor
  APTLoader& operator=(const APTLoader&); // disable copy-assignment operator

  const FGAirport* loadAirport(const string aptDat, const std::string airportID, RawAirportInfo* airport_info, bool createFGAirport=false);

  // Tell whether an apt.dat line is blank or a comment line
  bool isBlankOrCommentLine(const std::string& line);
  // Return a copy of 'line' with trailing '\r' char(s) removed
  std::string cleanLine(const std::string& line);
  void throwExceptionIfStreamError(const sg_gzifstream& input_stream,
                                   const SGPath& path);
  void parseAirportLine(unsigned int rowCode,
                        const std::vector<std::string>& token);
  void finishAirport(const std::string& aptDat);
  void parseRunwayLine810(const std::string& aptDat, unsigned int lineNum,
                          const std::vector<std::string>& token);
  void parseRunwayLine850(const std::string& aptDat, unsigned int lineNum,
                          const std::vector<std::string>& token);
  void parseWaterRunwayLine850(const std::string& aptDat, unsigned int lineNum,
                               const std::vector<std::string>& token);
  void parseHelipadLine850(const std::string& aptDat, unsigned int lineNum,
                           const std::vector<std::string>& token);
  void parseViewpointLine(const std::string& aptDat, unsigned int lineNum,
                          const std::vector<std::string>& token);
  void parsePavementLine850(const std::vector<std::string>& token);
  void parseNodeLine850(
    NodeList *nodelist,
    const std::string& aptDat, unsigned int lineNum, int rowCode,
    const std::vector<std::string>& token);

  void parseCommLine(
    const std::string& aptDat, unsigned int lineNum, unsigned int rowCode,
    const std::vector<std::string>& token);

  std::vector<std::string> token;
  AirportInfoMapType airportInfoMap;
  double rwy_lat_accum;
  double rwy_lon_accum;
  double last_rwy_heading;
  int rwy_count;
  std::string last_apt_id;
  double last_apt_elev;
  SGGeod tower;

  std::string pavement_ident;
  NodeList pavements;
  NodeList airport_boundary;
  NodeList linear_feature;

  // Not an airport identifier in the sense of the apt.dat spec!
  PositionedID currentAirportPosID;
  NavDataCache* cache;

  // Enum to keep track of whether we are tracking a pavement, airport boundary
  // or linear feature when parsing the file.
  enum NodeBlock { None, Pavement, AirportBoundary, LinearFeature};
};

bool metarDataLoad(const SGPath& path);

} // of namespace flighgear

#endif // _FG_APT_LOADER_HXX
