/**************************************************************************
 * navaids.hpp -- navigation defines and prototypes
 *
 * Written by Charles Hotchkiss, started March 1998.
 *
 * Copyright (C) 1998  Charles Hotchkiss chotchkiss@namg.us.anritsu.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


// Class nav_aid
//
//  Includes NDB and IMO Markers, base for all other navigational aids.
//  The destructor is virtual to permit use of nav_aid pointers to derived
//  classes, but is instantiable none the less.
//  Assumes global pointer to current aircraft.

class nav_aid
  private:
    LRECT  here_at;   // Codes left = Long,
                      //        top = Lat,
                      //      right = Alt,
                      //     bottom = Range
    UINT   frequency; // special coding
    UINT   range;
    char  *ID;
    char  *Name;

  public:
    nav_aid( LRECT loc,
             UNIT freq, UINT range,
             const char *eye_dee, const char *MyName );

    virtual ~nav_aid();
    operator ==

    void update_status( void );  // Called by main loop?

    bool        in_range   ( LRECT loc );
    const char *pID        ( void );
    const char *pName      ( void );
    Cartestian  Location   ( void );
};

// Class ATIS_stn
//

class ATIS_stn :: public nav_aid {
  private:
    int   runways;
    int  *pref_rnwys; // prefered runway list;
    char *format;

  public:
    bool set_message( const char *pmsg ); // MSFS coded message
    ~ATIS_stn();
    ATIS_stn( LRECT here,
              UINT  radius,
              char *eye_dee,
              char *itsname,
              int  *runways,
              char *defaultmsg);
};

// Class DirBeacon
//
// Includes VOR Stations. Base for Omni?

class DirBeacon :: public nav_aid{
  private:
    UINT        radial;

  public:
    int         phase_angle( void );
    int         deflection ( void );
    double      ground_rate( void );
};

// Class ils_station
//
// Includes ILS

class ils_station :: public DirBeacon{
  private:
    long slope_freq;
    UINT glide_angle;
    UINT approach_angle;

  public:
    ULONG deflection( void );
};
