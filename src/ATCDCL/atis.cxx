// atis.cxx - routines to generate the ATIS info string
// This is the implementation of the FGATIS class
//
// Written by David Luff, started October 2001.
//
// Copyright (C) 2001  David C Luff - david.luff@nottingham.ac.uk
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

/////
///// TODO:  _Cumulative_ sky coverage.
///// TODO:  wind _gust_
///// TODO:  more-sensible encoding of voice samples
/////       u-law?  outright synthesis?
/////

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "atis.hxx"
#include "atis_lexicon.hxx"

#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>

#include <stdlib.h> // atoi()
#include <stdio.h>  // sprintf
#include <string>
#include <iostream>

#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>
#include <Environment/atmosphere.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Airports/runways.hxx>
#include <Airports/dynamics.hxx>


#include "ATCutils.hxx"
#include "ATISmgr.hxx"

using std::string;
using std::map;
using std::cout;
using std::cout;
using boost::ref;
using boost::tie;

FGATIS::FGATIS(const string& commbase) :
  transmission(""),
  trans_ident(""),
  old_volume(0),
  atis_failed(false),
  msg_OK(0),
  attention(0),
  _prev_display(0),
  _commbase(commbase)
{
  fgTie("/environment/attention", this, (int_getter)0, &FGATIS::attend);

///////////////
// FIXME:  This would be more flexible and more extensible
// if the mappings were taken from an XML file, not hard-coded ...
// ... although having it in a .hxx file is better than nothing.
//
// Load the remap list from the .hxx file:
  using namespace lex;
# define NIL ""
# define REMAP(from,to) _remap[#from] = to;
# include "atis_remap.hxx"
# undef REMAP
# undef NIL

#ifdef ATIS_TEST
  SG_LOG(SG_ATC, SG_ALERT, "ATIS initialized");
#endif
}

// Hint:
// http://localhost:5400/environment/attention?value=1&submit=update

FGATIS::~FGATIS() {
  fgUntie("/environment/attention");
}

FGATCVoice* FGATIS::GetVoicePointer()
{
    FGATISMgr* pAtisMgr = globals->get_ATIS_mgr();
    if (!pAtisMgr)
    {
        SG_LOG(SG_ATC, SG_ALERT, "ERROR! No ATIS manager! Oops...");
        return NULL;
    }

    return pAtisMgr->GetVoicePointer(ATIS);
}

void FGATIS::Init() {
// Nothing to see here.  Move along.
}

void
FGATIS::attend (int attn)
{
  attention = attn;
#ifdef ATMO_TEST
  int flag = fgGetInt("/sim/logging/atmo");
  if (flag) {
    FGAltimeter().check_model();
	FGAltimeter().dump_stack();
  }
#endif
}


// Main update function - checks whether we are displaying or not the correct message.
void FGATIS::Update(double dt) {
  cur_time = globals->get_time_params()->get_cur_time();
  msg_OK = (msg_time < cur_time);

#ifdef ATIS_TEST
  if (msg_OK || _display != _prev_display) {
    cout << "ATIS Update: " << _display << "  " << _prev_display
      << "  len: " << transmission.length()
      << "  oldvol: " << old_volume
      << "  dt: " << dt << endl;
    msg_time = cur_time;
  }
#endif

  if(_display)
  {
    string prop = _commbase + "/volume";
    double volume = globals->get_props()->getDoubleValue(prop.c_str());

// Check if we need to update the message
// - basically every hour and if the weather changes significantly at the station
// If !_prev_display, the radio had been detuned for a while and our
// "transmission" variable was lost when we were de-instantiated.
    int changed = GenTransmission(!_prev_display, attention);
    TreeOut(msg_OK);
    if (changed || volume != old_volume) {
      //cout << "ATIS calling ATC::render  volume: " << volume << endl;
      Render(transmission, volume, _commbase, true);
      old_volume = volume;
    }
  } else {
// We shouldn't be displaying
    //cout << "ATIS.CXX - calling NoRender()..." << endl;
    NoRender(_commbase);
  }
  _prev_display = _display;
  attention = 0;
}

string uppercase(const string &s) {
  string rslt(s);
  for(string::iterator p = rslt.begin(); p != rslt.end(); p++){
    *p = toupper(*p);
  }
  return rslt;
}

// Replace all occurrences of a given word.
// Words in the original string must be separated by hyphens (not spaces).
// We check for the word as given, and for the all-caps version thereof.
string replace_word(const string _orig, const string _www, const string _nnn){
// The following are so we can match words at the beginning
// and end of the string.
  string orig = "-" + _orig + "-";
  string www = "-" + _www + "-";
  string nnn = "-" + _nnn + "-";

  size_t where(0);
  for ( ; (where = orig.find(www, where)) != string::npos ; ) {
    orig.replace(where, www.length(), nnn);
    where += nnn.length();
  }
  
  www = uppercase(www);
  for ( ; (where = orig.find(www, where)) != string::npos ; ) {
    orig.replace(where, www.length(), nnn);
    where += nnn.length();
  }
  where = orig.length();
  return orig.substr(1, where-2);
}

// Normally the interval is 1 hour, 
// but you can shorten it for testing.
const int minute(60);		// measured in seconds
#ifdef ATIS_TEST
  const int ATIS_interval(2*minute);
#else
  const int ATIS_interval(60*minute);
#endif

// FIXME:  This is heuristic.  It gets the right answer for
// more than 90% of the world's airports, which is a lot
// better than nothing ... but it's not 100%.
// We know "most" of the world uses millibars,
// but the US, Canada and *some* other places use inches of mercury,
// but (a) we have not implemented a reliable method of
// ascertaining which airports are in the US, let alone
// (b) ascertaining which other places use inches.
//
int Apt_US_CA(const string id) {
// Assume all IDs have length 3 or 4.
// No counterexamples have been seen.
  if (id.length() == 4) {
    if (id.substr(0,1) == "K") return 1;
    if (id.substr(0,2) == "CY") return 1;
  }
  for (string::const_iterator ptr = id.begin(); ptr != id.end();  ptr++) {
    if (isdigit(*ptr)) return 1;
  }
  return 0;
}

// Generate the actual broadcast ATIS transmission.
// Regen means regenerate the /current/ transmission.
// Special means generate a new transmission, with a new sequence.
// Returns 1 if we actually generated something.
int FGATIS::GenTransmission(const int regen, const int special) {
  using namespace atmodel;
  using namespace lex;

  string BRK = ".\n";
  string PAUSE = " / ";

  int interval = _type == ATIS ?
        ATIS_interval   // ATIS updated hourly
      : 2*minute;	// AWOS updated more frequently

  FGAirport* apt = FGAirport::findByIdent(ident);
  int sequence = apt->getDynamics()->updateAtisSequence(interval, special);
  if (!regen && sequence > LTRS) {
//xx      if (msg_OK) cout << "ATIS:  no change: " << sequence << endl;
//xx      msg_time = cur_time;
    return 0;   // no change since last time
  }

  const int bs(100);
  char buf[bs];
  string time_str = fgGetString("sim/time/gmt-string");
  string hours, mins;
  string phonetic_seq_string;

  transmission = "";

  int US_CA = Apt_US_CA(ident);

  if (!US_CA) {
// UK CAA radiotelephony manual indicates ATIS transmissions start
// with "This is ..." 
    transmission += This_is + " ";
  } else {
    // In the US they just start with the airport name.
  }

  // SG_LOG(SG_ATC, SG_ALERT, "ATIS: facility name: " << name);

// Note that at this point, multi-word facility names
// will sometimes contain hyphens, not spaces.
  
  vector<string> name_words;
  boost::split(name_words, name, boost::is_any_of(" -"));

  for (vector<string>::const_iterator wordp = name_words.begin();
                wordp != name_words.end(); wordp++) {
    string word(*wordp);
// Remap some abbreviations that occur in apt.dat, to
// make things nicer for the text-to-speech system:
    for (MSS::const_iterator replace = _remap.begin();
          replace != _remap.end(); replace++) {
      // Due to inconsistent capitalisation in the apt.dat file, we need
      // to do a case-insensitive comparison here.
      string tmp1 = word, tmp2 = replace->first;
      boost::algorithm::to_lower(tmp1);
      boost::algorithm::to_lower(tmp2);
      if (tmp1 == tmp2) {
        word = replace->second;
        break;
      }
    }
    transmission += word + " ";
  }

  if (_type == ATIS /* as opposed to AWOS */) {
    transmission += airport_information + " ";
  } else {
    transmission += Automated_weather_observation + " ";
  }

  phonetic_seq_string = GetPhoneticLetter(sequence);  // Add the sequence letter
  transmission += phonetic_seq_string + BRK;

// Warning - this is fragile if the time string format changes
  hours = time_str.substr(0,2).c_str();
  mins  = time_str.substr(3,2).c_str();
// speak each digit separately:
  transmission += ConvertNumToSpokenDigits(hours + mins);
  transmission += " " + zulu + " " + weather + BRK;

  transmission += wind + ": ";

  double wind_speed = fgGetDouble("/environment/config/boundary/entry[0]/wind-speed-kt");
  double wind_dir = fgGetDouble("/environment/config/boundary/entry[0]/wind-from-heading-deg");
  while (wind_dir <= 0) wind_dir += 360;
// The following isn't as bad a kludge as it might seem.
// It combines the magvar at the /aircraft/ location with
// the wind direction in the environment/config array.
// But if the aircraft is close enough to the station to
// be receiving the ATIS signal, this should be a good-enough
// approximation.  For more-distant aircraft, the wind_dir
// shouldn't be corrected anyway.
// The less-kludgy approach would be to use the magvar associated
// with the station, but that is not tabulated in the stationweather
// structure as it stands, and computing it would be expensive.
// Also note that as it stands, there is only one environment in
// the entire FG universe, so the aircraft environment is the same
// as the station environment anyway.
  wind_dir -= fgGetDouble("/environment/magnetic-variation-deg");       // wind_dir now magnetic
  if (wind_speed == 0) {
// Force west-facing rwys to be used in no-wind situations
// which is consistent with Flightgear's initial setup:
      wind_dir = 270;
      transmission += " " + light_and_variable;
  } else {
      // FIXME: get gust factor in somehow
      snprintf(buf, bs, "%03.0f", 5*SGMiscd::round(wind_dir/5));
      transmission += ConvertNumToSpokenDigits(buf);

      snprintf(buf, bs, "%1.0f", wind_speed);
      transmission += " " + at + " " + ConvertNumToSpokenDigits(buf) + BRK;
  }

// Sounds better with a pause in there:
  transmission += PAUSE;

  int did_some(0);
  int did_ceiling(0);

  for (int layer = 0; layer <= 4; layer++) {
    snprintf(buf, bs, "/environment/clouds/layer[%i]/coverage", layer);
    string coverage = fgGetString(buf);
    if (coverage == clear) continue;
    snprintf(buf, bs, "/environment/clouds/layer[%i]/thickness-ft", layer);
    if (fgGetDouble(buf) == 0) continue;
    snprintf(buf, bs, "/environment/clouds/layer[%i]/elevation-ft", layer);
    double ceiling = int(fgGetDouble(buf) - _geod.getElevationFt());
    if (ceiling > 12000) continue;

// BEWARE:  At the present time, the environment system has no
// way (so far as I know) to represent a "thin broken" or
// "thin overcast" layer.  If/when such things are implemented
// in the environment system, code will have to be written here
// to handle them.

// First, do the prefix if any:
    if (coverage == scattered || coverage == few) {
      if (!did_some) {
        transmission += "   " + Sky_condition + ": ";
        did_some++;
      }
    } else /* must be a ceiling */  if (!did_ceiling) {
      transmission += "   " + Ceiling + ": ";
      did_ceiling++;
      did_some++;
    } else {
      transmission += "   ";    // no prefix required
    }
    int cig00  = int(SGMiscd::round(ceiling/100));  // hundreds of feet
    if (cig00) {
      int cig000 = cig00/10;
      cig00 -= cig000*10;       // just the hundreds digit
      if (cig000) {
        snprintf(buf, bs, "%i", cig000);
        transmission += ConvertNumToSpokenDigits(buf);
        transmission += " " + thousand + " ";
      }
      if (cig00) {
        snprintf(buf, bs, "%i", cig00);
        transmission += ConvertNumToSpokenDigits(buf);
        transmission += " " + hundred + " ";
      }
    } else {
      // Should this be "sky obscured?"
      transmission += " " + zero + " ";     // not "zero hundred"
    }
    transmission += coverage + BRK;
  }
  if (!did_some) transmission += "   " + Sky + " " + clear + BRK;

  transmission += Temperature + ": ";
  double Tsl = fgGetDouble("/environment/temperature-sea-level-degc");
  int temp = int(SGMiscd::round(FGAtmo().fake_T_vs_a_us(_geod.getElevationFt(), Tsl)));
  if(temp < 0) {
      transmission += lex::minus + " ";
  }
  snprintf(buf, bs, "%i", abs(temp));
  transmission += ConvertNumToSpokenDigits(buf);
  if (US_CA) transmission += " " + Celsius;
  transmission += " " + dewpoint + " ";
  double dpsl = fgGetDouble("/environment/dewpoint-sea-level-degc");
  temp = int(SGMiscd::round(FGAtmo().fake_dp_vs_a_us(dpsl, _geod.getElevationFt())));
  if(temp < 0) {
      transmission += lex::minus + " ";
  }
  snprintf(buf, bs, "%i", abs(temp));
  transmission += ConvertNumToSpokenDigits(buf);
  if (US_CA) transmission += " " + Celsius;
  transmission += BRK;

  transmission += Visibility + ": ";
  double visibility = fgGetDouble("/environment/config/boundary/entry[0]/visibility-m");
  visibility /= atmodel::sm;    // convert to statute miles
  if (visibility < 0.25) {
    transmission += less_than_one_quarter;
  } else if (visibility < 0.5) {
    transmission += one_quarter;
  } else if (visibility < 0.75) {
    transmission += one_half;
  } else if (visibility < 1.0) {
    transmission += three_quarters;
  } else if (visibility >= 1.5 && visibility < 2.0) {
    transmission += one_and_one_half;
  } else {
    // integer miles
    if (visibility > 10) visibility = 10;
    sprintf(buf, "%i", int(.5 + visibility));
    transmission += ConvertNumToSpokenDigits(buf);
  }
  transmission += BRK;

  double myQNH;
  double Psl = fgGetDouble("/environment/pressure-sea-level-inhg");
  {
    double press, temp;
    
    tie(press, temp) = PT_vs_hpt(_geod.getElevationM(), Psl*inHg, Tsl + freezing);
#if 0
    SG_LOG(SG_ATC, SG_ALERT, "Field P: " << press << "  T: " << temp);
    SG_LOG(SG_ATC, SG_ALERT, "based on elev " << elev 
                                << "  Psl: " << Psl
                                << "  Tsl: " << Tsl);
#endif
    myQNH = FGAtmo().QNH(_geod.getElevationM(), press);
  }

// Convert to millibars for most of the world (not US, not CA)
  if((!US_CA) && fgGetBool("/sim/atc/use-millibars")) {
    transmission += QNH + ": ";
    myQNH /= mbar;
    if  (myQNH > 1000) myQNH -= 1000;       // drop high digit
    snprintf(buf, bs, "%03.0f", myQNH);
    transmission += ConvertNumToSpokenDigits(buf) + " " + millibars + BRK;
  } else {
    transmission += Altimeter + ": ";
    double asetting = myQNH / inHg;         // use inches of mercury
    asetting *= 100.;                       // shift two decimal places
    snprintf(buf, bs, "%04.0f", asetting);
    transmission += ConvertNumToSpokenDigits(buf) + BRK;
  }

  if (_type == ATIS /* as opposed to AWOS */) {
    const FGAirport* apt = fgFindAirportID(ident);
    if (apt) {
      string rwy_no = apt->getActiveRunwayForUsage()->ident();
      if(rwy_no != "NN") {
        transmission += Landing_and_departing_runway + " ";
        transmission += ConvertRwyNumToSpokenString(rwy_no) + BRK;
#ifdef ATIS_TEST
        if (msg_OK) {
          msg_time = cur_time;
          cout << "In atis.cxx, r.rwy_no: " << rwy_no
             << " wind_dir: " << wind_dir << endl;
        }
#endif
      }
    }
    transmission += On_initial_contact_advise_you_have_information + " ";
    transmission += phonetic_seq_string;
    transmission += "... " + BRK + PAUSE + PAUSE;
  }
  transmission_readable = transmission;
// Take the previous readable string and munge it to
// be relatively-more acceptable to the primitive tts system.
// Note that : ; and . are among the token-delimeters recognized
// by the tts system.
  for (size_t where;;) {
    where = transmission.find_first_of(":.");
    if (where == string::npos) break;
    transmission.replace(where, 1, PAUSE);
  }
  return 1;
}

// Put the transmission into the property tree.
// You can see it by pointing a web browser
// at the property tree.  The second comm radio is:
// http://localhost:5400/instrumentation/comm[1]
//
// (Also, if in debug mode, dump it to the console.)
void FGATIS::TreeOut(int msg_OK){
    string prop = _commbase + "/atis";
    globals->get_props()->setStringValue(prop.c_str(),
      ("<pre>\n" + transmission_readable + "</pre>\n").c_str());
    SG_LOG(SG_ATC, SG_DEBUG, "**** ATIS active on: " << prop <<
           "transmission: " << transmission_readable);
}
