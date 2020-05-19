/* -*- Mode: C++ -*- *****************************************************
 * atic.hxx
 * Written by Durk Talsma. Started August 1, 2010; based on earlier work
 * by David C. Luff
 *
 * Updated by Jonathan Redpath. Started June 12, 2019. Documenting and extending
 * functionality of the ATC subsystem
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

/**************************************************************************
 * The ATC Manager interfaces the users aircraft within the AI traffic system
 * and also monitors the ongoing AI traffic patterns for potential conflicts
 * and interferes where necessary.
 *************************************************************************/

#ifndef _ATC_MGR_HXX_
#define _ATC_MGR_HXX_


#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/SGReferenced.hxx>

#include <ATC/trafficcontrol.hxx>
#include <ATC/atcdialog.hxx>
#include <AIModel/AIAircraft.hxx>
#include <Traffic/Schedule.hxx>
#include <Traffic/SchedFlight.hxx>

typedef std::vector<FGATCController*> AtcVec;
typedef std::vector<FGATCController*>::iterator AtcVecIterator;

class FGATCManager : public SGSubsystem
{
private:
    AtcVec activeStations;
    FGATCController *controller, *prevController; // The ATC controller that is responsible for the user's aircraft.
    bool networkVisible;
    bool initSucceeded;
    SGPropertyNode_ptr trans_num;
    string destination;

    std::unique_ptr<FGAISchedule> userAircraftTrafficRef;
    std::unique_ptr<FGScheduledFlight> userAircraftScheduledFlight;
    
public:
    FGATCManager();
    virtual ~FGATCManager();

    // Subsystem API.
    void postinit() override;
    void shutdown() override;
    void update(double time) override;
    
    
    
    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "ATC"; }

    void addController(FGATCController *controller);
    void removeController(FGATCController* controller);
    
    void reposition();

};

#endif // _ATC_MRG_HXX_
