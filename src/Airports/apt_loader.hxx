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

#include <simgear/compiler.h>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGGeod.hxx>
#include <Navaids/positioned.hxx>

// Forward declarations
class SGPath;
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

  void parseAPT(const SGPath &aptdb_file);

private:
  typedef SGSharedPtr<FGPavement> FGPavementPtr;

  APTLoader(const APTLoader&);            // disable copy constructor
  APTLoader& operator=(const APTLoader&); // disable copy-assignment operator

  // Tell whether an apt.dat line is blank or a comment line
  bool isBlankOrCommentLine(const std::string& line);
  void throwExceptionIfStreamError(const sg_gzifstream& input_stream,
                                   const SGPath& path);
  void parseAirportLine(const std::string& aptDat,
                        const std::vector<std::string>& token);
  void finishAirport(const std::string& aptDat);
  void parseRunwayLine810(const std::vector<std::string>& token);
  void parseRunwayLine850(const std::vector<std::string>& token);
  void parseWaterRunwayLine850(const std::vector<std::string>& token);
  void parseHelipadLine850(const std::vector<std::string>& token);
  void parsePavementLine850(const std::vector<std::string>& token);
  void parsePavementNodeLine850(int num, const std::vector<std::string>& token);

  void parseCommLine(const std::string& aptDat, int lineId,
                     const std::vector<std::string>& token);

  double rwy_lat_accum;
  double rwy_lon_accum;
  double last_rwy_heading;
  int rwy_count;
  std::string last_apt_id;
  double last_apt_elev;
  SGGeod tower;

  std::string pavement_ident;
  bool pavement;
  std::vector<FGPavementPtr> pavements;

  NavDataCache* cache;
  // Not an airport identifier in the sense of the apt.dat spec!
  PositionedID currentAirportID;
};

// Load the airport data base from the specified aptdb file.  The
// metar file is used to mark the airports as having metar available
// or not.

bool airportDBLoad(const SGPath& path);

bool metarDataLoad(const SGPath& path);

} // of namespace flighgear

#endif // _FG_APT_LOADER_HXX
