// runways.hxx -- a simple class to manage airport runway info
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
#include STL_FUNCTIONAL
#include STL_ALGORITHM

#include "runways.hxx"

SG_USING_NAMESPACE(std);

#ifndef _MSC_VER
#define NDEBUG			// MSVC needs this
#endif // !_MSC_VER

#include <mk4.h>
#include <mk4str.h>

#ifndef _MSC_VER
#undef NDEBUG
#endif // !_MSC_VER

#ifdef SG_HAVE_STD_INCLUDES
#  include <istream>
#elif defined( SG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#elif defined( __BORLANDC__ ) || defined (__APPLE__)
#  include <iostream>
#else
#  include <istream.h>
#endif

#if ! defined( SG_HAVE_NATIVE_SGI_COMPILERS )
SG_USING_STD(istream);
#endif

inline istream&
operator >> ( istream& in, FGRunway& a )
{
    int tmp;

    return in >> a.rwy_no >> a.lat >> a.lon >> a.heading >> a.length >> a.width
	      >> a.surface_flags >> a.end1_flags >> tmp >> tmp >> a.end2_flags
	      >> tmp >> tmp;
}


FGRunways::FGRunways( const string& file ) {
    // open the specified database readonly
    storage = new c4_Storage( file.c_str(), false );

    if ( !storage->Strategy().IsValid() ) {
	SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
	exit(-1);
    }

    vRunway = new c4_View;
    *vRunway = 
	storage->GetAs("runway[ID:S,Rwy:S,Longitude:F,Latitude:F,Heading:F,Length:F,Width:F,SurfaceFlags:S,End1Flags:S,End2Flags:S]");

    next_index = 0;
}


// search for the specified apt id
bool FGRunways::search( const string& aptid, FGRunway* r ) {
    c4_StringProp pID ("ID");
    c4_StringProp pRwy ("Rwy");
    c4_FloatProp pLon ("Longitude");
    c4_FloatProp pLat ("Latitude");
    c4_FloatProp pHdg ("Heading");
    c4_FloatProp pLen ("Length");
    c4_FloatProp pWid ("Width");
    c4_StringProp pSurf ("SurfaceFlags");
    c4_StringProp pEnd1 ("End1Flags");
    c4_StringProp pEnd2 ("End2Flags");

    int index = vRunway->Find(pID[aptid.c_str()]);
    // cout << "index = " << index << endl;

    if ( index == -1 ) {
	return false;
    }

    next_index = index + 1;

    c4_RowRef row = vRunway->GetAt(index);

    r->id =      (const char *) pID(row);
    r->rwy_no =  (const char *) pRwy(row);
    r->lon =     (double) pLon(row);
    r->lat =     (double) pLat(row);
    r->heading = (double) pHdg(row);
    r->length =  (double) pLen(row);
    r->width =   (double) pWid(row);
    r->surface_flags = (const char *) pSurf(row);
    r->end1_flags =    (const char *) pEnd1(row);
    r->end2_flags =    (const char *) pEnd2(row);

    return true;
}


// search for the specified apt id and runway no
bool FGRunways::search( const string& aptid, const string& rwyno, FGRunway* r )
{
    c4_StringProp pID ("ID");
    c4_StringProp pRwy ("Rwy");
    c4_FloatProp pLon ("Longitude");
    c4_FloatProp pLat ("Latitude");
    c4_FloatProp pHdg ("Heading");
    c4_FloatProp pLen ("Length");
    c4_FloatProp pWid ("Width");
    c4_StringProp pSurf ("SurfaceFlags");
    c4_StringProp pEnd1 ("End1Flags");
    c4_StringProp pEnd2 ("End2Flags");

    int index = vRunway->Find(pID[aptid.c_str()]);
    // cout << "index = " << index << endl;

    if ( index == -1 ) {
	return false;
    }

    c4_RowRef row = vRunway->GetAt(index);
    string rowid = (const char *) pID(row);
    string rowrwyno = (const char *) pRwy(row);
    while ( rowid == aptid ) {
        next_index = index + 1;

        if ( rowrwyno == rwyno ) {
            r->id =      (const char *) pID(row);
            r->rwy_no =  (const char *) pRwy(row);
            r->lon =     (double) pLon(row);
            r->lat =     (double) pLat(row);
            r->heading = (double) pHdg(row);
            r->length =  (double) pLen(row);
            r->width =   (double) pWid(row);
            r->surface_flags = (const char *) pSurf(row);
            r->end1_flags =    (const char *) pEnd1(row);
            r->end2_flags =    (const char *) pEnd2(row);

            return true;
        }

        index++;
        c4_RowRef row = vRunway->GetAt(index);
        string rowid = (const char *) pID(row);
        string rowrwyno = (const char *) pRwy(row);
    }

    return false;
}


FGRunway FGRunways::search( const string& aptid ) {
    FGRunway a;
    search( aptid, &a );
    return a;
}


// search for the specified id
bool FGRunways::next( FGRunway* r ) {
    c4_StringProp pID ("ID");
    c4_StringProp pRwy ("Rwy");
    c4_FloatProp pLon ("Longitude");
    c4_FloatProp pLat ("Latitude");
    c4_FloatProp pHdg ("Heading");
    c4_FloatProp pLen ("Length");
    c4_FloatProp pWid ("Width");
    c4_StringProp pSurf ("SurfaceFlags");
    c4_StringProp pEnd1 ("End1Flags");
    c4_StringProp pEnd2 ("End2Flags");

    int size = vRunway->GetSize();
    // cout << "total records = " << size << endl;

    int index = next_index;
    // cout << "index = " << index << endl;

    if ( index == -1 || index >= size ) {
	return false;
    }

    next_index = index + 1;

    c4_RowRef row = vRunway->GetAt(index);

    r->id =      (const char *) pID(row);
    r->rwy_no =  (const char *) pRwy(row);
    r->lon =     (double) pLon(row);
    r->lat =     (double) pLat(row);
    r->heading = (double) pHdg(row);
    r->length =  (double) pLen(row);
    r->width =   (double) pWid(row);
    r->surface_flags = (const char *) pSurf(row);
    r->end1_flags =    (const char *) pEnd1(row);
    r->end2_flags =    (const char *) pEnd2(row);

    return true;
}


// Return the runway closest to a given heading
bool FGRunways::search( const string& aptid, const int tgt_hdg,
                        FGRunway* runway )
{
    FGRunway r;
    double found_dir = 0.0;  
 
    if ( !search( aptid, &r ) ) {
	SG_LOG( SG_GENERAL, SG_ALERT,
                "Failed to find " << aptid << " in database." );
	return false;
    }
    
    double diff;
    double min_diff = 360.0;
    
    while ( r.id == aptid ) {
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
	    runway = &r;
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
	    runway = &r;
	    found_dir = 180.0;
	}
	
	next( &r );
    }
    
    // SG_LOG( SG_GENERAL, SG_INFO, "closest runway = " << runway->rwy_no
    //	       << " + " << found_dir );
    
    double heading = runway->heading + found_dir;
    while ( heading >= 360.0 ) { heading -= 360.0; }
    runway->heading = heading;

    return true;
}


// Return the runway number of the runway closest to a given heading
string FGRunways::search( const string& aptid, const int tgt_hdg ) {
    FGRunway r;
    string rn;
    double found_dir = 0.0;  
 
    if ( !search( aptid, &r ) ) {
	SG_LOG( SG_GENERAL, SG_ALERT,
                "Failed to find " << aptid << " in database." );
	return "NN";
    }
    
    double diff;
    double min_diff = 360.0;
    
    while ( r.id == aptid ) {
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
	
	next( &r );
    }
    
    // SG_LOG( SG_GENERAL, SG_INFO, "closest runway = " << r.rwy_no
    //	       << " + " << found_dir );
    // rn = r.rwy_no;
    // cout << "In search, rn = " << rn << endl;
    if ( found_dir == 180 ) {
	int irn = atoi(rn.c_str());
	irn += 18;
	if ( irn > 36 ) {
	    irn -= 36;
	}
	char buf[4];            // 2 chars + string terminator + 1 for safety
	sprintf(buf, "%i", irn);
	rn = buf;
    }	
    
    return rn;
}


// Destructor
FGRunways::~FGRunways( void ) {
    delete storage;
}


// Constructor
FGRunwaysUtil::FGRunwaysUtil() {
}


// load the data
int FGRunwaysUtil::load( const string& file ) {
    FGRunway r;
    string apt_id;

    runways.erase( runways.begin(), runways.end() );

    sg_gzifstream in( file );
    if ( !in.is_open() ) {
	SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << file );
	exit(-1);
    }

    // skip first line of file
    char tmp[2048];
    in.getline( tmp, 2048 );

    // read in each line of the file

#ifdef __MWERKS__

    in >> ::skipws;
    char c = 0;
    while ( in.get(c) && c != '\0' ) {
	if ( c == 'A' ) {
	    in >> apt_id;
	    in >> skipeol;
	} else if ( c == 'R' ) {
	    in >> r;
	    r.id = apt_id;
	    runways.push_back(r);
	} else {
	    in >> skipeol;
	}
	in >> ::skipws;
    }

#else

    in >> ::skipws;
    while ( ! in.eof() ) {
	char c = 0;
 	in.get(c);
	if ( c == 'A' ) {
	    in >> apt_id;
	    in >> skipeol;
	} else if ( c == 'R' ) {
	    in >> r;
	    r.id = apt_id;
	    // cout << apt_id << " " << r.rwy_no << endl;
	    runways.push_back(r);
	} else {
	    in >> skipeol;
	}
	in >> ::skipws;
    }

#endif

    return 1;
}


// save the data in gdbm format
bool FGRunwaysUtil::dump_mk4( const string& file ) {
    // open database for writing
    c4_Storage storage( file.c_str(), true );

    // need to do something about error handling here!

    // define the properties
    c4_StringProp pID ("ID");
    c4_StringProp pRwy ("Rwy");
    c4_FloatProp pLon ("Longitude");
    c4_FloatProp pLat ("Latitude");
    c4_FloatProp pHdg ("Heading");
    c4_FloatProp pLen ("Length");
    c4_FloatProp pWid ("Width");
    c4_StringProp pSurf ("SurfaceFlags");
    c4_StringProp pEnd1 ("End1Flags");
    c4_StringProp pEnd2 ("End2Flags");

    // Start with an empty view of the proper structure.
    c4_View vRunway =
	storage.GetAs("runway[ID:S,Rwy:S,Longitude:F,Latitude:F,Heading:F,Length:F,Width:F,SurfaceFlags:S,End1Flags:S,End2Flags:S]");

    c4_Row row;

    iterator current = runways.begin();
    const_iterator end = runways.end();
    while ( current != end ) {
	// add each runway record
	pID (row) = current->id.c_str();
	pRwy (row) = current->rwy_no.c_str();
	pLon (row) = current->lon;
	pLat (row) = current->lat;
	pHdg (row) = current->heading;
	pLen (row) = current->length;
	pWid (row) = current->width;
	pSurf (row) = current->surface_flags.c_str();
	pEnd1 (row) = current->end1_flags.c_str();
	pEnd2 (row) = current->end2_flags.c_str();
	vRunway.Add(row);

	++current;
    }

    // commit our changes
    storage.Commit();

    return true;
}


#if 0
// search for the specified id
bool
FGRunwaysUtil::search( const string& id, FGRunway* a ) const
{
    const_iterator it = runways.find( FGRunway(id) );
    if ( it != runways.end() )
    {
	*a = *it;
	return true;
    }
    else
    {
	return false;
    }
}


FGRunway
FGRunwaysUtil::search( const string& id ) const
{
    FGRunway a;
    this->search( id, &a );
    return a;
}
#endif

// Destructor
FGRunwaysUtil::~FGRunwaysUtil( void ) {
}


