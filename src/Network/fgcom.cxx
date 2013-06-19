// fgcom.cxx -- FGCom: Voice communication
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

// flightgear includes
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <ATC/CommStation.hxx>
#include <Airports/airport.hxx>
#include <Navaids/navlist.hxx>

#include <utils/iaxclient/lib/iaxclient.h>


#define NUM_CALLS 4
#define MAX_RANGE 150.0
#define DEFAULT_SERVER "clemaez.dyndns.org"

static FGCom* static_instance = NULL;

static int IAXCallbackFunction(iaxc_event event)
{
    switch (event.type)
    {
    case IAXC_EVENT_STATE:
        static_instance->stateChanged(event.ev.call.state);
        break;
            
    default: return 0;
    }
    
    return 1; // success
}

FGCom::FGCom() :
    _register(true)
{
  _listener_active = 0;
}



FGCom::~FGCom()
{
}



void FGCom::bind()
{

  // Get some properties
  SGPropertyNode *node     = fgGetNode("/sim/fgcom", 0, true);
  _server_node             = node->getChild( "server", 0, true );
  _enabled_node            = node->getChild( "enabled", 0, true );
  _micBoost_node           = node->getChild( "mic-boost", 0, true );
  _micLevel_node           = node->getChild( "mic-level", 0, true );
  _speakerLevel_node       = node->getChild( "speaker-level", 0, true );
  _selectedInput_node      = node->getChild( "device-input", 0, true );
  _selectedOutput_node     = node->getChild( "device-output", 0, true );

  SGPropertyNode *reg_node = node->getChild("register", 0, true);
  _register_node           = reg_node->getChild( "enabled", 0, true );
  _username_node           = reg_node->getChild( "username", 0, true );
  _password_node           = reg_node->getChild( "password", 0, true );

  _nav0_node               = fgGetNode("/instrumentation/nav[0]/frequencies/selected-mhz", true);
  _nav1_node               = fgGetNode("/instrumentation/nav[1]/frequencies/selected-mhz", true);
  _comm0_node              = fgGetNode("/instrumentation/comm[0]/frequencies/selected-mhz", true);
  _comm1_node              = fgGetNode("/instrumentation/comm[1]/frequencies/selected-mhz", true);
  _ptt0_node               = fgGetNode("/instrumentation/comm[0]/ptt", true); //FIXME: what about /instrumentation/comm[1]/ptt ?
  _callsign_node           = fgGetNode("/sim/multiplay/callsign", true);

  // Set default values if not provided
  if ( !_enabled_node->hasValue() )
      _enabled_node->setBoolValue(true);

  if ( !_micBoost_node->hasValue() )
      _micBoost_node->setIntValue(1);

  if ( !_server_node->hasValue() )
      _server_node->setStringValue(DEFAULT_SERVER);

  if ( !_speakerLevel_node->hasValue() )
      _speakerLevel_node->setFloatValue(1.0);

  if ( !_micLevel_node->hasValue() )
      _micLevel_node->setFloatValue(1.0);

  if ( !_register_node->hasValue() )
      _register_node->setBoolValue(false);

  if ( !_username_node->hasValue() )
      _username_node->setStringValue("guest");

  if ( !_password_node->hasValue() )
      _password_node->setStringValue("guest");

  // Add some listeners
  _selectedOutput_node->addChangeListener(this);
  _selectedInput_node->addChangeListener(this);
  _speakerLevel_node->addChangeListener(this);
  _micBoost_node->addChangeListener(this);
  _micLevel_node->addChangeListener(this);
  _enabled_node->addChangeListener(this);
  _comm0_node->addChangeListener(this);
  _comm1_node->addChangeListener(this);
  _nav0_node->addChangeListener(this);
  _nav1_node->addChangeListener(this);
  _ptt0_node->addChangeListener(this);
}



void FGCom::unbind()
{
}



void FGCom::init()
{
  // Link some property often used in order to 
  //     reduce reading impact in property tree
  _enabled          = _enabled_node->getBoolValue();
  _server           = _server_node->getStringValue();
  _username         = _username_node->getStringValue();
  _password         = _password_node->getStringValue();

  _currentComm0     = _comm0_node->getDoubleValue();
  _currentComm1     = _comm1_node->getDoubleValue();
  _currentNav0      = _nav0_node->getDoubleValue();
  _currentNav1      = _nav1_node->getDoubleValue();

  _comm0Changed     = false;
  _comm1Changed     = false;
  _nav0Changed      = false;
  _nav1Changed      = false;
}



void FGCom::postinit()
{
  if( !_enabled ) {
      return;
  }
    
    //WARNING: this _must_ be executed after sound system is totally initialized !

      if( iaxc_initialize(NUM_CALLS) ) {
          SG_LOG(SG_IO, SG_ALERT, "FGCom: cannot initialize iaxclient!");
          _enabled = false;
          return;
      }
    
    assert(static_instance == NULL);
    static_instance = this;
    iaxc_set_event_callback(IAXCallbackFunction);
    
    // FIXME: To be implemented in IAX audio driver
    //iaxc_mic_boost_set( _micBoost_node->getIntValue() );
    iaxc_set_formats( IAXC_FORMAT_GSM, IAXC_FORMAT_GSM );
    iaxc_start_processing_thread ();
    
    if ( _register ) {
      _regId = iaxc_register( const_cast<char*>(_username.c_str()),
                              const_cast<char*>(_password.c_str()),
                              const_cast<char*>(_server.c_str()) );
        if( _regId == -1 ) {
            SG_LOG(SG_IO, SG_ALERT, "FGCom: cannot register iaxclient!");
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

    iaxc_millisleep(50);

    // Do the first call at start
    const double freq = _comm0_node->getDoubleValue();
    std::string num = computePhoneNumber(freq, getAirportCode(freq));
    if( num.size() > 0 ) {
      SG_LOG( SG_IO, SG_INFO, "FGCom comm[0] number=" << num );
      _callComm0 = iaxc_call(num.c_str());
    }
    if( _callComm0 == -1 )
      SG_LOG( SG_IO, SG_ALERT, "FGCom cannot call comm[0] freq" );

    
}

void FGCom::updateCall(bool& changed, int& phoneNumber, double freqMHz)
{
    if (!changed) {
        return;
    }
    
    SG_LOG( SG_IO, SG_INFO, "FGCom manage change" );
    changed = false; // FIXME, out-params are confusing

    iaxc_dump_call_number( phoneNumber );
    iaxc_millisleep(50);
    std::string num = computePhoneNumber(freqMHz, getAirportCode(freqMHz));
    if( !isInRange() )
        return;
    if( !num.empty() ) {
        SG_LOG( SG_IO, SG_INFO, "FGCom number=" << num );
        phoneNumber = iaxc_call_ex(num.c_str(), _callsign.c_str(), NULL, 0 /* no video */);
    }

    if( phoneNumber == -1 )
        SG_LOG( SG_IO, SG_ALERT, "FGCom cannot call comm[0] freq" );
}

void FGCom::update(double dt)
{

  if ( !_enabled ) {
      return;
  }
    
    updateCall(_comm0Changed, _callComm0, _comm0_node->getDoubleValue());
    updateCall(_comm1Changed, _callComm1, _comm1_node->getDoubleValue());
    
    // updateCall(_nav0Changed, _callNav0, _nav0_node->getDoubleValue());
    // updateCall(_nav1Changed, _callNav1, _nav1_node->getDoubleValue());
    
    if( !isInRange() ) {
      iaxc_dump_call();
    }
    
    //FIXME: need to handle range:
    //       check - for each nav0, nav1, comm0, comm1 - if the freq is out of range
    //       if it's out of range we must dump the call
    //       if it was out of range and now in range, we must re activate the call
    //       if it was in range and still in range, nothing to do :)


}

void FGCom::shutdown()
{
  SG_LOG( SG_IO, SG_INFO, "FGCom shutdown()" );
  _enabled = false;

    iaxc_set_event_callback(NULL);
  iaxc_unregister(_regId);
  iaxc_stop_processing_thread();
  iaxc_shutdown();
    
    assert(static_instance == this);
    static_instance = NULL;
}



void FGCom::valueChanged(SGPropertyNode *prop)
{
  if (prop == _enabled_node) {
    SG_LOG( SG_IO, SG_INFO, "FGCom enabled= " << prop->getBoolValue() );
    if( prop->getBoolValue() ) {
      init();
      postinit();
    } else {
      shutdown();
    }
    return;
  }

  if (prop == _ptt0_node && _enabled) {
    if( _ptt0_node->getBoolValue() ) {
      iaxc_input_level_set( _micLevel_node->getFloatValue() ); //0.0 = min , 1.0 = max
      iaxc_output_level_set( 0.0 );
    } else {
      iaxc_input_level_set( 0.0 );
      iaxc_output_level_set( _speakerLevel_node->getFloatValue() );
    }
  }

  //FIXME: not implemented in IAX audio driver (audio_openal.c)
  if (prop == _micBoost_node && _enabled) {
    int micBoost = prop->getIntValue();
    SG_LOG( SG_IO, SG_INFO, "FGCom mic-boost= " << micBoost );
    SG_CLAMP_RANGE<int>( micBoost, 0, 1 );
    iaxc_mic_boost_set( micBoost ) ; // 0 = enabled , 1 = disabled
    return;
  }

  //FIXME: not implemented in IAX audio driver (audio_openal.c)
  if ((prop == _selectedInput_node || prop == _selectedOutput_node) && _enabled) {
    int selectedInput = _selectedInput_node->getIntValue();
    int selectedOutput = _selectedOutput_node->getIntValue();
    SG_LOG( SG_IO, SG_INFO, "FGCom selected-input= " << selectedInput );
    SG_LOG( SG_IO, SG_INFO, "FGCom selected-output= " << selectedOutput );
    iaxc_audio_devices_set(selectedInput, selectedOutput, 0);
    return;
  }

  if (_listener_active)
    return;

  _listener_active++;

  if (prop == _speakerLevel_node && _enabled) {
    float speakerLevel = prop->getFloatValue();
    SG_LOG( SG_IO, SG_INFO, "FGCom speaker-level= " << speakerLevel );
    SG_CLAMP_RANGE<float>( speakerLevel, 0.0, 1.0 );
    _speakerLevel_node->setFloatValue(speakerLevel);
    iaxc_output_level_set(speakerLevel);
  }

  if (prop == _micLevel_node && _enabled) {
    float micLevel = prop->getFloatValue();
    SG_LOG( SG_IO, SG_INFO, "FGCom mic-level= " << micLevel );
    SG_CLAMP_RANGE<float>( micLevel, 0.0, 1.0 );
    _micLevel_node->setFloatValue(micLevel);
    iaxc_input_level_set(micLevel);
  }

  if (prop == _comm0_node) {
    if( _currentComm0 != prop->getDoubleValue() ) {
      SG_LOG( SG_IO, SG_INFO, "FGCom comm[0]/freq= " << prop->getDoubleValue() );
      _currentComm0 = prop->getDoubleValue();
      _comm0Changed = true;
    }
  }

  if (prop == _comm1_node) {
    if( _currentComm1 != prop->getDoubleValue() ) {
      SG_LOG( SG_IO, SG_INFO, "FGCom comm[1]/freq= " << prop->getDoubleValue() );
      _currentComm1 = prop->getDoubleValue();
      _comm1Changed = true;
    }
  }
/*
  if (prop == _nav0_node) {
    if( _currentNav0 != prop->getDoubleValue() ) {
      SG_LOG( SG_IO, SG_INFO, "FGCom nav[0]/freq= " << prop->getDoubleValue() );
      _currentNav0 = prop->getDoubleValue();
      _nav0Changed = true;
    }
  }

  if (prop == _nav1_node) {
    if( _currentNav1 != prop->getDoubleValue() ) {
      SG_LOG( SG_IO, SG_INFO, "FGCom nav[1]/freq= " << prop->getDoubleValue() );
      _currentNav1 = prop->getDoubleValue();
      _nav1Changed = true;
    }
  }
*/

  _listener_active--;
}


/*
  \param freq The requested frequency e.g 120.825
  \return The ICAO code as string e.g LFMV
*/

std::string FGCom::getAirportCode(const double& freq) const
{
  SGGeod aircraftPos = globals->get_aircraft_position();

  int freqKhz = 10 * static_cast<int>(freq * 100 + 0.25);

  flightgear::CommStation* apt = flightgear::CommStation::findByFreq(freqKhz, aircraftPos);
  if( !apt ) {
    SG_LOG( SG_IO, SG_INFO, "FGCom getAirportCode: not found" );
    return std::string();
  }
  SG_LOG( SG_IO, SG_INFO, "FGCom getAirportCode: found " << apt->airport()->ident() );

  return apt->airport()->ident();
}



/*
  \param freq The requested frequency e.g 112.7
  \return The ICAO code as string e.g ITS
*/

std::string FGCom::getVorCode(const double& freq) const
{
  SGGeod aircraftPos = globals->get_aircraft_position();
  FGNavList::TypeFilter filter(FGPositioned::VOR);

  FGNavRecord* vor = FGNavList::findByFreq( freq, aircraftPos, &filter);
  if( !vor ) {
    SG_LOG( SG_IO, SG_INFO, "FGCom getVorCode: not found" );
    return std::string();
  }
  SG_LOG( SG_IO, SG_INFO, "FGCom getVorCode: found " << vor->get_ident(); );

  return vor->get_ident();
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



bool FGCom::isInRange()
{
/*
  //SGGeod airportPos;
  //SGGeod aircraftPos = globals->get_aircraft_position();

  // Get airport position

  // Calculate aircraft range
  double delta_elevation_ft = fabs(aircraft_altitude_ft - airport_elevation_ft);
  double range_nm = 1.23 * sqrt(delta_elevation_ft);
  if(range_nm > MAX_RANGE)
    range_nm = MAX_RANGE;

  // Calculate distance btw aircraft <-> airport

  // Return 1 if in range else 0
  if(toto > range_nm)
     return 0;
*/
  return 1;
}

void FGCom::stateChanged(int newState)
{
    SG_LOG(SG_IO, SG_INFO, "FGCom transition to state:" << newState);
}


