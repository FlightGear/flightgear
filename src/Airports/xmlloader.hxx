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

#ifndef _XML_LOADER_HXX_
#define _XML_LOADER_HXX_

class FGAirportDynamics;
class FGRunwayPreference;
class FGSidStar;

class XMLVisitor; // ffrom easyxml.hxx

class XMLLoader {
public:
  XMLLoader();
  ~XMLLoader();
  
  static void load(FGRunwayPreference* p);
  static void load(FGAirportDynamics*  d);
  static void load(FGSidStar*          s);
  
  /**
   * Search the scenery for a file name of the form:
   *   I/C/A/ICAO.filename.xml
   * and parse it as an XML property list, passing the data to the supplied
   * visitor. If no such file could be found, returns false, otherwise returns
   * true. Other failures (malformed XML, etc) with throw an exception.
   */
  static bool loadAirportXMLDataIntoVisitor(const std::string& aICAO, 
    const std::string& aFileName, XMLVisitor& aVisitor);
  
  /**
   * Search the scenery for a file name of the form:
   *   I/C/A/ICAO.filename.xml
   * and return the corresponding SGPath if found (and true),
   * or false and invalid path if no matching data could be found
   */
  static bool findAirportData(const std::string& aICAO, 
    const std::string& aFileName, SGPath& aPath);
};

#endif
