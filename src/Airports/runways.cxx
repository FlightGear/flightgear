// runways.cxx -- a simple class to manage airport runway info
//
// Written by Curtis Olson, started August 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <math.h>               // fabs()
#include <stdio.h>              // sprintf()

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>

#include STL_STRING
#include STL_IOSTREAM
#include <map>

#include "runways.hxx"

SG_USING_NAMESPACE(std);
SG_USING_STD(istream);
SG_USING_STD(multimap);

inline istream&
operator >> ( istream& in, FGRunway& a )
{
    string type;
    int tmp;

    in >> a.type;
    if ( a.type == "R" ) {
        in >> a.id >> a.rwy_no >> a.lat >> a.lon >> a.heading
           >> a.length >> a.width >> a.surface_flags >> a.end1_flags
           >> tmp >> tmp >> a.end2_flags >> tmp >> tmp;
    } else if ( a.type == "T" ) {
        // in >> a.id >> a.rwy_no >> a.lat >> a.lon >> a.heading
        //    >> a.length >> a.width >> a.surface_flags;
        in >> skipeol;
    } else {
        in >> skipeol;
    }

    return in;
}


FGRunwayList::FGRunwayList( const string& file ) {
    SG_LOG( SG_GENERAL, SG_DEBUG, "Reading runway list: " << file );

    // open the specified file for reading
    sg_gzifstream in( file );
    if ( !in.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
	exit(-1);
    }

    // skip header line
    in >> skipeol;

    FGRunway rwy;
    while ( in ) {
        in >> rwy;
        if(rwy.type == "R") {
            runways.insert(pair<const string, FGRunway>(rwy.id, rwy));
        }
    }
}


// Return reverse rwy number
// eg 01 -> 19
// 03L -> 21R
static string GetReverseRunwayNo(string rwyno) {	
    // cout << "Original rwyno = " << rwyNo << '\n';
    
    // standardize input number
    string tmp = rwyno.substr(1, 1);
    if (( tmp == "L" || tmp == "R" || tmp == "C" ) || (rwyno.size() == 1)) {
	tmp = rwyno;
	rwyno = "0" + tmp;
        SG_LOG( SG_GENERAL, SG_INFO,
                "Standardising rwy number from " << tmp << " to " << rwyno );
    }
    
    char buf[4];
    int rn = atoi(rwyno.substr(0,2).c_str());
    rn += 18;
    while(rn > 36) {
	rn -= 36;
    }
    sprintf(buf, "%02i", rn);
    if(rwyno.size() == 3) {
	if(rwyno.substr(2,1) == "L") {
	    buf[2] = 'R';
	    buf[3] = '\0';
	} else if (rwyno.substr(2,1) == "R") {
	    buf[2] = 'L';
	    buf[3] = '\0';
	} else if (rwyno.substr(2,1) == "C") {
	    buf[2] = 'C';
	    buf[3] = '\0';
	} else if (rwyno.substr(2,1) == "T") {
	    buf[2] = 'T';
	    buf[3] = '\0';
	} else {
	    SG_LOG(SG_GENERAL, SG_ALERT, "Unknown runway code "
	    << rwyno << " passed to GetReverseRunwayNo(...)");
	}
    }
    return(buf);
}


// search for the specified apt id (wierd!)
bool FGRunwayList::search( const string& aptid, FGRunway* r ) {
    runway_map_iterator pos;

    pos = runways.lower_bound(aptid);
    if ( pos != runways.end() ) {
        current = pos;
        *r = pos->second;
        return true;
    } else {
        return false;
    }
}


// search for the specified apt id and runway no
bool FGRunwayList::search( const string& aptid, const string& rwyno,
                           FGRunway *r )
{
    string revrwyno = "";
    string runwayno = rwyno;
    if ( runwayno.length() ) {
        // standardize input number
        string tmp = runwayno.substr(1, 1);
        if (( tmp == "L" || tmp == "R" || tmp == "C" )
            || (runwayno.size() == 1))
        {
            tmp = runwayno;
            runwayno = "0" + tmp;
            SG_LOG( SG_GENERAL, SG_INFO, "Standardising rwy number from "
                    << tmp << " to " << runwayno );
        }
        revrwyno = GetReverseRunwayNo(runwayno);
    }

    runway_map_iterator pos;
    for ( pos = runways.lower_bound( aptid );
          pos != runways.upper_bound( aptid ); ++pos)
    {
        if ( pos->second.rwy_no == runwayno ) {
            current = pos;
            *r = pos->second;
            return true;
        } else if ( pos->second.rwy_no == revrwyno ) {
            // Search again with the other-end runway number.
            // Remember we have to munge the heading and rwy_no
            // results if this one matches
            current = pos;
            *r = pos->second;
            // NOTE - matching revrwyno implies that runwayno was
            // actually correct.
            r->rwy_no = runwayno;
            r->heading += 180.0;
            string tmp = r->end1_flags;
            r->end1_flags = r->end2_flags;
            r->end2_flags = tmp;
            return true;
        }
    }

    return false;
}


// (wierd!)
FGRunway FGRunwayList::search( const string& aptid ) {
    FGRunway a;
    search( aptid, &a );
    return a;
}


// Return the runway closest to a given heading
bool FGRunwayList::search( const string& aptid, const int tgt_hdg,
                           FGRunway *runway )
{
    string rwyNo = search(aptid, tgt_hdg);
    return(rwyNo == "NN" ? false : search(aptid, rwyNo, runway));
}


// Return the runway number of the runway closest to a given heading
string FGRunwayList::search( const string& aptid, const int tgt_hdg ) {
    FGRunway r;
    FGRunway tmp_r;	
    string rn;
    double found_dir = 0.0;  
 
    if ( !search( aptid, &tmp_r ) ) {
	SG_LOG( SG_GENERAL, SG_ALERT,
                "Failed to find " << aptid << " in database." );
	return "NN";
    }
    
    double diff;
    double min_diff = 360.0;
    
    while ( tmp_r.id == aptid ) {
	r = tmp_r;
	
	// forward direction
	diff = tgt_hdg - r.heading;
	while ( diff < -180.0 ) { diff += 360.0; }
	while ( diff >  180.0 ) { diff -= 360.0; }
	diff = fabs(diff);
        // SG_LOG( SG_GENERAL, SG_INFO,
        //	   "Runway " << r.rwy_no << " heading = " << r.heading <<
        //	   " diff = " << diff );
	if ( diff < min_diff ) {
	    min_diff = diff;
	    rn = r.rwy_no;
	    found_dir = 0;
	}
	
	// reverse direction
	diff = tgt_hdg - r.heading - 180.0;
	while ( diff < -180.0 ) { diff += 360.0; }
	while ( diff >  180.0 ) { diff -= 360.0; }
	diff = fabs(diff);
        // SG_LOG( SG_GENERAL, SG_INFO,
        //	   "Runway -" << r.rwy_no << " heading = " <<
        //	   r.heading + 180.0 <<
        //	   " diff = " << diff );
	if ( diff < min_diff ) {
	    min_diff = diff;
	    rn = r.rwy_no;
	    found_dir = 180.0;
	}
	
	next( &tmp_r );
    }
    
    // SG_LOG( SG_GENERAL, SG_INFO, "closest runway = " << r.rwy_no
    //	       << " + " << found_dir );
    rn = r.rwy_no;
    // cout << "In search, rn = " << rn << endl;
    if ( found_dir == 180 ) {
	rn = GetReverseRunwayNo(rn);
	//cout << "New rn = " << rn << '\n';
    }	
    
    return rn;
}


bool FGRunwayList::next( FGRunway* runway ) {
    ++current;
    if ( current != runways.end() ) {
        *runway = current->second;
        return true;
    } else {
        return false;
    }
}


FGRunway FGRunwayList::next() {
    FGRunway result;

    ++current;
    if ( current != runways.end() ) {
        result = current->second;
    }

    return result;
}


// Destructor
FGRunwayList::~FGRunwayList( void ) {
}
