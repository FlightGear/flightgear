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

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>

#include <Main/fg_props.hxx>

#include <utils/iaxclient/iaxclient.h>
#include "fgcom.hxx"


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
  _enabled_node            = node->getChild( "enabled", 0, true );
  _micBoost_node           = node->getChild( "mic-boost", 0, true );
  _server_node             = node->getChild( "server", 0, true );
  _speakerLevel_node       = node->getChild( "speaker-level", 0, true );
  _micLevel_node           = node->getChild( "mic-level", 0, true );

  SGPropertyNode *reg_node = node->getChild("register", 0, true);
  _register_node           = reg_node->getChild( "enabled", 0, true );
  _username_node           = reg_node->getChild( "username", 0, true );
  _password_node           = reg_node->getChild( "password", 0, true );

  _nav0_node               = fgGetNode("/instrumentation/nav[0]/frequencies/selected-mhz", true);
  _nav1_node               = fgGetNode("/instrumentation/nav[1]/frequencies/selected-mhz", true);
  _comm0_node              = fgGetNode("/instrumentation/comm[0]/frequencies/selected-mhz", true);
  _comm1_node              = fgGetNode("/instrumentation/comm[1]/frequencies/selected-mhz", true);
  _callsign_node           = fgGetNode("/sim/callsign", true);


  // Set default values if not provided
  if ( !_enabled_node->hasValue() )
      _enabled_node->setBoolValue(true);

  if ( !_micBoost_node->hasValue() )
      _micBoost_node->setIntValue(1);

  if ( !_server_node->hasValue() )
      _server_node->setStringValue("fgcom.flightgear.org");

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
  _enabled_node->addChangeListener(this);
  _micBoost_node->addChangeListener(this);
  _speakerLevel_node->addChangeListener(this);
  _micLevel_node->addChangeListener(this);
  _comm0_node->addChangeListener(this);
  _comm1_node->addChangeListener(this);
  _nav0_node->addChangeListener(this);
  _nav1_node->addChangeListener(this);

}



void FGCom::unbind()
{

}



void FGCom::init()
{

  _enabled  = _enabled_node->getBoolValue();

  /*
    Here we want initiate some IAX state


  // INITIALIZE IAX
  if(iaxc_initialize(4))
    SG_LOG(SG_IO, SG_ALERT, "FGCom: cannot initialize iaxclient!");

  // SET MIC BOOST
  iaxc_mic_boost_set ( _micBoost_node->getIntValue() );

  // SET CALLER ID
  std::string callerid = computeCallerId(_callsign);
  //FIXME iaxc_set_callerid( _username.c_str(), callerid.c_str() );
  iaxc_set_callerid ( const_cast<char*>( _username ), const_cast<char*>( callerid ) ); 

  // SET FORMATS
  iaxc_set_formats ( codec, IAXC_FORMAT_ULAW | IAXC_FORMAT_GSM | IAXC_FORMAT_SPEEX );

  // SET CALLBACK FUNCTION ????
  iaxc_set_event_callback ( iaxc_callback );

  // ????
  iaxc_start_processing_thread ();

  // REGISTER IAX
  if ( _register ) {
    _regId = iaxc_register (const_cast <char*>( _username ),
                   const_cast <char*>( _password ),
                   const_cast <char*>( _server ));
    if( _regId == -1 )
      SG_LOG(SG_IO, SG_ALERT, "FGCom: cannot register iaxclient!");
  }

*/
 

}



void FGCom::update(double dt)
{

  if( _enabled ) {

    if( _comm0Changed ) {
      // iaxc_dump_call_number( _callComm0 );
      const double freq = _comm0_node->getDoubleValue();
      const std::string icao = "searchICAO!";
      std::string num = computePhoneNumber(icao, freq); // num = "username:password@fgcom.flightgear.org/0165676568122825"
      // _callComm0 = iaxc_call(const num.c_str());
      _comm0Changed = false;
    }

    if( _comm1Changed ) {
      // iaxc_dump_call_number( _callComm1 );
      const double freq = _comm1_node->getDoubleValue();
      const std::string icao = "searchICAO!";
      std::string num = computePhoneNumber(icao, freq); // num = "username:password@fgcom.flightgear.org/0165676568122825"
      // _callComm1 = iaxc_call(const num.c_str());
      _comm1Changed = false;
    }

    if( _nav0Changed ) {
      // iaxc_dump_call_number( _callNav0 );
      const double freq = _nav0_node->getDoubleValue();
      const std::string icao = "searchICAO!";
      std::string num = computePhoneNumber(icao, freq); // num = "username:password@fgcom.flightgear.org/0165676568122825"
      // _callNav0 = iaxc_call(const num.c_str());
      _nav0Changed = false;
    }

    if( _nav1Changed ) {
      // iaxc_dump_call_number( _callNav1 );
      const double freq = _nav1_node->getDoubleValue();
      const std::string icao = "searchICAO!";
      std::string num = computePhoneNumber(icao, freq); // num = "username:password@fgcom.flightgear.org/0165676568122825"
      // _callNav1 = iaxc_call(const num.c_str());
      _nav1Changed = false;
    }



  } else {
    shutdown();
  }

}



void FGCom::shutdown()
{

  // HANGUP ALL CALLS
  iaxc_dump_call ();

  // UNREGISTER IAX
  //iaxc_unregister (_regId);

  // SHUTDOWN IAX
  //iaxc_shutdown ();

}



void FGCom::valueChanged(SGPropertyNode *prop)
{

  /*
   Here we want handle :
     - Mic boost change
     - Frequency change
     - Mic/speaker volume change
  */

  if (prop == _enabled_node) {
    SG_LOG( SG_IO, SG_ALERT, "FGCom enabled= " << prop->getBoolValue() );
    _enabled = prop->getBoolValue();
  }

  if (prop == _micBoost_node) {
    SG_LOG( SG_IO, SG_ALERT, "FGCom mic-boost= " << prop->getIntValue() );
    int micBoost = prop->getFloatValue();
    SG_CLAMP_RANGE<int>( micBoost, 0, 1 );
    // iaxc_mic_boost_set( micBoost ) ; 0 = enabled , 1 = disabled
  }

  if (prop == _speakerLevel_node) {
    SG_LOG( SG_IO, SG_ALERT, "FGCom speaker-level= " << prop->getFloatValue() );
    float speakerLevel = prop->getFloatValue();
    SG_CLAMP_RANGE<float>( speakerLevel, 0.0, 1.0 );
    // iaxc_output_level_set(speakerLevel); 0.0 = min , 1.0 = max
  }

  if (prop == _micLevel_node) {
    SG_LOG( SG_IO, SG_ALERT, "FGCom mic-level= " << prop->getFloatValue() );
    float micLevel = prop->getFloatValue();
    SG_CLAMP_RANGE<float>( micLevel, 0.0, 1.0 );
    // iaxc_input_level_set(micLevel); 0.0 = min , 1.0 = max
  }

  if (_listener_active)
    return;

  _listener_active++;

  if (prop == _comm0_node) {
    SG_LOG( SG_IO, SG_ALERT, "FGCom comm[0]/freq= " << prop->getDoubleValue() );
    _comm0Changed = true;
  }

  if (prop == _comm1_node) {
    SG_LOG( SG_IO, SG_ALERT, "FGCom comm[1]/freq= " << prop->getDoubleValue() );
    _comm1Changed = true;
  }

  if (prop == _nav0_node) {
    SG_LOG( SG_IO, SG_ALERT, "FGCom nav[0]/freq= " << prop->getDoubleValue() );
    _nav0Changed = true;
  }

  if (prop == _nav1_node) {
    SG_LOG( SG_IO, SG_ALERT, "FGCom nav[1]/freq= " << prop->getDoubleValue() );
    _nav1Changed = true;
  }

  _listener_active--;

}



std::string FGCom::computePhoneNumber(const std::string& icao, const double& freq) const
{
  return "toto";
}

std::string FGCom::computeCallerId(const std::string& callSign) const
{
  return "tata";
}










