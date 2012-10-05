#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "performancedb.hxx"

#include <boost/foreach.hpp>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/xml/easyxml.hxx>

#include <Main/globals.hxx>
#include <iostream>
#include <fstream>

#include "performancedata.hxx"

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

PerformanceData* PerformanceDB::getDataFor(const string& acType, const string& acClass)
{
  // first, try with the specific aircraft type, such as 738 or A322
    if (_db.find(acType) != _db.end()) {
        return _db[acType];
    }
    
    string alias = findAlias(acType);
    if (_db.find(alias) != _db.end()) {
      return _db[alias];
    }
  
    SG_LOG(SG_AI, SG_INFO, "no performance data for " << acType);
  
    if (_db.find(acClass) == _db.end()) {
        return _db["jet_transport"];
    }
  
    return _db[acClass];
}

void PerformanceDB::load(const SGPath& filename) {
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
        if (!strcmp(db_node->getName(), "aircraft")) {
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
        } else if (!strcmp(db_node->getName(), "alias")) {
            string alias(db_node->getStringValue("alias"));
            if (alias.empty()) {
                SG_LOG(SG_AI, SG_ALERT, "performance DB alias entry with no <alias> definition");
                continue;
            }
          
            BOOST_FOREACH(SGPropertyNode* matchNode, db_node->getChildren("match")) {
                string match(matchNode->getStringValue());
                _aliases.push_back(StringPair(match, alias));
            }
        } else {
            SG_LOG(SG_AI, SG_ALERT, "unrecognized performance DB entry:" << db_node->getName());
        }
    } // of nodes iteration
}

string PerformanceDB::findAlias(const string& acType) const
{
    BOOST_FOREACH(const StringPair& alias, _aliases) {
        if (acType.find(alias.first) == 0) { // matched!
            return alias.second;
        }
    } // of alias iteration
  
    return string();
}


