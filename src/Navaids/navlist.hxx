// navlist.hxx -- navaids management class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_NAVLIST_HXX
#define _FG_NAVLIST_HXX


#include <simgear/compiler.h>

#include <simgear/structure/SGSharedPtr.hxx>

#include <map>
#include <vector>
#include <string>

#include "navrecord.hxx"

// forward decls
class SGGeod;

// FGNavList ------------------------------------------------------------------

typedef SGSharedPtr<FGNavRecord> nav_rec_ptr;
typedef std::vector < nav_rec_ptr > nav_list_type;

class FGNavList
{
public:
  class TypeFilter : public FGPositioned::Filter
  {
  public:
    TypeFilter( const FGPositioned::Type type = FGPositioned::INVALID );
    TypeFilter( const FGPositioned::Type minType,
                const FGPositioned::Type maxType );
    
    /**
     * Construct from string containing type
     *
     * @param type One of "fix"|"vor"|"ndb"|"ils"|"dme"|"tacan"|"any"
     */
    bool fromTypeString(const std::string& type);
    
    virtual FGPositioned::Type minType() const { return _mintype; }
    virtual FGPositioned::Type maxType() const { return _maxtype; }
    
  protected:
    FGPositioned::Type _mintype;
    FGPositioned::Type _maxtype;
  };
  
  /**
   filter matching VOR & ILS/LOC transmitters
   */
  static TypeFilter* navFilter();
  
  /**
    * filter matching ILS/LOC transmitter
   */
  static TypeFilter* locFilter();
  
  static TypeFilter* ndbFilter();
  
  /**
   * Filter returning TACANs and VORTACs
   */
  static TypeFilter* tacanFilter();
  
  
  static TypeFilter* mobileTacanFilter();
  
    /** Query the database for the specified station.  It is assumed
      * that there will be multiple stations with matching frequencies
      * so a position must be specified.
      */
    static FGNavRecordRef findByFreq( double freq, const SGGeod& position,
                                   TypeFilter* filter = nullptr);

    /**
     * Overloaded version above - no positioned supplied so can be used with
     * mobile TACANs which have no valid position. The first match is
     * returned only.
     */
    static FGNavRecordRef findByFreq( double freq, TypeFilter* filter = NULL);
  
    static nav_list_type findAllByFreq( double freq, const SGGeod& position,
                                       TypeFilter* filter = NULL);
  
    // Given an Ident and optional frequency and type ,
    // return a list of matching stations.
    static nav_list_type findByIdentAndFreq( const std::string& ident,
                                             const double freq,
                                            TypeFilter* filter = NULL);

    // Given an Ident and optional frequency and type ,
    // return a list of matching stations sorted by distance to the given position
    static nav_list_type findByIdentAndFreq( const SGGeod & position,
        const std::string& ident, const double freq = 0.0,
                                            TypeFilter* filter = NULL);
  
  };


// FGTACANList ----------------------------------------------------------------


typedef SGSharedPtr<FGTACANRecord> tacan_rec_ptr;
typedef std::vector < tacan_rec_ptr > tacan_list_type;
typedef std::map < int, tacan_list_type > tacan_map_type;
typedef std::map < std::string, tacan_list_type > tacan_ident_map_type;

class FGTACANList {

    tacan_list_type channellist;
    tacan_map_type channels;
    tacan_ident_map_type ident_channels;

public:

    FGTACANList();
    ~FGTACANList();

    // initialize the TACAN list
    bool init();

    // add an entry
    bool add( FGTACANRecord *r );

    // Given a TACAN Channel, return the appropriate frequency.
    FGTACANRecord *findByChannel(const std::string& channel);

    // Given a TACAN Channel, return the appropriate frequency.
    FGTACANRecord *findByFrequency(int frequency_kHz);


};
#endif // _FG_NAVLIST_HXX
