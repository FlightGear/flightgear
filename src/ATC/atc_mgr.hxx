/*
 * SPDX-FileName: atc_mgr.hxx
 * SPDX-FileComment: Started August 1, 2010; based on earlier work by David C. Luff
 * SPDX-FileCopyrightText: Written by Durk Talsma.
 * SPDX-FileContributor: Updated by Jonathan Redpath. Started June 12, 2019. Documenting and extending functionality of the ATC subsystem
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/**************************************************************************
 * The ATC Manager interfaces the users aircraft within the AI traffic system
 * and also monitors the ongoing AI traffic patterns for potential conflicts
 * and interferes where necessary.
 *************************************************************************/

#pragma once

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <AIModel/AIAircraft.hxx>
#include <ATC/trafficcontrol.hxx>
#include <ATC/EnRouteController.hxx>
#include <Traffic/SchedFlight.hxx>
#include <Traffic/Schedule.hxx>


typedef std::vector<FGATCController*> AtcVec;
typedef std::vector<FGATCController*>::iterator AtcVecIterator;

class FGATCManager : public SGSubsystem
{
private:
    AtcVec activeStations;
    FGATCController *controller, *prevController; // The ATC controller that is responsible for the user's aircraft.
    FGATCController *enRouteController;

    bool networkVisible;
    bool initSucceeded;
    SGPropertyNode_ptr trans_num;
    std::string destination;

    std::unique_ptr<FGAISchedule> userAircraftTrafficRef;
    std::unique_ptr<FGScheduledFlight> userAircraftScheduledFlight;

    SGPropertyNode_ptr _routeManagerDestinationAirportNode;

public:
    FGATCManager();
    virtual ~FGATCManager();

    // Subsystem API.
    void postinit() override;
    void shutdown() override;
    void update(double time) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "ATC"; }

    void addController(FGATCController* controller);
    void removeController(FGATCController* controller);

    FGATCController* getEnRouteController();

    void reposition();
};
