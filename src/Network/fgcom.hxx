#ifndef FG_FGCOM_HXX
#define FG_FGCOM_HXX

// fgcom.hxx -- FGCom: Voice communication
//
// Written by Clement de l'Hamaide, started May 2013.
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

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>
#include <simgear/math/sg_geodesy.hxx>

class FGCom : public SGSubsystem,
              public SGPropertyChangeListener
{
public:
    FGCom();
    virtual ~FGCom();

    // Subsystem API.
    void bind() override;
    void init() override;
    void postinit() override;
    void shutdown() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "fgcom"; }

    virtual void valueChanged(SGPropertyNode *prop);
    void iaxTextEvent(struct iaxc_ev_text text);

private:
    SGPropertyNode_ptr _ptt_node;                             // PTT; nonzero int indicating channel number (instrumentation/comm/[channel-1])
    SGPropertyNode_ptr _selected_comm_node;                   // selected channel (fgcom); nonzero channel int indicating channel number (instrumentation/comm/[channel-1])
    SGPropertyNode_ptr _commFrequencyNode;                    // current comm node in use; e.g. /instrumentation/comm[0]
    SGPropertyNode_ptr _commVolumeNode;                       // current volume node in use; e.g. /instrumentation/comm[0]/volume
    SGPropertyNode_ptr _test_node;                            // sim/fgcom/test
    SGPropertyNode_ptr _text_node;                            // sim/fgcom/text
    SGPropertyNode_ptr _server_node;                          // sim/fgcom/server
    SGPropertyNode_ptr _enabled_node;                         // sim/fgcom/enabled
    SGPropertyNode_ptr _version_node;                         // sim/version/flightgear
    SGPropertyNode_ptr _micBoost_node;                        // sim/fgcom/mic-boost
    SGPropertyNode_ptr _callsign_node;                        // sim/multiplay/callsign
    SGPropertyNode_ptr _register_node;                        // sim/fgcom/register/enabled
    SGPropertyNode_ptr _username_node;                        // sim/fgcom/register/username
    SGPropertyNode_ptr _password_node;                        // sim/fgcom/register/password
    SGPropertyNode_ptr _micLevel_node;                        // sim/fgcom/mic-level
    SGPropertyNode_ptr _silenceThd_node;                      // sim/fgcom/silence-threshold
    SGPropertyNode_ptr _speakerLevel_node;                    // sim/fgcom/speaker-level
    SGPropertyNode_ptr _deviceID_node[4];                     // sim/fgcom/device[n]/id
    SGPropertyNode_ptr _deviceName_node[4];                   // sim/fgcom/device[n]/name
    SGPropertyNode_ptr _deviceInput_node[4];                  // sim/fgcom/device[n]/available-input
    SGPropertyNode_ptr _deviceOutput_node[4];                 // sim/fgcom/device[n]/available-output
    SGPropertyNode_ptr _selectedInput_node;                   // sim/fgcom/device-input
    SGPropertyNode_ptr _selectedOutput_node;                  // sim/fgcom/device-output
    SGPropertyNode_ptr _showMessages_node;                    // sim/fgcom/show-messages
    SGPropertyNode_ptr _mpTransmitFrequencyNode;              // sim/multiplay/comm-transmit-frequency-mhz
    SGPropertyNode_ptr _mpTransmitPowerNode;                  // sim/multiplay/comm-transmit-power-norm

    double   _maxRange = 0.0;
    double   _minRange = 0.0;
    double   _currentCommFrequency = 0.0;
    double   _currentCallFrequency = 0.0;
    bool     _register = true;
    bool     _enabled = false;
    bool     _initialized = false;
    int      _regId = 0;
    int      _currentCallIdent = -1;
    //int      _callComm1;
    int      _listener_active = 0;
    std::string   _server;
    std::string   _callsign;
    std::string   _username;
    std::string   _password;
    SGTimeStamp   _processingTimer;
    SGGeod        _aptPos;

    std::string   computePhoneNumber(const double& freq, const std::string& icao) const;
    std::string   getAirportCode(const double& freq);
//    SGGeod        getAirportPos(const double& freq) const;
    void          setupCommFrequency(int channel = -1);
    double        getCurrentFrequencyKhz() const;
    double        getCurrentCommVolume() const;
    bool          isInRange(const double& freq) const;

    void updateCall();
    void connectToCommFrequency();
    void testMode(bool testMode);
};

#endif // of FG_FGCOM_HXX


