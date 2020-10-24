/******************************************************************************
 * TrafficMGr.cxx
 * Written by Durk Talsma, started May 5, 2004.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 **************************************************************************/

/*
 * Traffic manager parses airlines timetable-like data and uses this to
 * determine the approximate position of each AI aircraft in its database.
 * When an AI aircraft is close to the user's position, a more detailed
 * AIModels based simulation is set up.
 *
 * I'm currently assuming the following simplifications:
 * 1) The earth is a perfect sphere
 * 2) Each aircraft flies a perfect great circle route.
 * 3) Each aircraft flies at a constant speed (with infinite accelerations and
 *    decelerations)
 * 4) Each aircraft leaves at exactly the departure time.
 * 5) Each aircraft arrives at exactly the specified arrival time.
 *
 *
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <time.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <mutex>


#include <string>
#include <vector>
#include <algorithm>

#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/timing/sg_time.hxx>

#include <simgear/xml/easyxml.hxx>
#include <simgear/scene/tsync/terrasync.hxx>

#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIBase.hxx>
#include <AIModel/performancedb.hxx>

#include <Airports/airport.hxx>
#include <Main/fg_init.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "TrafficMgr.hxx"

using std::sort;
using std::strcmp;
using std::endl;
using std::string;
using std::vector;

/**
 * Thread encapsulating parsing the traffic schedules.
 */
class ScheduleParseThread : public SGThread, public XMLVisitor
{
public:
  ScheduleParseThread(FGTrafficManager* traffic) :
    _trafficManager(traffic),
    _isFinished(false),
    _cancelThread(false),
    cruiseAlt(0),
    score(0),
    acCounter(0),
    radius(0),
    offset(0),
    heavy(false)
  {

  }

  // if we're destroyed while running, ensure the thread exits cleanly
  ~ScheduleParseThread()
  {
    _lock.lock();
    if (!_isFinished) {
      _cancelThread = true; // request cancellation so we don't wait ages
      _lock.unlock();
      join();
    } else {
      _lock.unlock();
    }
  }

  void setTrafficDirs(const PathList& dirs)
  {
    _trafficDirPaths = dirs;
  }

  bool isFinished() const
  {
    std::lock_guard<std::mutex> g(_lock);
    return _isFinished;
  }

  virtual void run()
  {
      for (const auto& p : _trafficDirPaths) {
          parseTrafficDir(p);
          if (_cancelThread) {
              return;
          }
      }

    std::lock_guard<std::mutex> g(_lock);
    _isFinished = true;
  }

    void startXML()
    {
        //cout << "Start XML" << endl;
        requiredAircraft = "";
        homePort = "";
    }

    void endXML()
    {
        //cout << "End XML" << endl;
    }

    void startElement(const char *name,
                                        const XMLAttributes & atts)
    {
        const char *attval;
        //cout << "Start element " << name << endl;
        //FGTrafficManager temp;
        //for (int i = 0; i < atts.size(); i++)
        //  if (string(atts.getName(i)) == string("include"))
        attval = atts.getValue("include");
        if (attval != 0) {
            //cout << "including " << attval << endl;
            SGPath path = globals->get_fg_root();
            path.append("/Traffic/");
            path.append(attval);
            readXML(path, *this);
        }
        elementValueStack.push_back("");
        //  cout << "  " << atts.getName(i) << '=' << atts.getValue(i) << endl;
    }

    void endElement(const char *name)
    {
        //cout << "End element " << name << endl;
        const string & value = elementValueStack.back();

        if (!strcmp(name, "model"))
            mdl = value;
        else if (!strcmp(name, "livery"))
            livery = value;
        else if (!strcmp(name, "home-port"))
            homePort = value;
        else if (!strcmp(name, "registration"))
            registration = value;
        else if (!strcmp(name, "airline"))
            airline = value;
        else if (!strcmp(name, "actype"))
            acType = value;
        else if (!strcmp(name, "required-aircraft"))
            requiredAircraft = value;
        else if (!strcmp(name, "flighttype"))
            flighttype = value;
        else if (!strcmp(name, "radius"))
            radius = atoi(value.c_str());
        else if (!strcmp(name, "offset"))
            offset = atoi(value.c_str());
        else if (!strcmp(name, "performance-class"))
            m_class = value;
        else if (!strcmp(name, "heavy")) {
            if (value == string("true"))
                heavy = true;
            else
                heavy = false;
        } else if (!strcmp(name, "callsign"))
            callsign = value;
        else if (!strcmp(name, "fltrules"))
            fltrules = value;
        else if (!strcmp(name, "port"))
            port = value;
        else if (!strcmp(name, "time"))
            timeString = value;
        else if (!strcmp(name, "departure")) {
            departurePort = port;
            departureTime = timeString;
        } else if (!strcmp(name, "cruise-alt"))
            cruiseAlt = atoi(value.c_str());
        else if (!strcmp(name, "arrival")) {
            arrivalPort = port;
            arrivalTime = timeString;
        } else if (!strcmp(name, "repeat"))
            repeat = value;
        else if (!strcmp(name, "flight")) {
            // We have loaded and parsed all the information belonging to this flight
            // so we temporarily store it.
            //cerr << "Pusing back flight " << callsign << endl;
            //cerr << callsign  <<  " " << fltrules     << " "<< departurePort << " " <<  arrivalPort << " "
            //   << cruiseAlt <<  " " << departureTime<< " "<< arrivalTime   << " " << repeat << endl;

            //Prioritize aircraft
            string apt = fgGetString("/sim/presets/airport-id");
            //cerr << "Airport information: " << apt << " " << departurePort << " " << arrivalPort << endl;
            //if (departurePort == apt) score++;
            //flights.push_back(new FGScheduledFlight(callsign,
            //                                fltrules,
            //                                departurePort,
            //                                arrivalPort,
            //                                cruiseAlt,
            //                                departureTime,
            //                                arrivalTime,
            //                                repeat));
            if (requiredAircraft == "") {
                char buffer[16];
                snprintf(buffer, 16, "%d", acCounter);
                requiredAircraft = buffer;
            }
            SG_LOG(SG_AI, SG_BULK, "Adding flight: " << callsign << " "
                   << fltrules << " "
                   << departurePort << " "
                   << arrivalPort << " "
                   << cruiseAlt << " "
                   << departureTime << " "
                   << arrivalTime << " " << repeat << " " << requiredAircraft);
            // For database maintainance purposes, it may be convenient to
            //
            if (fgGetBool("/sim/traffic-manager/dumpdata") == true) {
                SG_LOG(SG_AI, SG_ALERT, "Traffic Dump FLIGHT," << callsign << ","
                       << fltrules << ","
                       << departurePort << ","
                       << arrivalPort << ","
                       << cruiseAlt << ","
                       << departureTime << ","
                       << arrivalTime << "," << repeat << "," << requiredAircraft);
            }

            _trafficManager->flights[requiredAircraft].push_back(new FGScheduledFlight(callsign,
                                                                      fltrules,
                                                                      departurePort,
                                                                      arrivalPort,
                                                                      cruiseAlt,
                                                                      departureTime,
                                                                      arrivalTime,
                                                                      repeat,
                                                                      requiredAircraft));
            requiredAircraft = "";
        } else if (!strcmp(name, "aircraft")) {
            endAircraft();
        }

        elementValueStack.pop_back();
    }


    void data(const char *s, int len)
    {
        string token = string(s, len);
        //cout << "Character data " << string(s,len) << endl;
        elementValueStack.back() += token;
    }

    void pi(const char *target, const char *data)
    {
        //cout << "Processing instruction " << target << ' ' << data << endl;
    }

    void warning(const char *message, int line, int column)
    {
        SG_LOG(SG_IO, SG_WARN,
               "Warning: " << message << " (" << line << ',' << column << ')');
    }

    void error(const char *message, int line, int column)
    {
        SG_LOG(SG_IO, SG_ALERT,
               "Error: " << message << " (" << line << ',' << column << ')');
    }

private:
    void endAircraft()
    {
        string isHeavy = heavy ? "true" : "false";

        if (missingModels.find(mdl) != missingModels.end()) {
            // don't stat() or warn again
            requiredAircraft = homePort = "";
            return;
        }

        if (!FGAISchedule::validModelPath(mdl)) {
            missingModels.insert(mdl);
            SG_LOG(SG_AI, SG_DEV_WARN, "TrafficMgr: Missing model path:" << mdl);
            requiredAircraft = homePort = "";
            return;
        }

        int proportion =
        (int) (fgGetDouble("/sim/traffic-manager/proportion") * 100);
        int randval = rand() & 100;
        if (randval > proportion) {
            requiredAircraft = homePort = "";
            return;
        }

        if (fgGetBool("/sim/traffic-manager/dumpdata") == true) {
            SG_LOG(SG_AI, SG_ALERT, "Traffic Dump AC," << homePort << "," << registration << "," << requiredAircraft
                   << "," << acType << "," << livery << ","
                   << airline << ","  << m_class << "," << offset << "," << radius << "," << flighttype << "," << isHeavy << "," << mdl);
        }

        if (requiredAircraft == "") {
            char buffer[16];
            snprintf(buffer, 16, "%d", acCounter);
            requiredAircraft = buffer;
        }
        if (homePort == "") {
            homePort = departurePort;
        }

        // caution, modifying the scheduled aircraft strucutre from the
        // 'wrong' thread. This is safe becuase FGTrafficManager won't touch
        // the structure while we exist.
        _trafficManager->scheduledAircraft.push_back(new FGAISchedule(mdl,
                                                     livery,
                                                     homePort,
                                                     registration,
                                                     requiredAircraft,
                                                     heavy,
                                                     acType,
                                                     airline,
                                                     m_class,
                                                     flighttype,
                                                     radius, offset));

        acCounter++;
        requiredAircraft = "";
        homePort = "";
        score = 0;
    }

    void parseTrafficDir(const SGPath& path)
    {
        SGTimeStamp st;
        st.stamp();

        simgear::Dir trafficDir(path);
        simgear::PathList d = trafficDir.children(simgear::Dir::TYPE_DIR | simgear::Dir::NO_DOT_OR_DOTDOT);

        for (const auto& p : d) {
            simgear::Dir d2(p);
            SG_LOG(SG_AI, SG_DEBUG, "parsing traffic in:" << p);
            simgear::PathList trafficFiles = d2.children(simgear::Dir::TYPE_FILE, ".xml");
            for (const auto& xml : trafficFiles) {
                try {
                    readXML(xml, *this);
                    if (_cancelThread) {
                        return;
                    }
                } catch (sg_exception& e) {
                    SG_LOG(SG_AI, SG_WARN, "XML error parsing traffic file:" << xml << "\n\t" << e.getFormattedMessage());
                }
            }
        } // of sub-directories iteration

        SG_LOG(SG_AI, SG_INFO, "parsing traffic schedules took:" << st.elapsedMSec() << "msec");
    }

  FGTrafficManager* _trafficManager;
  mutable std::mutex _lock;
  bool _isFinished;
  bool _cancelThread;
  simgear::PathList _trafficDirPaths;

// parser state

    string_list elementValueStack;
    // record model paths which are missing, to avoid duplicate
    // warnings when parsing traffic schedules.
    std::set<std::string> missingModels;

    std::string mdl, livery, registration, callsign, fltrules,
    port, timeString, departurePort, departureTime, arrivalPort, arrivalTime,
    repeat, acType, airline, m_class, flighttype, requiredAircraft, homePort;
    int cruiseAlt;
    int score, acCounter;
    double radius, offset;
    bool heavy;

};

/******************************************************************************
 * TrafficManager
 *****************************************************************************/
FGTrafficManager::FGTrafficManager() :
  inited(false),
  doingInit(false),
  trafficSyncRequested(false),
  waitingMetarTime(0.0),
  enabled("/sim/traffic-manager/enabled"),
  aiEnabled("/sim/ai/enabled"),
  realWxEnabled("/environment/realwx/enabled"),
  metarValid("/environment/metar/valid"),
  active("/sim/traffic-manager/active"),
  aiDataUpdateNow("/sim/terrasync/ai-data-update-now")
{
}

FGTrafficManager::~FGTrafficManager()
{
    shutdown();
}

void FGTrafficManager::shutdown()
{
    if (!inited) {
      if (doingInit) {
        scheduleParser.reset();
        doingInit = false;
        active = false;
      }

      return;
    }

    // Save the heuristics data
    bool saveData = false;
    sg_ofstream cachefile;
    if (fgGetBool("/sim/traffic-manager/heuristics")) {
        SGPath cacheData(globals->get_fg_home());
        cacheData.append("ai");
        const string airport = fgGetString("/sim/presets/airport-id");

        if ((airport) != "") {
            char buffer[128];
            ::snprintf(buffer, 128, "%c/%c/%c/",
                       airport[0], airport[1], airport[2]);
            cacheData.append(buffer);
            cacheData.append(airport + "-cache.txt");

            // Note: Intuitively, this doesn't make sense, but I do need to create the full file path first
            // before creating the directories. The SimGear fgpath code has changed so that it first chops off
            // the trailing dir separator and then determines the directory part of the file path by searching
            // for the last dir separator. Effecively, this causes a full element of the directory tree to be
            // skipped.
            SG_LOG(SG_GENERAL, SG_DEBUG, "Trying to create dir for : " << cacheData);
            if (!cacheData.exists()) {
                cacheData.create_dir(0755);
            }
            saveData = true;
            cachefile.open(cacheData);
            cachefile << "[TrafficManagerCachedata:ref:2011:09:04]" << endl;

        }
    }

    for (auto acft : scheduledAircraft) {
        if (saveData) {
            cachefile << acft->getRegistration() << " "
                << acft->getRunCount() << " "
                << acft->getHits() << " "
                << acft->getLastUsed() << endl;
        }
        delete acft;
    }
    if (saveData) {
        cachefile.close();
    }
    scheduledAircraft.clear();

    for (auto flight : flights) {
        for (auto scheduled : flight.second)
            delete scheduled;
    }
    flights.clear();

    currAircraft = scheduledAircraft.begin();
    doingInit = false;
    inited = false;
    trafficSyncRequested = false;
    active = false;
}

bool FGTrafficManager::doDataSync()
{
    auto terraSync = globals->get_subsystem<simgear::SGTerraSync>();
    bool doDataSync = fgGetBool("/sim/terrasync/ai-data-enabled");
    if (doDataSync && terraSync) {
        if (!trafficSyncRequested) {
            SG_LOG(SG_AI, SG_INFO, "Sync of AI traffic via TerraSync enabled");
            terraSync->scheduleDataDir("AI/Traffic");
            trafficSyncRequested = true;
        }

        if (terraSync->isDataDirPending("AI/Traffic")) {
            return false; // remain in the init state
        }

        trafficSyncRequested = false;
        return true;

        SG_LOG(SG_AI, SG_INFO, "Traffic files sync complete");
    }
    return true;
}

void FGTrafficManager::init()
{
    if (!enabled) {
      return;
    }

    // TorstenD: don't start the traffic manager before the FDM is initialized
    // The FDM needs the scenery loaded and will wait for our spawned AIModels PagedLOD Nodes
    // to appear if they are close (less than 1000m) to our position
    if( !fgGetBool("/sim/signals/fdm-initialized") )
      return;

    assert(!doingInit);

    if (!doDataSync())
        return; // remain in the init state whilst updating

    doingInit = true;
    if (string(fgGetString("/sim/traffic-manager/datafile")).empty()) {
        simgear::PathList dirs = globals->get_data_paths("AI/Traffic");

        // temporary flag to restrict loading while traffic data is found
        // through terrasync /and/ fgdata. Ultimately we *do* want to be able to
        // overlay sources.

        if (dirs.size() > 1) {
            SGPath p = dirs.back();
            if (simgear::strutils::starts_with(p.utf8Str(),
                                               globals->get_fg_root().utf8Str()))
            {
                dirs.pop_back();
            }
        }

        if (dirs.empty()) {
            doingInit = false;
            return;
        }

        scheduleParser.reset(new ScheduleParseThread(this));
        scheduleParser->setTrafficDirs(dirs);
        scheduleParser->start();
    } else {
        fgSetBool("/sim/traffic-manager/heuristics", false);
        SGPath path = string(fgGetString("/sim/traffic-manager/datafile"));
        string ext = path.extension();
        if (path.extension() == "xml") {
            if (path.exists()) {
                // use a SchedulerParser to parse, but run it in this thread,
                // i.e don't start it
                ScheduleParseThread parser(this);
                readXML(path, parser);
            }
        } else if (path.extension() == "conf") {
            if (path.exists()) {
                readTimeTableFromFile(path);
            }
        } else {
             SG_LOG(SG_AI, SG_ALERT,
                               "Unknown data format " << path
                                << " for traffic");
        }
        //exit(1);
    }
    active = true;
}



void FGTrafficManager::finishInit()
{
    assert(doingInit);
    SG_LOG(SG_AI, SG_INFO, "finishing AI-Traffic init");
    loadHeuristics();
    PerformanceDB* perfDB = globals->get_subsystem<PerformanceDB>();
    // Do sorting and scoring separately, to take advantage of the "homeport" variable
    for (auto schedule : scheduledAircraft) {
        schedule->setScore();
        if (!perfDB->havePerformanceDataForAircraftType(schedule->getAircraft())) {
            SG_LOG(SG_AI, SG_DEV_WARN, "AI-Traffic: schedule aircraft missing performance data:" << schedule->getAircraft());
        }
    }

    sort(scheduledAircraft.begin(), scheduledAircraft.end(),
         compareSchedules);
    currAircraft = scheduledAircraft.begin();
    currAircraftClosest = scheduledAircraft.begin();

    doingInit = false;
    inited = true;
    active = true;
}

void FGTrafficManager::loadHeuristics()
{
    if (!fgGetBool("/sim/traffic-manager/heuristics")) {
        return;
    }

    HeuristicMap heurMap;
    //cerr << "Processing Heuristics" << endl;
    // Load the heuristics data
    SGPath cacheData(globals->get_fg_home());
    cacheData.append("ai");
    string airport = fgGetString("/sim/presets/airport-id");
    if ((airport) != "") {
      char buffer[128];
      ::snprintf(buffer, 128, "%c/%c/%c/",
                 airport[0], airport[1], airport[2]);
      cacheData.append(buffer);
      cacheData.append(airport + "-cache.txt");
      string revisionStr;
      if (cacheData.exists()) {
        sg_ifstream data(cacheData);
        data >> revisionStr;
        if (revisionStr != "[TrafficManagerCachedata:ref:2011:09:04]") {
          SG_LOG(SG_AI, SG_ALERT,"Traffic Manager Warning: discarding outdated cachefile " <<
                 cacheData << " for Airport " << airport);
        } else {
          while (1) {
            Heuristic h; // = new Heuristic;
            data >> h.registration >> h.runCount >> h.hits >> h.lastRun;
            if (data.eof())
              break;
            HeuristicMapIterator itr = heurMap.find(h.registration);
            if (itr != heurMap.end()) {
              SG_LOG(SG_AI, SG_DEV_WARN,"Traffic Manager Warning: found duplicate tailnumber " <<
                     h.registration << " for AI aircraft");
            } else {
              heurMap[h.registration] = h;
            }
          }
        }
      }
    }

  for(currAircraft = scheduledAircraft.begin(); currAircraft != scheduledAircraft.end(); ++currAircraft) {
        const string& registration = (*currAircraft)->getRegistration();
        HeuristicMapIterator itr = heurMap.find(registration);
        if (itr != heurMap.end()) {
            (*currAircraft)->setrunCount(itr->second.runCount);
            (*currAircraft)->setHits(itr->second.hits);
            (*currAircraft)->setLastUsed(itr->second.lastRun);
        }
    }
}

bool FGTrafficManager::metarReady(double dt)
{
    // wait for valid METAR (when realWX is enabled only), since we need
    // to know the active runway
    if (metarValid || !realWxEnabled)
    {
        waitingMetarTime = 0.0;
        return true;
    }

    // METAR timeout: when running offline, remote server is down etc
    if (waitingMetarStation != fgGetString("/environment/metar/station-id"))
    {
        // station has changed: wait for reply, restart timeout
        waitingMetarTime = 0.0;
        waitingMetarStation = fgGetString("/environment/metar/station-id");
        return false;
    }

    // timeout elapsed (10 seconds)?
    if (waitingMetarTime > 20.0)
    {
        return true;
    }

    waitingMetarTime += dt;
    return false;
}

void FGTrafficManager::update(double dt)
{
    if (!enabled)
    {
        if (inited || doingInit)
            shutdown();
        return;
    }

    if (!metarReady(dt))
        return;

    if (aiDataUpdateNow)
    {
        aiDataUpdateNow = false;
        shutdown();
    }
    if (!aiEnabled)
    {
        // traffic depends on AI module
        aiEnabled = true;
    }

    if (!inited) {
        if (!doingInit) {
            init();
        }

        if (!doingInit || !scheduleParser->isFinished()) {
          return;
        }

        finishInit();
    }


    if (scheduledAircraft.empty()) {
        return;
    }

    SGVec3d userCart = globals->get_aircraft_position_cart();

    if (currAircraft == scheduledAircraft.end()) {
        currAircraft = scheduledAircraft.begin();
    }

    time_t now = globals->get_time_params()->get_cur_time();

    //cerr << "Processing << " << (*currAircraft)->getRegistration() << " with score " << (*currAircraft)->getScore() << endl;
    if ((*currAircraft)->update(now, userCart)) {
        // schedule is done - process another aircraft in next iteration
        currAircraft++;
    }
}

void FGTrafficManager::readTimeTableFromFile(SGPath infileName)
{
    string model;
    string livery;
    string homePort;
    string registration;
    string flightReq;
    bool   isHeavy;
    string acType;
    string airline;
    string m_class;
    string FlightType;
    double radius;
    double offset;

    char buffer[256];
    string buffString;
    vector <string> tokens, depTime,arrTime;
    vector <string>::iterator it;

    sg_ifstream infile(infileName);
    while (1) {
         infile.getline(buffer, 256);
         if (infile.eof()) {
             break;
         }
         //cerr << "Read line : " << buffer << endl;
         buffString = string(buffer);
         tokens.clear();
         Tokenize(buffString, tokens, " \t");
         //for (it = tokens.begin(); it != tokens.end(); it++) {
         //    cerr << "Tokens: " << *(it) << endl;
         //}
         //cerr << endl;
         if (!tokens.empty()) {
             if (tokens[0] == string("AC")) {
                 if (tokens.size() != 13) {
                     throw sg_io_exception("Error parsing traffic file @ " + buffString, infileName);
                 }


                 model          = tokens[12];
                 livery         = tokens[6];
                 homePort       = tokens[1];
                 registration   = tokens[2];
                 if (tokens[11] == string("false")) {
                     isHeavy = false;
                 } else {
                     isHeavy = true;
                 }
                 acType         = tokens[4];
                 airline        = tokens[5];
                 flightReq      = tokens[3] + tokens[5];
                 m_class        = tokens[10];
                 FlightType     = tokens[9];
                 radius         = atof(tokens[8].c_str());
                 offset         = atof(tokens[7].c_str());;

                 if (!FGAISchedule::validModelPath(model)) {
                     SG_LOG(SG_AI, SG_WARN, "TrafficMgr: Missing model path:" <<
                            model << " from " << infileName);
                 } else {

                 SG_LOG(SG_AI, SG_INFO, "Adding Aircraft" << model << " " << livery << " " << homePort << " "
                                                                << registration << " " << flightReq << " " << isHeavy
                                                                << " " << acType << " " << airline << " " << m_class
                                                                << " " << FlightType << " " << radius << " " << offset);
                 scheduledAircraft.push_back(new FGAISchedule(model,
                                                              livery,
                                                              homePort,
                                                              registration,
                                                              flightReq,
                                                              isHeavy,
                                                              acType,
                                                              airline,
                                                              m_class,
                                                              FlightType,
                                                              radius,
                                                              offset));
                 } // of valid model path
             }
             if (tokens[0] == string("FLIGHT")) {
                 //cerr << "Found flight " << buffString << " size is : " << tokens.size() << endl;
                 if (tokens.size() != 10) {
                     SG_LOG(SG_AI, SG_ALERT, "Error parsing traffic file " << infileName << " at " << buffString);
                     exit(1);
                 }
                 string callsign = tokens[1];
                 string fltrules = tokens[2];
                 string weekdays = tokens[3];
                 string departurePort = tokens[5];
                 string arrivalPort   = tokens[7];
                 int    cruiseAlt     = atoi(tokens[8].c_str());
                 string depTimeGen    = tokens[4];
                 string arrTimeGen    = tokens[6];
                 string repeat        = "WEEK";
                 string requiredAircraft = tokens[9];

                 if (weekdays.size() != 7) {
                     SG_LOG(SG_AI, SG_ALERT, "Found misconfigured weekdays string" << weekdays);
                     exit(1);
                 }
                 depTime.clear();
                 arrTime.clear();
                 Tokenize(depTimeGen, depTime, ":");
                 Tokenize(arrTimeGen, arrTime, ":");
                 double dep = atof(depTime[0].c_str()) + (atof(depTime[1].c_str()) / 60.0);
                 double arr = atof(arrTime[0].c_str()) + (atof(arrTime[1].c_str()) / 60.0);
                 //cerr << "Using " << dep << " " << arr << endl;
                 bool arrivalWeekdayNeedsIncrement = false;
                 if (arr < dep) {
                       arrivalWeekdayNeedsIncrement = true;
                 }
                 for (int i = 0; i < 7; i++) {
                     int j = i+1;
                     if (weekdays[i] != '.') {
                         char buffer[4];
                         snprintf(buffer, 4, "%d/", j);
                         string departureTime = string(buffer) + depTimeGen + string(":00");
                         string arrivalTime;
                         if (!arrivalWeekdayNeedsIncrement) {
                             arrivalTime   = string(buffer) + arrTimeGen + string(":00");
                         }
                         if (arrivalWeekdayNeedsIncrement && i != 6 ) {
                             snprintf(buffer, 4, "%d/", j+1);
                             arrivalTime   = string(buffer) + arrTimeGen + string(":00");
                         }
                         if (arrivalWeekdayNeedsIncrement && i == 6 ) {
                             snprintf(buffer, 4, "%d/", 0);
                             arrivalTime   = string(buffer) + arrTimeGen  + string(":00");
                         }
                         SG_LOG(SG_AI, SG_ALERT, "Adding flight " << callsign       << " "
                                                      << fltrules       << " "
                                                      <<  departurePort << " "
                                                      <<  arrivalPort   << " "
                                                      <<  cruiseAlt     << " "
                                                      <<  departureTime << " "
                                                      <<  arrivalTime   << " "
                                                      << repeat        << " "
                                                      <<  requiredAircraft);

                         flights[requiredAircraft].push_back(new FGScheduledFlight(callsign,
                                                                 fltrules,
                                                                 departurePort,
                                                                 arrivalPort,
                                                                 cruiseAlt,
                                                                 departureTime,
                                                                 arrivalTime,
                                                                 repeat,
                                                                 requiredAircraft));
                    }
                }
             }
         }

    }
    //exit(1);
}


void FGTrafficManager::Tokenize(const string& str,
                      vector<string>& tokens,
                      const string& delimiters)
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGTrafficManager> registrantFGTrafficManager(
    SGSubsystemMgr::POST_FDM,
    {{"terrasync", SGSubsystemMgr::Dependency::HARD},
     {"PerformanceDB", SGSubsystemMgr::Dependency::HARD}});
