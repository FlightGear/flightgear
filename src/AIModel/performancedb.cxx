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

PerformanceData* PerformanceDB::getDataFor(const string& acType, const string& acClass)
{
  // first, try with the specific aircraft type, such as 738 or A322
    if (_db.find(acType) != _db.end()) {
        return _db[acType];
    }
    
    const string& alias = findAlias(acType);
    if (_db.find(alias) != _db.end()) {
      return _db[alias];
    }
  
    SG_LOG(SG_AI, SG_INFO, "no performance data for " << acType);
  
    if (_db.find(acClass) == _db.end()) {
        return _db["jet_transport"];
    }
  
    return _db[acClass];
}

void PerformanceDB::load(const SGPath& filename)
{
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
            PerformanceData* data = NULL;
            if (db_node->hasChild("base")) {
              const string& baseName = db_node->getStringValue("base");
              PerformanceData* baseData = _db[baseName];
              if (!baseData) {
                SG_LOG(SG_AI, SG_ALERT,
                       "Error reading AI aircraft performance database: unknown base type " << baseName);
                return;
              }
              
              // clone base data to 'inherit' from it
              data = new PerformanceData(baseData); 
            } else {
              data = new PerformanceData;
            }
          
            data->initFromProps(db_node);
            const string& name  = db_node->getStringValue("type", "heavy_jet");
            registerPerformanceData(name, data);
        } else if (!strcmp(db_node->getName(), "alias")) {
            const string& alias(db_node->getStringValue("alias"));
            if (alias.empty()) {
                SG_LOG(SG_AI, SG_ALERT, "performance DB alias entry with no <alias> definition");
                continue;
            }
          
            BOOST_FOREACH(SGPropertyNode* matchNode, db_node->getChildren("match")) {
                const string& match(matchNode->getStringValue());
                _aliases.push_back(StringPair(match, alias));
            }
        } else {
            SG_LOG(SG_AI, SG_ALERT, "unrecognized performance DB entry:" << db_node->getName());
        }
    } // of nodes iteration
}

const string& PerformanceDB::findAlias(const string& acType) const
{
    BOOST_FOREACH(const StringPair& alias, _aliases) {
        if (acType.find(alias.first) == 0) { // matched!
            return alias.second;
        }
    } // of alias iteration
  
    static const string empty;
    return empty;
}


