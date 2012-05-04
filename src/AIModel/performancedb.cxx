#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/xml/easyxml.hxx>

#include <Main/globals.hxx>
#include <iostream>
#include <fstream>

#include "performancedb.hxx"

using std::string;
using std::cerr;

PerformanceDB::PerformanceDB()
{
    SGPath dbpath( globals->get_fg_root() );
    

    dbpath.append( "/AI/Aircraft/" );
    dbpath.append( "performancedb.xml"); 
    load(dbpath);
}


PerformanceDB::~PerformanceDB()
{}

void PerformanceDB::registerPerformanceData(const std::string& id, PerformanceData* data) {
    //TODO if key exists already replace data "inplace", i.e. copy to existing PerfData instance
    // this updates all aircraft currently using the PerfData instance.
    _db[id] = data;
}

void PerformanceDB::registerPerformanceData(const std::string& id, const std::string& filename) {
    registerPerformanceData(id, new PerformanceData(filename));
}

PerformanceData* PerformanceDB::getDataFor(const std::string& id) {
    if (_db.find(id) == _db.end()) // id not found -> return jet_transport data
        return _db["jet_transport"];

    return _db[id];
}

void PerformanceDB::load(SGPath filename) {
    string name;
    double acceleration;
    double deceleration;
    double climbRate;
    double descentRate;
    double vRotate;
    double vTakeOff;
    double vClimb;
    double vCruise;
    double vDescent;
    double vApproach;
    double vTouchdown;
    double vTaxi;
    SGPropertyNode root;
    try {
        readProperties(filename.str(), &root);
    } catch (const sg_exception &) {
        SG_LOG(SG_AI, SG_ALERT,
            "Error reading AI aircraft performance database: " << filename.str());
        return;
    }

    SGPropertyNode * node = root.getNode("performancedb");
    for (int i = 0; i < node->nChildren(); i++) { 
        SGPropertyNode * db_node = node->getChild(i);
            name         = db_node->getStringValue("type", "heavy_jet");
            acceleration = db_node->getDoubleValue("acceleration-kts-hour", 4.0);
            deceleration = db_node->getDoubleValue("deceleration-kts-hour", 2.0);
            climbRate    = db_node->getDoubleValue("climbrate-fpm", 3000.0);
            descentRate  = db_node->getDoubleValue("decentrate-fpm", 1500.0);
            vRotate      = db_node->getDoubleValue("rotate-speed-kts", 150.0);
            vTakeOff     = db_node->getDoubleValue("takeoff-speed-kts", 160.0);
            vClimb       = db_node->getDoubleValue("climb-speed-kts", 300.0);
            vCruise      = db_node->getDoubleValue("cruise-speed-kts", 430.0);
            vDescent     = db_node->getDoubleValue("decent-speed-kts", 300.0);
            vApproach    = db_node->getDoubleValue("approach-speed-kts", 170.0);
            vTouchdown   = db_node->getDoubleValue("touchdown-speed-kts", 150.0);
            vTaxi        = db_node->getDoubleValue("taxi-speed-kts", 15.0);

            registerPerformanceData(name, new PerformanceData(
                acceleration, deceleration, climbRate, descentRate, vRotate, vTakeOff, vClimb, vCruise, vDescent, vApproach, vTouchdown, vTaxi));
    }
}

