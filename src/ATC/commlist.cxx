// commlist.cxx -- comm frequency lookup class
//
// Written by David Luff and Alexander Kappes, started Jan 2003.
// Based on navlist.cxx by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/bucket/newbucket.hxx>

#include "commlist.hxx"
//#include "atislist.hxx"


FGCommList *current_commlist;


// Constructor
FGCommList::FGCommList( void ) {
}


// Destructor
FGCommList::~FGCommList( void ) {
}


// load the navaids and build the map
bool FGCommList::init( SGPath path ) {

	SGPath temp = path;
    commlist_freq.erase(commlist_freq.begin(), commlist_freq.end());
	commlist_bck.erase(commlist_bck.begin(), commlist_bck.end());
    temp.append( "ATC/default.atis" );
	LoadComms(temp);
	temp = path;
	temp.append( "ATC/default.tower" );
	LoadComms(temp);
	temp = path;
	temp.append( "ATC/default.approach" );
	LoadComms(temp);
	return true;
}
	

bool FGCommList::LoadComms(SGPath path) {

    sg_gzifstream fin( path.str() );
    if ( !fin.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }
	
    // read in each line of the file
    fin >> skipcomment;

#ifdef __MWERKS__
    char c = 0;
    while ( fin.get(c) && c != '\0' ) {
        fin.putback(c);
#else
    while ( !fin.eof() ) {
#endif
        ATCData a;
		fin >> a;
		if(a.type == INVALID) {
            break;
        }
		
		// Push all stations onto frequency map
        commlist_freq[a.freq].push_back(a);
		
		// Push approach stations onto bucket map as well
		if(a.type == APPROACH) {
			// get bucket number
    		SGBucket bucket(a.lon, a.lat);
    		int bucknum = bucket.gen_index();
			commlist_bck[bucknum].push_back(a);
		}
		
        fin >> skipcomment;
    }
    return true;	
}


// query the database for the specified frequency, lon and lat are in
// degrees, elev is in meters
// If no atc_type is specified, it returns true if any non-invalid type is found
// If atc_type is specifed, returns true only if the specified type is found
bool FGCommList::FindByFreq( double lon, double lat, double elev, double freq,
						ATCData* ad, atc_type tp )
{
	lon *= SGD_DEGREES_TO_RADIANS;
	lat *= SGD_DEGREES_TO_RADIANS;
	
	comm_list_type stations = commlist_freq[(int)(freq*100.0 + 0.5)];
	comm_list_iterator current = stations.begin();
	comm_list_iterator last = stations.end();
	
	// double az1, az2, s;
	Point3D aircraft = sgGeodToCart( Point3D(lon, lat, elev) );
	Point3D station;
	double d;
	// TODO - at the moment this loop returns the first match found in range
	// We want to return the closest match in the event of a frequency conflict
	for ( ; current != last ; ++current ) {
		//cout << "testing " << current->get_ident() << endl;
		station = Point3D(current->x, current->y, current->z);
		//cout << "aircraft = " << aircraft << endl;
		//cout << "station = " << station << endl;
		
		d = aircraft.distance3Dsquared( station );
		
		//cout << "  dist = " << sqrt(d)
		//     << "  range = " << current->get_range() * SG_NM_TO_METER << endl;
		
		// match up to twice the published range so we can model
		// reduced signal strength
		if ( d < (2 * current->range * SG_NM_TO_METER 
		* 2 * current->range * SG_NM_TO_METER ) ) {
			//cout << "matched = " << current->get_ident() << endl;
			if((tp == INVALID) || (tp == (*current).type)) {
				*ad = *current;
				return true;
			}
		}
	}
	
	return false;
}


int FGCommList::FindByPos(double lon, double lat, double elev, comm_list_type* stations, atc_type tp)
{
	// number of relevant stations found within range
	int found = 0;
	stations->erase(stations->begin(), stations->end());
	
	// get bucket number for plane position
	SGBucket buck(lon, lat);

	// get neigboring buckets
	int max_range = 100;
	int bx = (int)( max_range*SG_NM_TO_METER / buck.get_width_m() / 2);
	int by = (int)( max_range*SG_NM_TO_METER / buck.get_height_m() / 2 );
	
	// loop over bucket range 
	for ( int i=-bx; i<bx; i++) {
		for ( int j=-by; j<by; j++) {
			buck = sgBucketOffset(lon, lat, i, j);
			long int bucket = buck.gen_index();
			comm_list_type Fstations = commlist_bck[bucket];
			comm_list_iterator current = Fstations.begin();
			comm_list_iterator last = Fstations.end();
			
			double rlon = lon * SGD_DEGREES_TO_RADIANS;
			double rlat = lat * SGD_DEGREES_TO_RADIANS;
			
			// double az1, az2, s;
			Point3D aircraft = sgGeodToCart( Point3D(rlon, rlat, elev) );
			Point3D station;
			double d;
			for(; current != last; ++current) {
				if((current->type == tp) || (tp == INVALID)) {
					station = Point3D(current->x, current->y, current->z);
					d = aircraft.distance3Dsquared( station );
					if ( d < (current->range * SG_NM_TO_METER 
					* current->range * SG_NM_TO_METER ) ) {
						stations->push_back(*current);
						++found;
					}
				}
			}
		}
	}
	return found;
}


// TODO - this function should move somewhere else eventually!
// Return an appropriate call-sign for an ATIS transmission.
int FGCommList::GetCallSign( string apt_id, int hours, int mins )
{
	atis_transmission_type tran;
	
	if(atislog.find(apt_id) == atislog.end()) {
		// This station has not transmitted yet - return a random identifier
		// and add the transmission to the log
		tran.hours = hours;
		tran.mins = mins;
		sg_srandom_time();
		tran.callsign = int(sg_random() * 25) + 1;	// This *should* give a random int between 1 and 26
		//atislog[apt_id].push_back(tran);
		atislog[apt_id] = tran;
	} else {
		// This station has transmitted - calculate the appropriate identifier
		// and add the transmission to the log if it has changed
		tran = atislog[apt_id];
		// This next bit assumes that no-one comes back to the same ATIS station
		// after running FlightGear for more than 24 hours !!
		if((tran.hours == hours) && (tran.mins == mins)) {
			return(tran.callsign);
		} else {
			if(tran.hours == hours) {
				// The minutes must have changed
				tran.mins = mins;
				tran.callsign++;
			} else {
				if(hours < tran.hours) {
					hours += 24;
				}
				tran.callsign += (hours - tran.hours);
				if(mins != 0) {
					// Assume transmissions were made on every hour
					tran.callsign++;
				}
				tran.hours = hours;
				tran.mins = mins;
			}
			// Wrap if we've exceeded Zulu
			if(tran.callsign > 26) {
				tran.callsign -= 26;
			}
			// And write the new transmission to the log
			atislog[apt_id] = tran;
		}
	}
	return(tran.callsign);
}
