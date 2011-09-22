/* -*- Mode: C++ -*- *****************************************************
 * atic.hxx
 * Written by Durk Talsma. Started August 1, 2010; based on earlier work
 * by David C. Luff
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
 * The ATC Manager interfaces the users aircraft with the AI traffic system
 * and also monitors the ongoing AI traffic patterns for potential conflicts
 * and interferes where necessary. 
 *************************************************************************/ 

#ifndef _ATC_MGR_HXX_
#define _ATC_MGR_HXX_

//#include <simgear/structure/SGReferenced.hxx>
//#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


#include <ATC/trafficcontrol.hxx>
#include <ATC/atcdialog.hxx>

#include <AIModel/AIAircraft.hxx>
//class FGATCController;


typedef vector<FGATCController*> AtcVec;
typedef vector<FGATCController*>::iterator AtcVecIterator;

class FGATCManager : public SGSubsystem
{
private:
  AtcVec activeStations;
  FGAIAircraft ai_ac;
  FGATCController *controller, *prevController; // The ATC controller that is responsible for the user's aircraft. 
  bool networkVisible;
  bool initSucceeded;

public:
  FGATCManager();
  ~FGATCManager();
  void init();
  void addController(FGATCController *controller);
  void update(double time);
};
  
#endif // _ATC_MRG_HXX_