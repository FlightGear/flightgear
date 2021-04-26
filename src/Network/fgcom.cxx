// fgcom.cxx -- FGCom: Voice communication
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "fgcom.hxx"

// standard library includes
#include <cstdio>

// simgear includes
#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/timing/timestamp.hxx>

// flightgear includes
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <ATC/CommStation.hxx>
#include <Airports/airport.hxx>
#include <Navaids/navlist.hxx>
#include <Main/sentryIntegration.hxx>

#include <3rdparty/iaxclient/lib/iaxclient.h>


#define NUM_CALLS 4
#define MAX_GND_RANGE 10.0
#define MAX_TWR_RANGE 50.0
#define MAX_RANGE 100.0
#define MIN_RANGE  20.0
#define MIN_GNDTWR_RANGE 0.0
#define DEFAULT_SERVER "fgcom.flightgear.org"
#define IAX_DELAY 300  // delay between calls in milliseconds
#define TEST_FREQ 910.00
#define NULL_ICAO "ZZZZ"

const int special_freq[] = { // Define some freq who need to be used with NULL_ICAO
	910000,
	911000,
	700000,
	123450,
	122750,
	121500,
	123500 };

static FGCom* static_instance = NULL;



static int iaxc_callback( iaxc_event e )
{
  switch( e.type )
  {
    case IAXC_EVENT_TEXT:
      static_instance->iaxTextEvent(e.ev.text);
      break;
    default:
      return 0;
  }
  return 1;
}



void FGCom::iaxTextEvent(struct iaxc_ev_text text)
{
  if( (text.type == IAXC_TEXT_TYPE_STATUS ||
       text.type == IAXC_TEXT_TYPE_IAX) &&
       _showMessages_node->getBoolValue() )
  {
    _text_node->setStringValue(text.message);
  }
}



FGCom::FGCom()
{
    _maxRange         = MAX_RANGE;
    _minRange         = MIN_RANGE;
}



FGCom::~FGCom()
{
}



void FGCom::bind()
{
  SGPropertyNode *node     = fgGetNode("/sim/fgcom", 0, true);
  _test_node               = node->getChild( "test", 0, true );
  _server_node             = node->getChild( "server", 0, true );
  _enabled_node            = node->getChild( "enabled", 0, true );
  _micBoost_node           = node->getChild( "mic-boost", 0, true );
  _micLevel_node           = node->getChild( "mic-level", 0, true );
  _silenceThd_node         = node->getChild( "silence-threshold", 0, true );
  _speakerLevel_node       = node->getChild( "speaker-level", 0, true );
  _selectedInput_node      = node->getChild( "device-input", 0, true );
  _selectedOutput_node     = node->getChild( "device-output", 0, true );
  _showMessages_node       = node->getChild( "show-messages", 0, true );

  SGPropertyNode *reg_node = node->getChild("register", 0, true);
  _register_node           = reg_node->getChild( "enabled", 0, true );
  _username_node           = reg_node->getChild( "username", 0, true );
  _password_node           = reg_node->getChild( "password", 0, true );

  _ptt_node                = fgGetNode("/controls/radios/comm-ptt", true);
  _selected_comm_node      = fgGetNode("/controls/radios/comm-radio-selected", true);
  _callsign_node           = fgGetNode("/sim/multiplay/callsign", true);
  _text_node               = fgGetNode("/sim/messages/atc", true );
  _version_node            = fgGetNode("/sim/version/flightgear", true );

  // Set default values if not provided
  if ( !_enabled_node->hasValue() )
      _enabled_node->setBoolValue(true);

  if ( !_test_node->hasValue() )
      _test_node->setBoolValue(false);

  if ( !_micBoost_node->hasValue() )
      _micBoost_node->setIntValue(1);

  if ( !_server_node->hasValue() )
      _server_node->setStringValue(DEFAULT_SERVER);

  if ( !_speakerLevel_node->hasValue() )
      _speakerLevel_node->setFloatValue(1.0);

  if ( !_micLevel_node->hasValue() )
      _micLevel_node->setFloatValue(1.0);

  if ( !_silenceThd_node->hasValue() )
      _silenceThd_node->setFloatValue(-35.0);

  if ( !_register_node->hasValue() )
      _register_node->setBoolValue(false);

  if ( !_username_node->hasValue() )
      _username_node->setStringValue("guest");

  if ( !_password_node->hasValue() )
      _password_node->setStringValue("guest");

  if ( !_showMessages_node->hasValue() )
      _showMessages_node->setBoolValue(false);

  _mpTransmitFrequencyNode = fgGetNode("sim/multiplay/comm-transmit-frequency-hz", 0, true);
  _mpTransmitPowerNode = fgGetNode("sim/multiplay/comm-transmit-power-norm", 0, true);
      
  _selectedOutput_node->addChangeListener(this);
  _selectedInput_node->addChangeListener(this);
  _speakerLevel_node->addChangeListener(this);
  _silenceThd_node->addChangeListener(this);
  _micBoost_node->addChangeListener(this);
  _micLevel_node->addChangeListener(this);
  _enabled_node->addChangeListener(this);
  _ptt_node->addChangeListener(this);
  _selected_comm_node->addChangeListener(this);
  _test_node->addChangeListener(this);
  // if you listen to more properties, be sure to remove
  // the listener in unbind() as well
}



void FGCom::unbind()
{
    _selectedOutput_node->removeChangeListener(this);
    _selectedInput_node->removeChangeListener(this);
    _speakerLevel_node->removeChangeListener(this);
    _silenceThd_node->removeChangeListener(this);
    _micBoost_node->removeChangeListener(this);
    _micLevel_node->removeChangeListener(this);
    _enabled_node->removeChangeListener(this);
    _ptt_node->removeChangeListener(this);
    _selected_comm_node->removeChangeListener(this);
    _test_node->removeChangeListener(this);
}



void FGCom::init()
{
  _enabled          = _enabled_node->getBoolValue();
  _server           = _server_node->getStringValue();
  _register         = _register_node->getBoolValue();
  _username         = _username_node->getStringValue();
  _password         = _password_node->getStringValue();

    _currentCommFrequency = 0.0;

  _maxRange         = MAX_RANGE;
  _minRange         = MIN_RANGE;
}



void FGCom::postinit()
{
    if( !_enabled ) {
        return;
    }
    
    //WARNING: this _must_ be executed after sound system is totally initialized !
    if( iaxc_initialize(NUM_CALLS) ) {
        SG_LOG(SG_IO, SG_ALERT, "FGCom: cannot initialize iaxclient");
        _enabled = false;
        return;
    }

    assert( static_instance == NULL );
    static_instance = this;
    iaxc_set_event_callback( iaxc_callback );
    
    // FIXME: To be implemented in IAX audio driver
    //iaxc_mic_boost_set( _micBoost_node->getIntValue() );
    std::string app = "FGFS-";
    app += _version_node->getStringValue();

    iaxc_set_callerid( _callsign_node->getStringValue(), app.c_str() );
    iaxc_set_formats (IAXC_FORMAT_SPEEX, IAXC_FORMAT_ULAW|IAXC_FORMAT_SPEEX);
    iaxc_set_speex_settings(1, 5, 0, 1, 0, 3);
    iaxc_set_filters(IAXC_FILTER_AGC | IAXC_FILTER_DENOISE);
    iaxc_set_silence_threshold(_silenceThd_node->getFloatValue());
    iaxc_start_processing_thread ();

    // Now IAXClient is initialized
    _initialized = true;
    
    if ( _register ) {
      _regId = iaxc_register( const_cast<char*>(_username.c_str()),
                              const_cast<char*>(_password.c_str()),
                              const_cast<char*>(_server.c_str()) );
        if( _regId == -1 ) {
            SG_LOG(SG_IO, SG_ALERT, "FGCom: cannot register iaxclient");
            return;
        }
    }

    /*
      Here we will create the list of available audio devices
      Each audio device has a name, an ID, and a list of capabilities
      If an audio device can output sound, available-output=true 
      If an audio device can input sound, available-input=true 

      /sim/fgcom/selected-input (int)
      /sim/fgcom/selected-output (int)

      /sim/fgcom/device[n]/id (int)
      /sim/fgcom/device[n]/name (string)
      /sim/fgcom/device[n]/available-input (bool)
      /sim/fgcom/device[n]/available-output (bool)
    */

    //FIXME: OpenAL driver use an hard-coded device
    //       so all following is unused finally until someone
    //       implement "multi-device" support in IAX audio driver
    SGPropertyNode *node     = fgGetNode("/sim/fgcom", 0, true);

    struct iaxc_audio_device *devs;
    int nDevs, input, output, ring;

    iaxc_audio_devices_get(&devs,&nDevs, &input, &output, &ring);

    for(int i=0; i<nDevs; i++ ) {
      SGPropertyNode *in_node = node->getChild("device", i, true);

      // devID
      _deviceID_node[i] = in_node->getChild("id", 0, true);
      _deviceID_node[i]->setIntValue(devs[i].devID);

      // name
      _deviceName_node[i] = in_node->getChild("name", 0, true);
      _deviceName_node[i]->setStringValue(devs[i].name);

      // input capability
      _deviceInput_node[i] = in_node->getChild("available-input", 0, true);
      if( devs[i].capabilities & IAXC_AD_INPUT )
        _deviceInput_node[i]->setBoolValue(true);
      else 
        _deviceInput_node[i]->setBoolValue(false);

      // output capability
      _deviceOutput_node[i] = in_node->getChild("available-output", 0, true);
      if( devs[i].capabilities & IAXC_AD_OUTPUT )
        _deviceOutput_node[i]->setBoolValue(true);
      else 
        _deviceOutput_node[i]->setBoolValue(false);

      // use default device at start
      if( devs[i].capabilities & IAXC_AD_INPUT_DEFAULT )
        _selectedInput_node->setIntValue(devs[i].devID);
      if( devs[i].capabilities & IAXC_AD_OUTPUT_DEFAULT )
        _selectedOutput_node->setIntValue(devs[i].devID);
    }

    // Mute the mic and set speaker at start
    iaxc_input_level_set( 0.0 );
    iaxc_output_level_set(getCurrentCommVolume());

    iaxc_millisleep(50);

    // Do the first call at start
    setupCommFrequency();
    connectToCommFrequency();
}

double FGCom::getCurrentCommVolume() const {
    double rv = 1.0;

    if (_speakerLevel_node)
        rv = _speakerLevel_node->getFloatValue();

    if (_commVolumeNode)
        rv = rv * _commVolumeNode->getFloatValue();

    return rv;
}
double FGCom::getCurrentFrequencyKhz() const {
    return 10 * static_cast<int>(_currentCommFrequency * 100 + 0.25); 
}

void FGCom::setupCommFrequency(int channel) {
    if (channel < 1) {
        if (_selected_comm_node != nullptr) {
            channel = _selected_comm_node->getIntValue();
        }
    }
    // disconnect if channel set to 0
    if (channel < 1) {
        if (_currentCallIdent != -1) {
            iaxc_dump_call_number(_currentCallIdent);
            SG_LOG(SG_IO, SG_INFO, "FGCom: disconnect as channel 0 " << _currentCallIdent);
            _currentCallIdent = -1;
        }
        _currentCommFrequency = 0.0;
        return;
    }

    if (channel > 0)
    {
        channel--; // adjust back to zero based index.
        SGPropertyNode *commRadioNode = fgGetNode("/instrumentation/")->getChild("comm", channel, false);
        if (commRadioNode) {
            SGPropertyNode *frequencyNode = commRadioNode->getChild("frequencies");
            if (_commVolumeNode)
                _commVolumeNode->removeChangeListener(this);
            _commVolumeNode = commRadioNode->getChild("volume");
            if (frequencyNode) {
                frequencyNode = frequencyNode->getChild("selected-mhz");
                if (frequencyNode) {
                    if (_commFrequencyNode != frequencyNode) {
                        if (_commFrequencyNode)
                            _commFrequencyNode->removeChangeListener(this);
                        _commFrequencyNode = frequencyNode;
                        _commFrequencyNode->addChangeListener(this);
                        _commVolumeNode->addChangeListener(this);
                    }
                    _currentCommFrequency = frequencyNode->getDoubleValue();
                    return;
                }
            }
        }
        SG_LOG(SG_IO, SG_INFO, "FGCom: setupCommFrequency node listener failed: channel " << channel);
    }

    if (_commFrequencyNode)
        _commFrequencyNode->removeChangeListener(this);
    SG_LOG(SG_IO, SG_INFO, "FGCom: setupCommFrequency invalid channel " << channel);

    _currentCommFrequency = 0.0;
}

void FGCom::connectToCommFrequency() {
    // ensure that the current comm is still in range
    if ((_currentCallFrequency > 0.0) && !isInRange(_currentCallFrequency)) {
        SG_LOG(SG_IO, SG_WARN, "FGCom: call out of range of: " << _currentCallFrequency);
        _currentCallFrequency = 0.0;
    }

    // don't connected (and disconnect if already connected) when tuned freq is 0
    if (_currentCommFrequency < 1.0) {
        if (_currentCallIdent != -1) {
            iaxc_dump_call_number(_currentCallIdent);
            SG_LOG(SG_IO, SG_INFO, "FGCom: disconnect as freq 0: current call " << _currentCallIdent);
            _currentCallIdent = -1;
        }        
        return;
    }

    if (_currentCallFrequency != _currentCommFrequency || _currentCallIdent == -1) {
        if (_currentCallIdent != -1) {
            iaxc_dump_call_number(_currentCallIdent);
            SG_LOG(SG_IO, SG_INFO, "FGCom: dump_call_number " << _currentCallIdent);
            _currentCallIdent = -1;
        }

        if (_currentCallIdent == -1)
        {
            std::string num = computePhoneNumber(_currentCommFrequency, getAirportCode(_currentCommFrequency));
            _processingTimer.stamp();
            if (!isInRange(_currentCommFrequency)) {
                if (_currentCallIdent != -1) {
                    SG_LOG(SG_IO, SG_INFO, "FGCom: disconnect call as not in range " << _currentCallIdent);
                    if (_currentCallIdent != -1) {
                        iaxc_dump_call_number(_currentCallIdent);
                        _currentCallIdent = -1;
                    }
                }
                return;
            }
            if (!num.empty()) {
                _currentCallIdent = iaxc_call(num.c_str());
                if (_currentCallIdent == -1)
                    SG_LOG(SG_IO, SG_DEBUG, "FGCom: cannot call " << num.c_str());
                else {
                    SG_LOG(SG_IO, SG_DEBUG, "FGCom: call established " << num.c_str() << " Freq: " << _currentCommFrequency);
                    _currentCallFrequency = _currentCommFrequency;
                }
            }
            else
                SG_LOG(SG_IO, SG_WARN, "FGCom: frequency " << _currentCommFrequency << " does not map to valid IAX address");
        }
    }
}

void FGCom::updateCall()
{
    if (_processingTimer.elapsedMSec() > IAX_DELAY) {
        _processingTimer.stamp();
        connectToCommFrequency();
    }
}



void FGCom::update(double dt)
{
    if (!_enabled || !_initialized) {
        return;
    }

    updateCall();
}



void FGCom::shutdown()
{
    if( !_enabled ) {
        return;
    }

  _initialized = false;
  _enabled = false;

  iaxc_set_event_callback(NULL);
  iaxc_unregister(_regId);
  iaxc_stop_processing_thread();
  iaxc_shutdown();

    // added to help debugging lingering IAX thread after fgOSCloseWindow
    flightgear::addSentryBreadcrumb("Did shutdown FGCom", "info");
    
  assert( static_instance == this );
  static_instance = NULL;
}

void FGCom::valueChanged(SGPropertyNode *prop)
{
  if (prop == _enabled_node) {
      bool isEnabled = prop->getBoolValue();
      if (_enabled == isEnabled) {
          return;
      }
      
    if( isEnabled ) {
      init();
      postinit();
    } else {
      shutdown();
    }
    return;
  }

  // avoid crash when properties change before FGCom::postinit
  // https://sourceforge.net/p/flightgear/codetickets/2574/
  if (!_initialized) {
      return;
  }

  if (prop == _commVolumeNode && _enabled) {
      if (_ptt_node->getIntValue()) {
          SG_LOG(SG_IO, SG_INFO, "FGCom: ignoring change comm volume as PTT pressed");
      }
      else
      {
          iaxc_input_level_set(0.0);
          iaxc_output_level_set(getCurrentCommVolume());
          SG_LOG(SG_IO, SG_INFO, "FGCom: change comm volume=" << _commVolumeNode->getFloatValue());
      }
  }

  if (prop == _selected_comm_node && _enabled) {
      setupCommFrequency();
      SG_LOG(SG_IO, SG_INFO, "FGCom: change comm frequency (selected node): set to " << _currentCommFrequency);
  }

  if (prop == _commFrequencyNode && _enabled) {
      setupCommFrequency();
      SG_LOG(SG_IO, SG_INFO, "FGCom: change comm frequency (property updated): set to " << _currentCommFrequency);
  }

  if (prop == _ptt_node && _enabled) {
      if (_ptt_node->getIntValue()) {
          // ensure that we are on the right channel by calling setupCommFrequency  
          setupCommFrequency();
          iaxc_output_level_set(0.0);
          iaxc_input_level_set(_micLevel_node->getFloatValue()); //0.0 = min , 1.0 = max
          _mpTransmitFrequencyNode->setValue(_currentCallFrequency * 1000000);
          _mpTransmitPowerNode->setValue(1.0);
          SG_LOG(SG_IO, SG_INFO, "FGCom: PTT active: " << _currentCallFrequency);
      }
      else {
          iaxc_output_level_set(getCurrentCommVolume());
          iaxc_input_level_set(0.0);
          SG_LOG(SG_IO, SG_INFO, "FGCom: PTT release: " << _currentCallFrequency << " vol=" << getCurrentCommVolume());
          _mpTransmitFrequencyNode->setValue(0);
          _mpTransmitPowerNode->setValue(0);
      }
  }

  if (prop == _test_node) {
    testMode( prop->getBoolValue() );
    return;
  }

  if (prop == _silenceThd_node && _initialized) {
    float silenceThd = prop->getFloatValue();
    SG_CLAMP_RANGE<float>( silenceThd, -60, 0 );
    iaxc_set_silence_threshold( silenceThd );
    return;
  }

  //FIXME: not implemented in IAX audio driver (audio_openal.c)
  if (prop == _micBoost_node && _initialized) {
    int micBoost = prop->getIntValue();
    SG_CLAMP_RANGE<int>( micBoost, 0, 1 );
    iaxc_mic_boost_set( micBoost ) ; // 0 = enabled , 1 = disabled
    return;
  }

  //FIXME: not implemented in IAX audio driver (audio_openal.c)
  if ((prop == _selectedInput_node || prop == _selectedOutput_node) && _initialized) {
    int selectedInput = _selectedInput_node->getIntValue();
    int selectedOutput = _selectedOutput_node->getIntValue();
    iaxc_audio_devices_set(selectedInput, selectedOutput, 0);
    return;
  }

  if (_listener_active)
    return;

  _listener_active++;

  if (prop == _speakerLevel_node && _enabled) {
    float speakerLevel = prop->getFloatValue();
    SG_CLAMP_RANGE<float>( speakerLevel, 0.0, 1.0 );
    _speakerLevel_node->setFloatValue(speakerLevel);
    iaxc_output_level_set(speakerLevel);
  }

  if (prop == _micLevel_node && _enabled) {
    float micLevel = prop->getFloatValue();
    SG_CLAMP_RANGE<float>( micLevel, 0.0, 1.0 );
    _micLevel_node->setFloatValue(micLevel);
    //iaxc_input_level_set(micLevel);
  }

  _listener_active--;
}



void FGCom::testMode(bool testMode)
{
  if(testMode && _initialized) {
    _enabled = false;
    iaxc_dump_all_calls();
    iaxc_input_level_set( 1.0 );
    iaxc_output_level_set( _speakerLevel_node->getFloatValue() );
    std::string num = computePhoneNumber(TEST_FREQ, NULL_ICAO);
    if( num.size() > 0 ) {
      iaxc_millisleep(IAX_DELAY);
      _currentCallIdent = iaxc_call(num.c_str());
    }
    if( _currentCallIdent == -1 )
      SG_LOG( SG_IO, SG_DEBUG, "FGCom: cannot call " << num.c_str() );
  } else {
    if( _initialized ) {
      iaxc_dump_all_calls();
      iaxc_millisleep(IAX_DELAY);
      iaxc_input_level_set( 0.0 );
      iaxc_output_level_set(getCurrentCommVolume());
      _currentCallIdent = -1;
      _enabled = true;
    }
  }
}



/*
  \param freq The requested frequency e.g 120.825
  \return The ICAO code as string e.g LFMV
*/

std::string FGCom::getAirportCode(const double& freq)
{
  SGGeod aircraftPos = globals->get_aircraft_position();

  for(size_t i=0; i<sizeof(special_freq)/sizeof(special_freq[0]); i++) { // Check if it's a special freq
    if(special_freq[i] == getCurrentFrequencyKhz()) {
      return NULL_ICAO;
    }
  }

  flightgear::CommStation* apt = flightgear::CommStation::findByFreq(getCurrentFrequencyKhz(), aircraftPos);
  if( !apt ) {
    apt = flightgear::CommStation::findByFreq(getCurrentFrequencyKhz() -10, aircraftPos); // Check for 8.33KHz
    if( !apt ) {
      return std::string();
    }
  }

  if( apt->type() == FGPositioned::FREQ_TOWER ) {
    _maxRange = MAX_TWR_RANGE;
    _minRange = MIN_GNDTWR_RANGE;
  } else if( apt->type() == FGPositioned::FREQ_GROUND ) {
    _maxRange = MAX_GND_RANGE;
    _minRange = MIN_GNDTWR_RANGE;
  } else {
    _maxRange = MAX_RANGE;
    _minRange = MIN_RANGE;
  }

  _aptPos = apt->geod();
  return apt->airport()->ident();
}



/*
  \param freq The requested frequency e.g 120.825
  \param iaco The associated ICAO code e.g LFMV
  \return The phone number as string i.e username:password@fgcom.flightgear.org/0176707786120825
*/

std::string FGCom::computePhoneNumber(const double& freq, const std::string& icao) const
{
  if( icao.empty() )
    return std::string(); 

  char phoneNumber[256];
  char exten[32];
  char tmp[5];

  /*Convert ICAO to ASCII */
  sprintf( tmp, "%4s", icao.c_str() );

  /*Built the phone number */
  sprintf( exten,
           "%02d%02d%02d%02d%02d%06d",
           01,
           tmp[0],
	   tmp[1],
           tmp[2],
           tmp[3],
	   (int) (freq * 1000 + 0.5) );
  exten[16] = '\0';

  snprintf( phoneNumber,
            sizeof (phoneNumber),
            "%s:%s@%s/%s",
            _username.c_str(),
            _password.c_str(),
	    _server.c_str(),
            exten);

  return phoneNumber;
}



/*
  \return A boolean value, 1=in range, 0=out of range
*/

bool FGCom::isInRange(const double &freq) const
{
    for(size_t i=0; i<sizeof(special_freq)/sizeof(special_freq[0]); i++) { // Check if it's a special freq
      if( (special_freq[i]) == getCurrentFrequencyKhz()) {
        return 1;
      }
    }

    SGGeod acftPos = globals->get_aircraft_position();
    double distNm = SGGeodesy::distanceNm(_aptPos, acftPos);
    double delta_elevation_ft = fabs(acftPos.getElevationFt() - _aptPos.getElevationFt());
    double rangeNm = 1.23 * sqrt(delta_elevation_ft);

    if (rangeNm > _maxRange) rangeNm = _maxRange;
    if (rangeNm < _minRange) rangeNm = _minRange;
    if( distNm > rangeNm )   return 0;
    return 1;
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGCom> registrantFGCom;
