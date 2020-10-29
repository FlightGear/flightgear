// apt_loader.cxx -- a front end loader of the apt.dat file.  This loader
//                   populates the runway and basic classes.
//
// Written by Curtis Olson, started August 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "apt_loader.hxx"

#include <simgear/compiler.h>

#include <stdlib.h> // atof(), atoi()
#include <string.h> // memchr()
#include <ctype.h> // isspace()
#include <cerrno>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <cstddef>              // std::size_t
#include <string>
#include <vector>
#include <utility>              // std::pair, std::move()

#include "airport.hxx"
#include "runways.hxx"
#include "pavement.hxx"
#include <Navaids/NavDataCache.hxx>
#include <Navaids/positioned.hxx>
#include <ATC/CommStation.hxx>

#include <iostream>
#include <sstream>              // std::istringstream

using std::vector;
using std::string;

namespace strutils = simgear::strutils;


static FGPositioned::Type fptypeFromRobinType(unsigned int aType)
{
  switch (aType) {
  case 1: return FGPositioned::AIRPORT;
  case 16: return FGPositioned::SEAPORT;
  case 17: return FGPositioned::HELIPORT;
  default:
    SG_LOG(SG_GENERAL, SG_ALERT, "unsupported type:" << aType);
    throw sg_range_exception("Unsupported airport type", "fptypeFromRobinType");
  }
}

namespace flightgear
{
APTLoader::APTLoader()
  :  last_apt_id(""),
     last_apt_elev(0.0),
     currentAirportPosID(0),
     cache(NavDataCache::instance())
{ }

APTLoader::~APTLoader() { }

void APTLoader::readAptDatFile(const SGPath &aptdb_file,
                               std::size_t bytesReadSoFar,
                               std::size_t totalSizeOfAllAptDatFiles)
{
  string apt_dat = aptdb_file.utf8Str(); // full path to the file being parsed
  sg_gzifstream in(aptdb_file, std::ios_base::in | std::ios_base::binary, true);

  if ( !in.is_open() ) {
    const std::string errMsg = simgear::strutils::error_string(errno);
    SG_LOG( SG_GENERAL, SG_ALERT,
            "Cannot open file '" << apt_dat << "': " << errMsg );
    throw sg_io_exception("Cannot open file (" + errMsg + ")",
                          sg_location(aptdb_file));
  }

  string line;

  unsigned int rowCode = 0;     // terminology used in the apt.dat format spec
  unsigned int line_num = 0;
  // "airport identifier": terminology used in the apt.dat format spec. It is
  // often an ICAO code, but not always.
  string currentAirportId;
  // Boolean used to make sure we don't try to load the same airport several
  // times. Defaults to true only to ensure we don't add garbage to
  // 'airportInfoMap' under the key "" (empty airport identifier) in case the
  // apt.dat file doesn't have a start-of-airport row code (1, 16 or 17) after
  // its header---which would be invalid, anyway.
  bool skipAirport = true;

  // Read the apt.dat header (two lines)
  while ( line_num < 2 && std::getline(in, line) ) {
    // 'line' may end with an \r character (tested on Linux, only \n was
    // stripped: std::getline() only discards the _native_ line terminator)
    line_num++;

    if ( line_num == 1 ) {
      std::string stripped_line = simgear::strutils::strip(line);
      // First line indicates IBM ("I") or Macintosh ("A") line endings.
      if ( stripped_line != "I" && stripped_line != "A" ) {
        std::string pb = "invalid first line (neither 'I' nor 'A')";
        SG_LOG( SG_GENERAL, SG_ALERT, aptdb_file << ": " << pb);
        throw sg_format_exception("cannot parse '" + apt_dat + "': " + pb,
                                  stripped_line);
      }
    } else {     // second line of the file
      vector<string> fields(strutils::split(line, 0, 1));

      if (fields.empty()) {
        string errMsg = "unable to parse format version: empty line";
        SG_LOG(SG_GENERAL, SG_ALERT, apt_dat << ": " << errMsg);
        throw sg_format_exception("cannot parse '" + apt_dat + "': " + errMsg,
                                  string());
      } else {
        unsigned int aptDatFormatVersion =
          strutils::readNonNegativeInt<unsigned int>(fields[0]);
        SG_LOG(SG_GENERAL, SG_INFO,
               "apt.dat format version (" << apt_dat << "): " <<
               aptDatFormatVersion);
      }
    }
  } // end of the apt.dat header

  throwExceptionIfStreamError(in, aptdb_file);

  while ( std::getline(in, line) ) {
    // 'line' may end with an \r character, see above
    line_num++;

    if ( isBlankOrCommentLine(line) )
      continue;

    if ((line_num % 100) == 0) {
      // every 100 lines
      unsigned int percent = ((bytesReadSoFar + in.approxOffset()) * 100)
                             / totalSizeOfAllAptDatFiles;
      cache->setRebuildPhaseProgress(
        NavDataCache::REBUILD_READING_APT_DAT_FILES, percent);
    }

    // Extract the first field into 'rowCode'
    rowCode = atoi(line.c_str());

    if ( rowCode == 1  /* Airport */ ||
         rowCode == 16 /* Seaplane base */ ||
         rowCode == 17 /* Heliport */ ) {
      vector<string> tokens(simgear::strutils::split(line));
      if (tokens.size() < 6) {
        SG_LOG( SG_GENERAL, SG_WARN,
                apt_dat << ":"  << line_num << ": invalid airport header "
                "(at least 6 fields are required)" );
        skipAirport = true; // discard everything until the next airport header
        continue;
      }

      currentAirportId = tokens[4]; // often an ICAO, but not always
      // Check if the airport is already in 'airportInfoMap'; get the
      // existing entry, if any, otherwise insert a new one.
      std::pair<AirportInfoMapType::iterator, bool>
        insertRetval = airportInfoMap.insert(
          AirportInfoMapType::value_type(currentAirportId, RawAirportInfo()));
      skipAirport = !insertRetval.second;

      if ( skipAirport ) {
        SG_LOG( SG_GENERAL, SG_INFO,
                apt_dat << ":"  << line_num << ": skipping airport " <<
                currentAirportId << " (already defined earlier)" );
      } else {
        // We haven't seen this airport yet in any apt.dat file
        RawAirportInfo& airportInfo = insertRetval.first->second;
        airportInfo.file = aptdb_file;
        airportInfo.rowCode = rowCode;
        airportInfo.firstLineNum = line_num;
        airportInfo.firstLineTokens = std::move(tokens);
      }
    } else if ( rowCode == 99 ) {
      SG_LOG( SG_GENERAL, SG_DEBUG,
              apt_dat << ":"  << line_num << ": code 99 found "
              "(normally at end of file)" );
    } else if ( !skipAirport ) {
      // Line belonging to an already started, and not skipped airport entry;
      // just append it.
      airportInfoMap[currentAirportId].otherLines.emplace_back(
        line_num, rowCode, line);
    }
  } // of file reading loop

  throwExceptionIfStreamError(in, aptdb_file);
}

static bool isCommLine(const int code)
{
    return ((code >= 50) && (code <= 56)) || ((code >= 1050) && (code <= 1056));
}

void APTLoader::loadAirports()
{
  AirportInfoMapType::size_type nbLoadedAirports = 0;
  AirportInfoMapType::size_type nbAirports = airportInfoMap.size();

  // Loop over all airports found in all apt.dat files
  for (AirportInfoMapType::const_iterator it = airportInfoMap.begin();
       it != airportInfoMap.end(); it++) {
    // Full path to the apt.dat file this airport info comes from
    const string aptDat = it->second.file.utf8Str();
    last_apt_id = it->first;    // this is just the current airport identifier
    // The first line for this airport was already split over whitespace, but
    // remains to be parsed for the most part.
    parseAirportLine(it->second.rowCode, it->second.firstLineTokens);
    const LinesList& lines = it->second.otherLines;

    // Loop over the second and subsequent lines
    for (LinesList::const_iterator linesIt = lines.begin();
         linesIt != lines.end(); linesIt++) {
      // Beware that linesIt->str may end with an '\r' character, see above!
      unsigned int rowCode = linesIt->rowCode;

      if ( rowCode == 10 ) { // Runway v810
        parseRunwayLine810(aptDat, linesIt->number,
                           simgear::strutils::split(linesIt->str));
      } else if ( rowCode == 100 ) { // Runway v850
        parseRunwayLine850(aptDat, linesIt->number,
                           simgear::strutils::split(linesIt->str));
      } else if ( rowCode == 101 ) { // Water Runway v850
        parseWaterRunwayLine850(aptDat, linesIt->number,
                                simgear::strutils::split(linesIt->str));
      } else if ( rowCode == 102 ) { // Helipad v850
        parseHelipadLine850(aptDat, linesIt->number,
                            simgear::strutils::split(linesIt->str));
      } else if ( rowCode == 18 ) {
        // beacon entry (ignore)
      } else if ( rowCode == 14 ) {  // Viewpoint/control tower
        parseViewpointLine(aptDat, linesIt->number,
                           simgear::strutils::split(linesIt->str));
      } else if ( rowCode == 19 ) {
        // windsock entry (ignore)
      } else if ( rowCode == 20 ) {
        // Taxiway sign (ignore)
      } else if ( rowCode == 21 ) {
        // lighting objects (ignore)
      } else if ( rowCode == 15 ) {
        // custom startup locations (ignore)
      } else if ( rowCode == 0 ) {
        // ??
      } else if ( isCommLine(rowCode)) {
        parseCommLine(aptDat, linesIt->number, rowCode,
                      simgear::strutils::split(linesIt->str));
      } else if ( rowCode == 110 ) {
        pavement = true;
        parsePavementLine850(simgear::strutils::split(linesIt->str, 0, 4));
      } else if ( rowCode >= 111 && rowCode <= 114 ) {
        if ( pavement )
          parsePavementNodeLine850(aptDat, linesIt->number, rowCode,
                                   simgear::strutils::split(linesIt->str));
      } else if ( rowCode >= 115 && rowCode <= 116 ) {
        // other pavement nodes (ignore)
      } else if ( rowCode == 120 ) {
        pavement = false;
      } else if ( rowCode == 130 ) {
        pavement = false;
      } else if ( rowCode >= 1000 ) {
        // airport traffic flow (ignore)
      } else {
        std::ostringstream oss;
        string cleanedLine = cleanLine(linesIt->str);
        oss << aptDat << ":" << linesIt->number << ": unknown row code " <<
          rowCode;
        SG_LOG( SG_GENERAL, SG_ALERT, oss.str() << " (" << cleanedLine << ")" );
        throw sg_format_exception(oss.str(), cleanedLine);
      }
    } // of loop over the second and subsequent apt.dat lines for the airport

    finishAirport(aptDat);
    nbLoadedAirports++;

    if ((nbLoadedAirports % 300) == 0) {
      // Every 300 airports
      unsigned int percent = nbLoadedAirports * 100 / nbAirports;
      cache->setRebuildPhaseProgress(NavDataCache::REBUILD_LOADING_AIRPORTS,
                                     percent);
    }
  } // of loop over 'airportInfoMap'

  SG_LOG( SG_GENERAL, SG_INFO,
          "Loaded data for " << nbLoadedAirports << " airports" );
}

// Tell whether an apt.dat line is blank or a comment line
bool APTLoader::isBlankOrCommentLine(const std::string& line)
{
  size_t pos = line.find_first_not_of(" \t");
  return ( pos == std::string::npos ||
           line[pos] == '\r' ||
           line.find("##", pos) == pos );
}

std::string APTLoader::cleanLine(const std::string& line)
{
  std::string res = line;

  // Lines obtained from readAptDatFile() may end with \r, which can be quite
  // confusing when printed to the terminal.
  for (std::string::reverse_iterator it = res.rbegin();
       it != res.rend() && *it == '\r'; /* empty */)
  { // The beauty of C++ iterators...
    it = std::string::reverse_iterator(res.erase( (it+1).base() ));
  }

  return res;
}

void APTLoader::throwExceptionIfStreamError(const sg_gzifstream& input_stream,
                                            const SGPath& path)
{
  if (input_stream.bad()) {
    const std::string errMsg = simgear::strutils::error_string(errno);

    SG_LOG( SG_NAVAID, SG_ALERT,
            "Error while reading '" << path.utf8Str() << "': " << errMsg );
    throw sg_io_exception("Error reading file (" + errMsg + ")",
                          sg_location(path));
  }
}

void APTLoader::finishAirport(const string& aptDat)
{
  if (currentAirportPosID == 0) {
    return;
  }

  if (!rwy_count) {
    currentAirportPosID = 0;
    SG_LOG( SG_GENERAL, SG_ALERT, "Error in '" << aptDat <<
            "': no runways for " << last_apt_id << ", skipping." );
    return;
  }

  double lat = rwy_lat_accum / (double)rwy_count;
  double lon = rwy_lon_accum / (double)rwy_count;

  SGGeod pos(SGGeod::fromDegFt(lon, lat, last_apt_elev));
  cache->updatePosition(currentAirportPosID, pos);

  currentAirportPosID = 0;
}

// 'rowCode' is passed to avoid decoding it twice, since that work was already
// done in order to detect the start of the new airport.
void APTLoader::parseAirportLine(unsigned int rowCode,
                                 const vector<string>& token)
{
  // The algorithm in APTLoader::readAptDatFile() ensures this is at least 5.
  vector<string>::size_type lastIndex = token.size() - 1;
  const string& id(token[4]);
  double elev = atof( token[1].c_str() );
  last_apt_elev = elev;

  string name;
  // build the name
  for ( vector<string>::size_type i = 5; i < lastIndex; ++i ) {
    name += token[i] + " ";
  }
  name += token[lastIndex];

  // clear runway list for start of next airport
  rwy_lon_accum = 0.0;
  rwy_lat_accum = 0.0;
  rwy_count = 0;

  currentAirportPosID = cache->insertAirport(fptypeFromRobinType(rowCode),
                                             id, name);
}

void APTLoader::parseRunwayLine810(const string& aptDat, unsigned int lineNum,
                                   const vector<string>& token)
{
  if (token.size() < 11) {
    SG_LOG( SG_GENERAL, SG_WARN,
            aptDat << ":" << lineNum << ": invalid v810 runway line " <<
            "(row code 10): at least 11 fields are required" );
    return;
  }

  double lat = atof( token[1].c_str() );
  double lon = atof( token[2].c_str() );
  rwy_lat_accum += lat;
  rwy_lon_accum += lon;
  rwy_count++;

  const string& rwy_no(token[3]);

  double heading = atof( token[4].c_str() );
  double length = atoi( token[5].c_str() );
  double width = atoi( token[8].c_str() );
  length *= SG_FEET_TO_METER;
  width *= SG_FEET_TO_METER;

  // adjust lat / lon to the start of the runway/taxiway, not the middle
  SGGeod pos_1 = SGGeodesy::direct( SGGeod::fromDegFt(lon, lat, last_apt_elev),
                                    heading, -length/2 );

  last_rwy_heading = heading;

  int surface_code = atoi( token[10].c_str() );

  if (rwy_no[0] == 'x') {  // Taxiway
    cache->insertRunway(
      FGPositioned::TAXIWAY, rwy_no, pos_1, currentAirportPosID,
      heading, length, width, 0.0, 0.0, surface_code);
  } else if (rwy_no[0] == 'H') {  // Helipad
    SGGeod pos(SGGeod::fromDegFt(lon, lat, last_apt_elev));
    cache->insertRunway(FGPositioned::HELIPAD, rwy_no, pos, currentAirportPosID,
                        heading, length, width, 0.0, 0.0, surface_code);
  } else {
    // (pair of) runways
    string rwy_displ_threshold = token[6];
    vector<string> displ
      = simgear::strutils::split( rwy_displ_threshold, "." );
    double displ_thresh1 = atof( displ[0].c_str() );
    double displ_thresh2 = atof( displ[1].c_str() );
    displ_thresh1 *= SG_FEET_TO_METER;
    displ_thresh2 *= SG_FEET_TO_METER;

    string rwy_stopway = token[7];
    vector<string> stop
      = simgear::strutils::split( rwy_stopway, "." );
    double stopway1 = atof( stop[0].c_str() );
    double stopway2 = atof( stop[1].c_str() );
    stopway1 *= SG_FEET_TO_METER;
    stopway2 *= SG_FEET_TO_METER;

    SGGeod pos_2 = SGGeodesy::direct( pos_1, heading, length );

    PositionedID rwy = cache->insertRunway(FGPositioned::RUNWAY, rwy_no, pos_1,
                                           currentAirportPosID, heading, length,
                                           width, displ_thresh1, stopway1,
                                           surface_code);

    PositionedID reciprocal = cache->insertRunway(
      FGPositioned::RUNWAY,
      FGRunway::reverseIdent(rwy_no), pos_2,
      currentAirportPosID,
      SGMiscd::normalizePeriodic(0, 360, heading + 180.0),
      length, width, displ_thresh2, stopway2,
      surface_code);

    cache->setRunwayReciprocal(rwy, reciprocal);
  }
}

void APTLoader::parseRunwayLine850(const string& aptDat, unsigned int lineNum,
                                   const vector<string>& token)
{
  if (token.size() < 26) {
    SG_LOG( SG_GENERAL, SG_WARN,
            aptDat << ":" << lineNum << ": invalid v850 runway line " <<
            "(row code 100): at least 26 fields are required" );
    return;
  }

  double width = atof( token[1].c_str() );
  int surface_code = atoi( token[2].c_str() );

  double lat_1 = atof( token[9].c_str() );
  double lon_1 = atof( token[10].c_str() );
  SGGeod pos_1(SGGeod::fromDegFt(lon_1, lat_1, 0.0));
  rwy_lat_accum += lat_1;
  rwy_lon_accum += lon_1;
  rwy_count++;

  double lat_2 = atof( token[18].c_str() );
  double lon_2 = atof( token[19].c_str() );
  SGGeod pos_2(SGGeod::fromDegFt(lon_2, lat_2, 0.0));
  rwy_lat_accum += lat_2;
  rwy_lon_accum += lon_2;
  rwy_count++;

  double length, heading_1, heading_2;
  SGGeodesy::inverse( pos_1, pos_2, heading_1, heading_2, length );

  last_rwy_heading = heading_1;

  const string& rwy_no_1(token[8]);
  const string& rwy_no_2(token[17]);
  if ( rwy_no_1.empty() || rwy_no_2.empty() ) // these tests are weird...
    return;

  double displ_thresh1 = atof( token[11].c_str() );
  double displ_thresh2 = atof( token[20].c_str() );

  double stopway1 = atof( token[12].c_str() );
  double stopway2 = atof( token[21].c_str() );

  PositionedID rwy = cache->insertRunway(FGPositioned::RUNWAY, rwy_no_1, pos_1,
                                         currentAirportPosID, heading_1, length,
                                         width, displ_thresh1, stopway1,
                                         surface_code);

  PositionedID reciprocal = cache->insertRunway(
    FGPositioned::RUNWAY,
    rwy_no_2, pos_2,
    currentAirportPosID, heading_2, length,
    width, displ_thresh2, stopway2,
    surface_code);

  cache->setRunwayReciprocal(rwy, reciprocal);
}

void APTLoader::parseWaterRunwayLine850(const string& aptDat,
                                        unsigned int lineNum,
                                        const vector<string>& token)
{
  if (token.size() < 9) {
    SG_LOG( SG_GENERAL, SG_WARN,
            aptDat << ":" << lineNum << ": invalid v850 water runway line " <<
            "(row code 101): at least 9 fields are required" );
    return;
  }

  double width = atof( token[1].c_str() );

  double lat_1 = atof( token[4].c_str() );
  double lon_1 = atof( token[5].c_str() );
  SGGeod pos_1(SGGeod::fromDegFt(lon_1, lat_1, 0.0));
  rwy_lat_accum += lat_1;
  rwy_lon_accum += lon_1;
  rwy_count++;

  double lat_2 = atof( token[7].c_str() );
  double lon_2 = atof( token[8].c_str() );
  SGGeod pos_2(SGGeod::fromDegFt(lon_2, lat_2, 0.0));
  rwy_lat_accum += lat_2;
  rwy_lon_accum += lon_2;
  rwy_count++;

  double length, heading_1, heading_2;
  SGGeodesy::inverse( pos_1, pos_2, heading_1, heading_2, length );

  last_rwy_heading = heading_1;

  const string& rwy_no_1(token[3]);
  const string& rwy_no_2(token[6]);

  PositionedID rwy = cache->insertRunway(FGPositioned::RUNWAY, rwy_no_1, pos_1,
                                         currentAirportPosID, heading_1, length,
                                         width, 0.0, 0.0, 13);

  PositionedID reciprocal = cache->insertRunway(
    FGPositioned::RUNWAY,
    rwy_no_2, pos_2,
    currentAirportPosID, heading_2, length,
    width, 0.0, 0.0, 13);

  cache->setRunwayReciprocal(rwy, reciprocal);
}

void APTLoader::parseHelipadLine850(const string& aptDat, unsigned int lineNum,
                                    const vector<string>& token)
{
  if (token.size() < 12) {
    SG_LOG( SG_GENERAL, SG_WARN,
            aptDat << ":" << lineNum << ": invalid v850 helipad line " <<
            "(row code 102): at least 12 fields are required" );
    return;
  }

  double length = atof( token[5].c_str() );
  double width = atof( token[6].c_str() );

  double lat = atof( token[2].c_str() );
  double lon = atof( token[3].c_str() );
  SGGeod pos(SGGeod::fromDegFt(lon, lat, 0.0));
  rwy_lat_accum += lat;
  rwy_lon_accum += lon;
  rwy_count++;

  double heading = atof( token[4].c_str() );

  last_rwy_heading = heading;

  const string& rwy_no(token[1]);
  int surface_code = atoi( token[7].c_str() );

  cache->insertRunway(FGPositioned::HELIPAD, rwy_no, pos,
                      currentAirportPosID, heading, length,
                      width, 0.0, 0.0, surface_code);
}

void APTLoader::parseViewpointLine(const string& aptDat, unsigned int lineNum,
                                   const vector<string>& token)
{
  if (token.size() < 5) {
    SG_LOG( SG_GENERAL, SG_WARN,
            aptDat << ":" << lineNum << ": invalid viewpoint line "
            "(row code 14): at least 5 fields are required" );
  } else {
    double lat = atof(token[1].c_str());
    double lon = atof(token[2].c_str());
    double elev = atof(token[3].c_str());
    tower = SGGeod::fromDegFt(lon, lat, elev + last_apt_elev);
    cache->insertTower(currentAirportPosID, tower);
  }
}

void APTLoader::parsePavementLine850(const vector<string>& token)
{
  if ( token.size() >= 5 ) {
    pavement_ident = token[4];
    if ( !pavement_ident.empty() &&
         pavement_ident[pavement_ident.size()-1] == '\r' )
      pavement_ident.erase( pavement_ident.size()-1 );
  } else {
    pavement_ident = "xx";
  }
}

void APTLoader::parsePavementNodeLine850(const string& aptDat,
                                         unsigned int lineNum, int rowCode,
                                         const vector<string>& token)
{
  static const unsigned int minNbTokens[] = {3, 5, 3, 5};
  assert(111 <= rowCode && rowCode <= 114);

  if (token.size() < minNbTokens[rowCode-111]) {
    SG_LOG( SG_GENERAL, SG_WARN,
            aptDat << ":" << lineNum << ": invalid v850 node line " <<
            "(row code " << rowCode << "): at least " <<
            minNbTokens[rowCode-111] << " fields are required" );
    return;
  }

  double lat = atof( token[1].c_str() );
  double lon = atof( token[2].c_str() );
  SGGeod pos(SGGeod::fromDegFt(lon, lat, 0.0));

  FGPavement* pvt = 0;
  if ( !pavement_ident.empty() ) {
    pvt = new FGPavement( 0, pavement_ident, pos );
    pavements.push_back( pvt );
    pavement_ident = "";
  } else {
    pvt = pavements.back();
  }
  if ( rowCode == 112 || rowCode == 114 ) {
    double lat_b = atof( token[3].c_str() );
    double lon_b = atof( token[4].c_str() );
    SGGeod pos_b(SGGeod::fromDegFt(lon_b, lat_b, 0.0));
    pvt->addBezierNode(pos, pos_b, rowCode == 114);
  } else {
    pvt->addNode(pos, rowCode == 113);
  }
}

void APTLoader::parseCommLine(const string& aptDat,
                              unsigned int lineNum, unsigned int rowCode,
                              const vector<string>& token)
{
  if (token.size() < 3) {
    SG_LOG( SG_GENERAL, SG_WARN,
            aptDat << ":" << lineNum << ": invalid Comm Frequency line " <<
            "(row code " << rowCode << "): at least 3 fields are required" );
    return;
  } else if (rwy_count <= 0) {
    SG_LOG( SG_GENERAL, SG_ALERT,
            aptDat << ":" << lineNum << ": no runways, skipping Comm " <<
            "Frequency line for " << last_apt_id );
    // There used to be no 'return' here. Not sure how useful this is, but
    // clearly this code block doesn't make sense without it, so here it is.
    return;
  }

  SGGeod pos = SGGeod::fromDegFt(rwy_lon_accum / (double)rwy_count,
                                 rwy_lat_accum / (double)rwy_count,
                                 last_apt_elev);

  const bool isAPT1000Code = rowCode > 1000;
  if (isAPT1000Code) {
      rowCode -= 1000;
  }

  // short int representing tens of kHz:
  int freqKhz = std::stoi(token[1]) * (isAPT1000Code ? 1 : 10);
  int rangeNm = 50;
  FGPositioned::Type ty;

  switch (rowCode) {
  case 50:
    ty = FGPositioned::FREQ_AWOS;
    for (size_t i = 2; i < token.size(); ++i)
    {
      if (token[i] == "ATIS")
      {
        ty = FGPositioned::FREQ_ATIS;
        break;
      }
    }
    break;

  case 51:    ty = FGPositioned::FREQ_UNICOM; break;
  case 52:    ty = FGPositioned::FREQ_CLEARANCE; break;
  case 53:    ty = FGPositioned::FREQ_GROUND; break;
  case 54:    ty = FGPositioned::FREQ_TOWER; break;
  case 55:
  case 56:    ty = FGPositioned::FREQ_APP_DEP; break;
  default:
    throw sg_range_exception("unsupported apt.dat comm station type");
  }

  // Name can contain whitespace. All tokens after the second token are
  // part of the name.
  string name = token[2];
  for (size_t i = 3; i < token.size(); ++i)
    name += ' ' + token[i];

  cache->insertCommStation(ty, name, pos, freqKhz, rangeNm,
                           currentAirportPosID);
}

// The 'metar.dat' file lists the airports that have METAR available.
bool metarDataLoad(const SGPath& metar_file)
{
  sg_gzifstream metar_in( metar_file );
  if ( !metar_in.is_open() ) {
    SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << metar_file );
    return false;
  }

  NavDataCache* cache = NavDataCache::instance();

  string ident;
  while ( metar_in ) {
    metar_in >> ident;
    if ( ident == "#" || ident == "//" ) {
      metar_in >> skipeol;
    } else {
      cache->setAirportMetar(ident, true);
    }
  }

  return true;
}

} // of namespace flightgear
