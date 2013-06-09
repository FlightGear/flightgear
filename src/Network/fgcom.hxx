// fgcom.hxx -- FGCom: Voice communication
//
// Written by Clement de l'Hamaide, started Mai 2013.
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

#include "iaxclient.h"
#include <simgear/structure/subsystem_mgr.hxx>

#define NUM_CALLS 4
#define MAX_RANGE 150.0
#define DEFAULT_SERVER "clemaez.dyndns.org"


class FGCom : public SGSubsystem, public SGPropertyChangeListener
{
  public:
    FGCom();
    virtual ~FGCom();

    virtual void bind();
    virtual void unbind();
    virtual void init();
    virtual void postinit();
    virtual void update(double dt);
    virtual void valueChanged(SGPropertyNode *prop);
    virtual void shutdown();

  private:

    SGPropertyNode_ptr _ptt0_node;                            // instrumentation/nav[0]/ptt
    SGPropertyNode_ptr _nav0_node;                            // instrumentation/nav[0]/frequencies/selected-mhz
    SGPropertyNode_ptr _nav1_node;                            // instrumentation/nav[1]/frequencies/selected-mhz
    SGPropertyNode_ptr _comm0_node;                           // instrumentation/comm[0]/frequencies/selected-mhz
    SGPropertyNode_ptr _comm1_node;                           // instrumentation/comm[1]/frequencies/selected-mhz
    SGPropertyNode_ptr _server_node;                          // sim/fgcom/server
    SGPropertyNode_ptr _enabled_node;                         // sim/fgcom/enabled
    SGPropertyNode_ptr _micBoost_node;                        // sim/fgcom/mic-boost
    SGPropertyNode_ptr _callsign_node;                        // sim/multiplay/callsign
    SGPropertyNode_ptr _register_node;                        // sim/fgcom/register/enabled
    SGPropertyNode_ptr _username_node;                        // sim/fgcom/register/username
    SGPropertyNode_ptr _password_node;                        // sim/fgcom/register/password
    SGPropertyNode_ptr _micLevel_node;                        // sim/fgcom/mic-level
    SGPropertyNode_ptr _speakerLevel_node;                    // sim/fgcom/speaker-level
    SGPropertyNode_ptr _deviceID_node[4];                     // sim/fgcom/device[n]/id
    SGPropertyNode_ptr _deviceName_node[4];                   // sim/fgcom/device[n]/name
    SGPropertyNode_ptr _deviceInput_node[4];                  // sim/fgcom/device[n]/available-input
    SGPropertyNode_ptr _deviceOutput_node[4];                 // sim/fgcom/device[n]/available-output
    SGPropertyNode_ptr _selectedInput_node;                   // sim/fgcom/device-input
    SGPropertyNode_ptr _selectedOutput_node;                  // sim/fgcom/device-output



    double   _currentComm0;
    double   _currentComm1;
    double   _currentNav0;
    double   _currentNav1;
    bool     _nav0Changed;
    bool     _nav1Changed;
    bool     _comm0Changed;
    bool     _comm1Changed;
    bool     _register;
    bool     _enabled;
    int      _regId;
    int      _callNav0;
    int      _callNav1;
    int      _callComm0;
    int      _callComm1;
    int      _listener_active;
    std::string   _server;
    std::string   _callsign;
    std::string   _username;
    std::string   _password;

    std::string   computePhoneNumber(const double& freq, const std::string& icao) const;
    std::string   getAirportCode(const double& freq) const;
    std::string   getVorCode(const double& freq) const;
    bool          isInRange();

};
