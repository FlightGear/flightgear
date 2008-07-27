// AIMgr.hxx - definition of FGAIMgr 
// - a global management class for FlightGear generated AI traffic
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C Luff - david.luff@nottingham.ac.uk
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

#ifndef _FG_AIMGR_HXX
#define _FG_AIMGR_HXX

#include <simgear/structure/subsystem_mgr.hxx>

#include <Main/fg_props.hxx>

#include <list>

#include "ATCmgr.hxx"
#include "AIEntity.hxx"

using std::list;


class FGAIMgr : public SGSubsystem
{

private:
	FGATCMgr* ATC;	
	// This is purely for synactic convienience to avoid writing globals->get_ATC_mgr()-> all through the code!

    // A list of pointers to all currently active AI stuff
    typedef list <FGAIEntity*> ai_list_type;
    typedef ai_list_type::iterator ai_list_iterator;
    typedef ai_list_type::const_iterator ai_list_const_iterator;

    // Everything put in this list should be created dynamically
    // on the heap and ***DELETED WHEN REMOVED!!!!!***
    ai_list_type ai_list;
    ai_list_iterator ai_list_itr;
    // Any member function of FGATCMgr is permitted to leave this iterator pointing
    // at any point in or at the end of the list.
    // Hence any new access must explicitly first check for atc_list.end() before dereferencing.
	
	// A list of airport or airplane ID's
	typedef list < string > ID_list_type;
	typedef ID_list_type::iterator ID_list_iterator;

	// Temporary storage of ID of planes scheduled for removeal
	ID_list_type removalList;
	
	// A map of airport-IDs that have taxiway network files against bucket number
	typedef map < int, ID_list_type* > ai_apt_map_type;
	typedef ai_apt_map_type::iterator ai_apt_map_iterator;
	ai_apt_map_type facilities;
	
	// A map of airport ID's that we've activated AI traffic at
	typedef map < string, int > ai_activated_map_type;
	typedef ai_activated_map_type::iterator ai_activated_map_iterator;
	ai_activated_map_type activated;
	
	// AI traffic lists mapped by airport
	typedef map < string, ai_list_type > ai_traffic_map_type;
	typedef ai_traffic_map_type::iterator ai_traffic_map_iterator;
	ai_traffic_map_type traffic;
	
	// A map of callsigns that we have used (eg CFGFS or N0546D - the code will generate Cessna-four-six-delta from this later)
	typedef map < string, int > ai_callsigns_map_type;
	typedef ai_callsigns_map_type::iterator ai_callsigns_map_iterator;
	ai_callsigns_map_type ai_callsigns_used;

    // Position of the Users Aircraft
    double lon;
    double lat;
    double elev;
    // Pointers to current users position
    SGPropertyNode_ptr lon_node;
    SGPropertyNode_ptr lat_node;
    SGPropertyNode_ptr elev_node;

public:

    FGAIMgr();
    ~FGAIMgr();

    void init();

    void bind();

    void unbind();

    void update(double dt);
	
	// Signal that it is OK to remove a plane of callsign s
	// (To be called by the plane itself).
	void ScheduleRemoval(const string& s);

private:
	
        osg::ref_ptr<osg::Node> _defaultModel;  // Cessna 172!
	osg::ref_ptr<osg::Node> _piperModel;    // pa28-161

	bool initDone;	// Hack - guard against update getting called before init

    // Remove a class from the ai_list and delete it from memory
    //void RemoveFromList(const char* id, atc_type tp);
	
	// Activate AI traffic at an airport
	void ActivateAirport(const string& ident);
	
	// Hack - Generate AI traffic at an airport with no facilities file, with the first plane being at least min_dist out.
	void GenerateSimpleAirportTraffic(const string& ident, double min_dist = 0.0);
	
	// Search for valid airports in the vicinity of the user and activate them if necessary
	void SearchByPos(double range);
	
	string GenerateCallsign();
	
	string GenerateUniqueCallsign();
	
	string GenerateShortForm(const string& callsign, const string& plane_str = "Cessna-", bool local = false);
	
	// TODO - implement a proper robust system for registering and loading AI GA aircraft models
	bool _havePiperModel;
};

#endif  // _FG_AIMGR_HXX
