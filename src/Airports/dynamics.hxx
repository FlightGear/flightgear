// dynamics.hxx - a class to manage the higher order airport ground activities
// Written by Durk Talsma, started December 2004.
//
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
//
// $Id$


#ifndef _AIRPORT_DYNAMICS_HXX_
#define _AIRPORT_DYNAMICS_HXX_

#include <ATC/trafficcontrol.hxx>
#include "parking.hxx"
#include "groundnetwork.hxx"
#include "runwayprefs.hxx"
#include "sidstar.hxx"

// forward decls
class FGAirport;
class FGEnvironment;

class FGAirportDynamics {

private:
    FGAirport* _ap;

    FGParkingVec         parkings;
    FGRunwayPreference   rwyPrefs;
    FGSidStar            SIDs;
    FGStartupController  startupController;
    FGGroundNetwork      groundNetwork;
    FGTowerController    towerController;
    FGApproachController approachController;

    time_t lastUpdate;
    std::string prevTrafficType;
    stringVec landing;
    stringVec takeoff;
    stringVec milActive, comActive, genActive, ulActive;
    stringVec *currentlyActive;
    intVec freqAwos;     // </AWOS>
    intVec freqUnicom;   // </UNICOM>
    intVec freqClearance;// </CLEARANCE>
    intVec freqGround;   // </GROUND>
    intVec freqTower;    // </TOWER>
    intVec freqApproach; // </APPROACH>

    int atisSequenceIndex;
    double atisSequenceTimeStamp;

    std::string chooseRunwayFallback();
    bool innerGetActiveRunway(const std::string &trafficType, int action, std::string &runway, double heading);
    std::string chooseRwyByHeading(stringVec rwys, double heading);

    double elevation;

public:
    FGAirportDynamics(FGAirport* ap);
    ~FGAirportDynamics();

    void addAwosFreq     (int val) {
        freqAwos.push_back(val);
    };
    void addUnicomFreq   (int val) {
        freqUnicom.push_back(val);
    };
    void addClearanceFreq(int val) {
        freqClearance.push_back(val);
    };
    void addGroundFreq   (int val) {
        freqGround.push_back(val);
    };
    void addTowerFreq    (int val) {
        freqTower.push_back(val);
    };
    void addApproachFreq (int val) {
        freqApproach.push_back(val);
    };

    void init();
    double getLongitude() const;
    // Returns degrees
    double getLatitude()  const;
    // Returns ft
    double getElevation() const;
    const string& getId() const;

    void getActiveRunway(const string& trafficType, int action, string& runway, double heading);

    void addParking(FGParking& park);
    bool getAvailableParking(double *lat, double *lon,
                             double *heading, int *gate, double rad, const string& fltype,
                             const string& acType, const string& airline);
    void getParking         (int id, double *lat, double* lon, double *heading);
    FGParking *getParking(int i);
    void releaseParking(int id);
    string getParkingName(int i);
    int getNrOfParkings() {
        return parkings.size();
    };
    //FGAirport *getAddress() { return this; };
    //const string &getName() const { return _name;};
    // Returns degrees

    // Departure / Arrival procedures
    FGSidStar * getSIDs() {
        return &SIDs;
    };
    FGAIFlightPlan * getSID(string activeRunway, double heading);


    // ATC related functions.
    FGStartupController    *getStartupController()    {
        return &startupController;
    };
    FGGroundNetwork        *getGroundNetwork()        {
        return &groundNetwork;
    };
    FGTowerController      *getTowerController()      {
        return &towerController;
    };
    FGApproachController   *getApproachController()   {
        return &approachController;
    };

    int getGroundFrequency(unsigned leg);
    int getTowerFrequency  (unsigned nr);

    /// get current ATIS sequence letter
    const std::string getAtisSequence();

    /// get the current ATIS sequence number, updating it if necessary
    int updateAtisSequence(int interval, bool forceUpdate);

    void setRwyUse(const FGRunwayPreference& ref);
};



#endif
