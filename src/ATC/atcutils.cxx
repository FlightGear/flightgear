// commlist.cxx -- comm frequency lookup class
//
// Written by David Luff and Alexander Kappes, started Jan 2003.
// Based on navlist.cxx by Curtis Olson, started April 2000.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif



#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/bucket/newbucket.hxx>
#include <Airports/simple.hxx>

#include "atcutils.hxx"

#if !ENABLE_ATCDCL


FGCommList *current_commlist;


// Constructor
FGCommList::FGCommList( void ) {
      sg_srandom_time();
}


// Destructor
FGCommList::~FGCommList( void ) {
}


// load the navaids and build the map
bool FGCommList::init( const SGPath& path ) {
/*
    SGPath temp = path;
    commlist_freq.erase(commlist_freq.begin(), commlist_freq.end());
    commlist_bck.erase(commlist_bck.begin(), commlist_bck.end());
    temp.append( "ATC/default.atis" );
    LoadComms(temp);
    temp = path;
    temp.append( "ATC/default.tower" );
    LoadComms(temp);
    temp = path;
    temp.append( "ATC/default.ground" );
    LoadComms(temp);
    temp = path;
    temp.append( "ATC/default.approach" );
    LoadComms(temp);
    return true;*/
}
    

bool FGCommList::LoadComms(const SGPath& path) {
/*
    sg_gzifstream fin( path.str() );
    if ( !fin.is_open() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Cannot open file: " << path.str() );
        exit(-1);
    }
    
    // read in each line of the file
    fin >> skipcomment;

    while ( !fin.eof() ) {
        ATCData a;
        fin >> a;
        if(a.type == INVALID) {
            SG_LOG(SG_GENERAL, SG_DEBUG, "WARNING - INVALID type found in " << path.str() << '\n');
        } else {        
            // Push all stations onto frequency map
            commlist_freq[a.freq].push_back(a);
            
            // Push non-atis stations onto bucket map as well
            // In fact, push all stations onto bucket map for now so FGATCMgr::GetFrequency() works.
            //if(a.type != ATIS and/or AWOS?) {
                // get bucket number
                SGBucket bucket(a.geod);
                int bucknum = bucket.gen_index();
                commlist_bck[bucknum].push_back(a);
            //}
        }
        
        fin >> skipcomment;
    }
    
    fin.close();
    return true;    
*/
}


// query the database for the specified frequency, lon and lat are in
// degrees, elev is in meters
// If no atc_type is specified, it returns true if any non-invalid type is found
// If atc_type is specifed, returns true only if the specified type is found
bool FGCommList::FindByFreq(const SGGeod& aPos, double freq,
						ATCData* ad, atc_type tp )
{
/*
    comm_list_type stations;
    stations = commlist_freq[kHz10(freq)];
    comm_list_iterator current = stations.begin();
    comm_list_iterator last = stations.end();
    
    // double az1, az2, s;
    SGVec3d aircraft = SGVec3d::fromGeod(aPos);
    const double orig_max_d = 1e100; 
    double max_d = orig_max_d;
    double d;
    // TODO - at the moment this loop returns the first match found in range
    // We want to return the closest match in the event of a frequency conflict
    for ( ; current != last ; ++current ) {
        d = distSqr(aircraft, current->cart);
        
        //cout << "  dist = " << sqrt(d)
        //     << "  range = " << current->range * SG_NM_TO_METER << endl;
        
        // TODO - match up to twice the published range so we can model
        // reduced signal strength
        // NOTE The below is squared since we match to distance3Dsquared (above) to avoid a sqrt.
        if ( d < (current->range * SG_NM_TO_METER 
             * current->range * SG_NM_TO_METER ) ) {
            //cout << "matched = " << current->ident << endl;
            if((tp == INVALID) || (tp == (*current).type)) {
                if(d < max_d) {
                    max_d = d;
                    *ad = *current;
                }
            }
        }
    }
    
    if(max_d < orig_max_d) {
        return true;
    } else {
        return false;
    }
*/
}

int FGCommList::FindByPos(const SGGeod& aPos, double range, comm_list_type* stations, atc_type tp)
{
/*
     // number of relevant stations found within range
     int found = 0;
     stations->erase(stations->begin(), stations->end());
     
     // get bucket number for plane position
     SGBucket buck(aPos);
 
     // get neigboring buckets
     int bx = (int)( range*SG_NM_TO_METER / buck.get_width_m() / 2) + 1;
     int by = (int)( range*SG_NM_TO_METER / buck.get_height_m() / 2 ) + 1;
     
     // loop over bucket range 
     for ( int i=-bx; i<=bx; i++) {
         for ( int j=-by; j<=by; j++) {
             buck = sgBucketOffset(aPos.getLongitudeDeg(), aPos.getLatitudeDeg(), i, j);
             long int bucket = buck.gen_index();
             comm_map_const_iterator Fstations = commlist_bck.find(bucket);
             if (Fstations == commlist_bck.end()) continue;
             comm_list_const_iterator current = Fstations->second.begin();
             comm_list_const_iterator last = Fstations->second.end();
             
             
             // double az1, az2, s;
             SGVec3d aircraft = SGVec3d::fromGeod(aPos);
             double d;
             for(; current != last; ++current) {
                 if((current->type == tp) || (tp == INVALID)) {
                     d = distSqr(aircraft, current->cart);
                     // NOTE The below is squared since we match to distance3Dsquared (above) to avoid a sqrt.
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
*/
}


// Returns the distance in meters to the closest station of a given type,
// with the details written into ATCData& ad.  If no type is specifed simply
// returns the distance to the closest station of any type.
// Returns -9999 if no stations found within max_range in nautical miles (default 100 miles).
// Note that the search algorithm starts at 10 miles and multiplies by 10 thereafter, so if
// say 300 miles is specifed 10, then 100, then 1000 will be searched, breaking at first result 
// and giving up after 1000.
double FGCommList::FindClosest(const SGGeod& aPos, ATCData& ad, atc_type tp, double max_range) {
/*
    int num_stations = 0;
    int range = 10;
    comm_list_type stations;
    comm_list_iterator itr;
    double distance = -9999.0;
    
    while(num_stations == 0) {
        num_stations = FindByPos(aPos, range, &stations, tp);
        if(num_stations) {
            double closest = max_range * SG_NM_TO_METER;
            double tmp;
            for(itr = stations.begin(); itr != stations.end(); ++itr) { 
                ATCData ad2 = *itr;    
                const FGAirport *a = fgFindAirportID(ad2.ident);
                if (a) {
                    tmp = dclGetHorizontalSeparation(ad2.geod, aPos);
                    if(tmp <= closest) {
                        closest = tmp;
                        distance = tmp;
                        ad = *itr;
                    }
                }
            }
            //cout << "Closest station is " << ad.ident << " at a range of " << distance << " meters\n";
            return(distance);
        }
        if(range > max_range) {
            break;
        }
        range *= 10;
    }
    return(-9999.0);
*/
}


// Find by Airport code.
// This is basically a wrapper for a call to the airport database to get the airport
// position followed by a call to FindByPos(...)
bool FGCommList::FindByCode( const string& ICAO, ATCData& ad, atc_type tp ) {
/*
    const FGAirport *a = fgFindAirportID( ICAO);
    if ( a) {
        comm_list_type stations;
        int found = FindByPos(a->geod(), 10.0, &stations, tp);
        if(found) {
            comm_list_iterator itr = stations.begin();
            while(itr != stations.end()) {
                if(((*itr).ident == ICAO) && ((*itr).type == tp)) {
                    ad = *itr;
                    //cout << "FindByCode returns " << ICAO
                    //     << "  type: " << tp
                    //   << "  freq: " << itr->freq
                    //   << endl;
                    return true;
                }
                ++itr;
            }
        }
    }
    return false;
*/
}

// TODO - this function should move somewhere else eventually!
// Return an appropriate sequence number for an ATIS transmission.
// Return sequence number + 2600 if sequence is unchanged since 
// last time.
int FGCommList::GetAtisSequence( const string& apt_id, 
        const double tstamp, const int interval, const int special)
{
/*
    atis_transmission_type tran;
    
    if(atislog.find(apt_id) == atislog.end()) { // New station
      tran.tstamp = tstamp - interval;
// Random number between 0 and 25 inclusive, i.e. 26 equiprobable outcomes:
      tran.sequence = int(sg_random() * LTRS);
      atislog[apt_id] = tran;
      //cout << "New ATIS station: " << apt_id << " seq-1: "
      //      << tran.sequence << endl;
    } 

// calculate the appropriate identifier and update the log
    tran = atislog[apt_id];

    int delta = int((tstamp - tran.tstamp) / interval);
    tran.tstamp += delta * interval;
    if (special && !delta) delta++;     // a "special" ATIS update is required
    tran.sequence = (tran.sequence + delta) % LTRS;
    atislog[apt_id] = tran;
    //if (delta) cout << "New ATIS sequence: " << tran.sequence
    //      << "  Delta: " << delta << endl;
    return(tran.sequence + (delta ? 0 : LTRS*1000));
*/
}
/*****************************************************************************
 * FGKln89AlignedProjection 
 ****************************************************************************/

FGKln89AlignedProjection::FGKln89AlignedProjection() {
    _origin.setLatitudeRad(0);
    _origin.setLongitudeRad(0);
    _origin.setElevationM(0);
    _correction_factor = cos(_origin.getLatitudeRad());
}

FGKln89AlignedProjection::FGKln89AlignedProjection(const SGGeod& centre, double heading) {
    _origin = centre;
    _theta = heading * SG_DEGREES_TO_RADIANS;
    _correction_factor = cos(_origin.getLatitudeRad());
}

FGKln89AlignedProjection::~FGKln89AlignedProjection() {
}

void FGKln89AlignedProjection::Init(const SGGeod& centre, double heading) {
    _origin = centre;
    _theta = heading * SG_DEGREES_TO_RADIANS;
    _correction_factor = cos(_origin.getLatitudeRad());
}

SGVec3d FGKln89AlignedProjection::ConvertToLocal(const SGGeod& pt) {
    // convert from lat/lon to orthogonal
    double delta_lat = pt.getLatitudeRad() - _origin.getLatitudeRad();
    double delta_lon = pt.getLongitudeRad() - _origin.getLongitudeRad();
    double y = sin(delta_lat) * SG_EQUATORIAL_RADIUS_M;
    double x = sin(delta_lon) * SG_EQUATORIAL_RADIUS_M * _correction_factor;

    // Align
    if(_theta != 0.0) {
        double xbar = x;
        x = x*cos(_theta) - y*sin(_theta);
        y = (xbar*sin(_theta)) + (y*cos(_theta));
    }

    return SGVec3d(x, y, pt.getElevationM());
}

SGGeod FGKln89AlignedProjection::ConvertFromLocal(const SGVec3d& pt) {
    // de-align
    double thi = _theta * -1.0;
    double x = pt.x()*cos(thi) - pt.y()*sin(thi);
    double y = (pt.x()*sin(thi)) + (pt.y()*cos(thi));

    // convert from orthogonal to lat/lon
    double delta_lat = asin(y / SG_EQUATORIAL_RADIUS_M);
    double delta_lon = asin(x / SG_EQUATORIAL_RADIUS_M) / _correction_factor;

    return SGGeod::fromRadM(_origin.getLongitudeRad()+delta_lon, _origin.getLatitudeRad()+delta_lat, pt.z());
}

#endif // #ENABLE_ATCDCL