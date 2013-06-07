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

#include <stdio.h>

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <ATC/CommStation.hxx>
#include <Airports/airport.hxx>
#include <Navaids/navlist.hxx>

#include "fgcom.hxx"

#define DEFAULT_RANGE 50.0
#define DEFAULT_SERVER "clemaez.dyndns.org"


FGCom::FGCom()
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
  _chat_node               = fgGetNode("/sim/multiplay/chat", true); // Because we can do it, we do it :)

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
  _chat_node->addChangeListener(this);
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
  /*
    Here we want initiate some IAX state
    Also we will provide the list of available devices
    WARNING: this_must_ be executed after sound system is totally initialized !
  */

  // INITIALIZE IAX
  if( iaxc_initialize(4) )
    SG_LOG(SG_IO, SG_ALERT, "FGCom: cannot initialize iaxclient!");

  // SET MIC BOOST
  // FIXME: //Not used by OpenAL (should we implement it ?)
  //iaxc_mic_boost_set( _micBoost_node->getIntValue() );

  // SET CALLER ID
  // FIXME: is it really necessary ?
  std::string callerID = _callsign_node->getStringValue();
  iaxc_set_callerid( const_cast<char*>(callerID.c_str()), const_cast<char*>(callerID.c_str()) ); 

  // SET FORMATS
  iaxc_set_formats( IAXC_FORMAT_GSM, IAXC_FORMAT_GSM );

  // SET CALLBACK FUNCTION ????
  // FIXME: to be done...
  //iaxc_set_event_callback( iaxc_callback );

  // INTERNAL PROCESSING OF IAX
  iaxc_start_processing_thread ();

  // REGISTER IAX
  // FIXME: require server-side implementation.
  //        AFAIK no one is ready for this feature... keep it ?
  if ( _register ) {
    _regId = iaxc_register( const_cast<char*>(_username.c_str()),
                            const_cast<char*>(_password.c_str()),
                            const_cast<char*>(_server.c_str()) );
    if( _regId == -1 )
      SG_LOG(SG_IO, SG_ALERT, "FGCom: cannot register iaxclient!");
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
  //       so all following is unused finally
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

  iaxc_millisleep(300);

  if( _enabled ) {
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
}



void FGCom::update(double dt)
{

  if( _enabled ) {

    if( _comm0Changed ) {
      SG_LOG( SG_IO, SG_INFO, "FGCom manage comm0 change" );
      iaxc_dump_call_number( _callComm0 );
      iaxc_millisleep(300);
      const double freq = _comm0_node->getDoubleValue();
      std::string num = computePhoneNumber(freq, getAirportCode(freq));
      if( num.size() > 0 ) {
        SG_LOG( SG_IO, SG_INFO, "FGCom comm[0] number=" << num );
        _callComm0 = iaxc_call(num.c_str());
      }
      if( _callComm0 == -1 )
        SG_LOG( SG_IO, SG_ALERT, "FGCom cannot call comm[0] freq" );
        _comm0Changed = false;
    }

    if( _comm1Changed ) {
      SG_LOG( SG_IO, SG_INFO, "FGCom manage comm1 change" );
      iaxc_dump_call_number( _callComm1 );
      iaxc_millisleep(300);
      const double freq = _comm1_node->getDoubleValue();
      std::string num = computePhoneNumber(freq, getAirportCode(freq));
      if( num.size() > 0 ) {
        SG_LOG( SG_IO, SG_INFO, "FGCom comm[1] number=" << num );
        _callComm1 = iaxc_call(num.c_str());
      }
      if( _callComm1 == -1 )
        SG_LOG( SG_IO, SG_ALERT, "FGCom cannot call comm[1] freq" );
      _comm1Changed = false;
    }

    if( _nav0Changed ) {
      SG_LOG( SG_IO, SG_INFO, "FGCom manage nav0 change" );
      iaxc_dump_call_number( _callNav0 );
      iaxc_millisleep(300);
      const double freq = _nav0_node->getDoubleValue();
      std::string num = computePhoneNumber(freq, getVorCode(freq));
      if( num.size() > 0 ) {
        SG_LOG( SG_IO, SG_INFO, "FGCom nav[0] number=" << num );
        _callNav0 = iaxc_call(num.c_str());
      }
      if( _callNav0 == -1 )
        SG_LOG( SG_IO, SG_ALERT, "FGCom cannot call nav[0] freq" );
      _nav0Changed = false;
    }

    if( _nav1Changed ) {
      SG_LOG( SG_IO, SG_INFO, "FGCom manage nav1 change" );
      iaxc_dump_call_number( _callNav1 );
      iaxc_millisleep(300);
      const double freq = _nav1_node->getDoubleValue();
      std::string num = computePhoneNumber(freq, getVorCode(freq));
      if( num.size() > 0 ) {
        SG_LOG( SG_IO, SG_INFO, "FGCom nav[1] number=" << num );
        _callNav1 = iaxc_call(num.c_str());
      }
      if( _callNav1 == -1 )
        SG_LOG( SG_IO, SG_ALERT, "FGCom cannot call nav[1] freq" );
      _nav1Changed = false;
    }

    if( _chatChanged ) {
      SG_LOG( SG_IO, SG_INFO, "FGCom manage chat change" );
      const std::string msg = _chat_node->getStringValue();
      iaxc_send_text_call( _callComm0, msg.c_str() );
      _chatChanged = false;
    }

    //FIXME: should be better with a listener
    if( _ptt0_node->getBoolValue() ) {
      iaxc_input_level_set( _micLevel_node->getFloatValue() ); //0.0 = min , 1.0 = max
      iaxc_output_level_set( 0.0 );
    } else {
      iaxc_input_level_set( 0.0 );
      iaxc_output_level_set( _speakerLevel_node->getFloatValue() ); //0.0 = min , 1.0 = max    
    }

    //FIXME: need to handle range:
    //       check - for each nav0, nav1, comm0, comm1 - if the freq is out of range
    //       if it's out of range we must dump the call
    //       if it was out of range and now in range, we must re activate the call
    //       if it was in range and still in range, nothing to do :)

  } //if( _enabled )

}



void FGCom::shutdown()
{
  SG_LOG( SG_IO, SG_INFO, "FGCom shutdown()" );
  _enabled = false;

  // UNREGISTER IAX
  iaxc_unregister(_regId);

  // STOP IAX THREAD
  iaxc_stop_processing_thread();

  // SHUTDOWN IAX
  iaxc_shutdown();
}



void FGCom::valueChanged(SGPropertyNode *prop)
{
  /*
   Here we want :
     - Handle mic boost change
     - Detect frequency change
     - Handle mic/speaker volume change
  */

  if (prop == _enabled_node) {
    SG_LOG( SG_IO, SG_INFO, "FGCom enabled= " << prop->getBoolValue() );
    if( prop->getBoolValue() ) {
      //FIXME: how to handle enabled/disabled ?
      //shutdown();
      //init();
      //_enabled = true;
    } else {
      //shutdown();
    }
    return;
  }

  if (prop == _micBoost_node) {
    int micBoost = prop->getIntValue();
    SG_LOG( SG_IO, SG_INFO, "FGCom mic-boost= " << micBoost );
    SG_CLAMP_RANGE<int>( micBoost, 0, 1 );
    iaxc_mic_boost_set( micBoost ) ; // 0 = enabled , 1 = disabled
    return;
  }

  if (prop == _selectedInput_node || prop == _selectedOutput_node) {
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

  if (prop == _speakerLevel_node) {
    float speakerLevel = prop->getFloatValue();
    SG_LOG( SG_IO, SG_INFO, "FGCom speaker-level= " << speakerLevel );
    SG_CLAMP_RANGE<float>( speakerLevel, 0.0, 1.0 );
    _speakerLevel_node->setFloatValue(speakerLevel);
  }

  if (prop == _micLevel_node) {
    float micLevel = prop->getFloatValue();
    SG_LOG( SG_IO, SG_INFO, "FGCom mic-level= " << micLevel );
    SG_CLAMP_RANGE<float>( micLevel, 0.0, 1.0 );
    _micLevel_node->setFloatValue(micLevel);
  }

  if (prop == _comm0_node) {
    if( _currentComm0 != prop->getDoubleValue() ) { // Because property-swap trigger valueChanged() twice
      SG_LOG( SG_IO, SG_INFO, "FGCom comm[0]/freq= " << prop->getDoubleValue() );
      _currentComm0 = prop->getDoubleValue();
      _comm0Changed = true;
    }
  }

  if (prop == _comm1_node) {
    if( _currentComm1 != prop->getDoubleValue() ) { // Because property-swap trigger valueChanged() twice
      SG_LOG( SG_IO, SG_INFO, "FGCom comm[1]/freq= " << prop->getDoubleValue() );
      _currentComm1 = prop->getDoubleValue();
      _comm1Changed = true;
    }
  }

  if (prop == _nav0_node) {
    if( _currentNav0 != prop->getDoubleValue() ) { // Because property-swap trigger valueChanged() twice
      SG_LOG( SG_IO, SG_INFO, "FGCom nav[0]/freq= " << prop->getDoubleValue() );
      _currentNav0 = prop->getDoubleValue();
      _nav0Changed = true;
    }
  }

  if (prop == _nav1_node) {
    if( _currentNav1 != prop->getDoubleValue() ) { // Because property-swap trigger valueChanged() twice
      SG_LOG( SG_IO, SG_INFO, "FGCom nav[1]/freq= " << prop->getDoubleValue() );
      _currentNav1 = prop->getDoubleValue();
      _nav1Changed = true;
    }
  }

  if (prop == _chat_node) {
    if( std::string(prop->getStringValue()).size() > 0 ) { // Don't manage empty message
      SG_LOG( SG_IO, SG_INFO, "FGCom chat= " << prop->getStringValue() );
      _chatChanged = true;
    }
  }

  _listener_active--;
}



int FGCom::iaxc_callback(iaxc_event e)
{
  switch(e.type) {
    case IAXC_EVENT_TEXT:
      return textEvent(e.ev.text.type, e.ev.text.callNo, e.ev.text.message);
      break;
    default:
      return 0;
  }
}



int FGCom::textEvent(int type, int callNo, char *message)
{
  // FIXME: must be displayed on screen and added in chat history
  //_chat_node->setStringValue(message);
  return 1;
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

  FGNavRecord *vor = FGNavList::findByFreq( freq, aircraftPos, &filter);
  if( !vor ) {
    SG_LOG( SG_IO, SG_INFO, "FGCom getVorCode: not found" );
    return std::string();
  }
  SG_LOG( SG_IO, SG_INFO, "FGCom getVorCode: found " << vor->get_ident(); );
  //double lon = vor->get_lon();
  //double lat = vor->get_lat();
  //double elev = vor->get_elev_ft();

  return vor->get_ident();;
}



/*
  \param freq The requested frequency e.g 120.825
  \param iaco The associated ICAO code e.g LFMV
  \return The phone number as string i.e username:password@fgcom.flightgear.org/0176707786120825
*/

std::string FGCom::computePhoneNumber(const double& freq, const std::string& icao) const
{
  if( !icao.size() )
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



int FGCom::isOutOfRange()
{
  //SGGeod aircraftPos = globals->get_aircraft_position();

  return 0;
}




