// FGGround - a class to provide ground control at larger airports.
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C. Luff - david.luff@nottingham.ac.uk
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

#ifndef _FG_GROUND_HXX
#define _FG_GROUND_HXX

#include <vector>
#include <list>
//#include <map>

SG_USING_STD(vector);
SG_USING_STD(list);
//SG_USING_STD(map);

//////////////////////////////////////////////////////
// Types for the logical network data structure
typedef enum arc_type {
    RUNWAY,
    TAXIWAY
};

typedef enum node_type {
    GATE,
    APRON,
    HOLD
};

typedef struct arc {
    int distance;
    char* name;
    arc_type type;
};

typedef list<arc> arc_list_type;
typedef arc_list_type::iterator arc_list_itr;
typedef arc_list_type::const_iterator arc_list_const_itr; 

typedef struct node {
    point pos;
    char* name;
    node_type node;
    arc_list arcs;
};

typedef vector<node> node_array_type;
typedef node_array_type::iterator node_array_itr;
typedef node_array_type::const_iterator node_array_const_itr;
// end logical network types
///////////////////////////////////////////////////////

// somewhere in the ATC/AI system we are going to have defined something like
// typedef struct plane_rec
// list <PlaneRec> plane_rec_list_type

// A more specialist plane rec to include ground information
typedef struct ground_rec {
    plane_rec plane;
    point current_pos;
    node destination;
    node last_clearance;
    bool cleared;  // set true when the plane has been cleared to somewhere
    bool incoming; //true for arrivals, false for departures
    // status?
    // Almost certainly need to add more here
};

typedef list<ground_rec*> ground_rec_list;
typedef ground_rec_list::iterator ground_rec_list_itr;
typedef ground_rec_list::const_iterator ground_rec_list_const_itr;

///////////////////////////////////////////////////////////////////////////////
//
// FGGround
//
///////////////////////////////////////////////////////////////////////////////
class FGGround : public FGATC {

private:

    // Need a data structure to hold details of the various active planes
    // Need a data structure to hold details of the logical network
    // including which gates are filled - or possibly another data structure
    // with the positions of the inactive planes.
    // Need a data structure to hold outstanding communications from aircraft.
    // Possibly need a data structure to hold outstanding communications to aircraft.

    // logical network
    node_array_type network;

    // Planes currently active
    ground_rec_list ground_traffic;

public:

    void Init();

    void Update();

    inline void SetDisplay() {display = true;}
    inline void SetNoDisplay() {display = false;}

    // Its possible that NewArrival and NewDeparture should simply be rolled into Request.

    // Contact ground control on arrival, assumed to request any gate
    void NewArrival(plane_rec plane);

    // Contact ground control on departure, assumed to request currently active runway.
    void NewDeparture(plane_rec plane);

    // Contact ground control when the calling routine doesn't know if arrival
    // or departure is appropriate.
    void NewContact(plane_rec plane);

    // Make a request of ground control.
    void Request(ground_request request);

private:

    // Find the shortest route through the logical network between two points.
    FindShortestRoute(point a, point b);

    // Project a point in WGS84 lat/lon onto the local gnomonic.
    ConvertWGS84ToXY(sgVec3 wgs84, point xy);

    // Assign a gate or parking location to a new arrival
    AssignGate(ground_rec &g);

    // Generate the next clearance for an airplane
    NextClearance(ground_rec &g);

};

#endif  //_FG_GROUND_HXX