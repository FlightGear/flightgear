// atis.cxx - routines to generate the ATIS info string
// This is the implementation of the FGATIS class
//
// Written by David Luff, started October 2001.
// Extended by Thorsten Brehm, October 2012.
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
#include <simgear/misc/strutils.hxx>

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

#include <ATC/CommStation.hxx>
#include <Navaids/navrecord.hxx>

#include "ATCutils.hxx"
#include "ATISmgr.hxx"

using std::string;
using std::map;
using std::cout;
using std::cout;
using boost::ref;
using boost::tie;
using flightgear::CommStation;

FGATIS::FGATIS(const std::string& name, int num) :
  _name(name),
  _num(num),
  _cb_attention(this, &FGATIS::attend, fgGetNode("/environment/attention", true)),
  transmission(""),
  trans_ident(""),
  old_volume(0),
  atis_failed(false),
  msg_time(0),
  cur_time(0),
  msg_OK(0),
  _attention(false),
  _check_transmission(true),
  _prev_display(0),
  _time_before_search_sec(0),
  _last_frequency(0)
{
  _root         = fgGetNode("/instrumentation", true)->getNode(_name, num, true);
  _volume       = _root->getNode("volume",true);
  _serviceable  = _root->getNode("serviceable",true);

  if (name != "nav")
  {
      // only drive "operable" for non-nav instruments (nav radio drives this separately)
      _operable = _root->getNode("operable",true);
      _operable->setBoolValue(false);
  }

  _electrical   = fgGetNode("/systems/electrical/outputs",true)->getNode(_name,num, true);
  _atis         = _root->getNode("atis",true);
  _freq         = _root->getNode("frequencies/selected-mhz",true);

  // current position
  _lon_node  = fgGetNode("/position/longitude-deg", true);
  _lat_node  = fgGetNode("/position/latitude-deg",  true);
  _elev_node = fgGetNode("/position/altitude-ft",   true);

  // backward compatibility: some properties may not exist (but default to "ON")
  if (!_serviceable->hasValue())
      _serviceable->setBoolValue(true);
  if (!_electrical->hasValue())
      _electrical->setDoubleValue(24.0);

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

    _report.psl = 0;
}

// Hint:
// http://localhost:5400/environment/attention?value=1&submit=update

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

void FGATIS::init()
{
// Nothing to see here.  Move along.
}

void FGATIS::reinit()
{
    _time_before_search_sec = 0;
    _check_transmission = true;
}

void
FGATIS::attend(SGPropertyNode* node)
{
  if (node->getBoolValue())
      _attention = true;
#ifdef ATMO_TEST
  int flag = fgGetInt("/sim/logging/atmo");
  if (flag) {
    FGAltimeter().check_model();
    FGAltimeter().dump_stack();
  }
#endif
}


// Main update function - checks whether we are displaying or not the correct message.
void FGATIS::update(double dt) {
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

  double volume = 0;
  if ((_electrical->getDoubleValue() > 8) && _serviceable->getBoolValue())
  {
      // radio is switched on and OK
      if (_operable.valid())
          _operable->setBoolValue(true);

      _check_transmission |= search(dt);

      if (_display)
      {
          volume = _volume->getDoubleValue();
      }
  }
  else
  {
      // radio is OFF
      if (_operable.valid())
          _operable->setBoolValue(false);
      _time_before_search_sec = 0;
  }

  if (volume > 0.05)
  {
    bool changed = false;
    if (_check_transmission)
    {
        _check_transmission = false;
        // Check if we need to update the message
        // - basically every hour and if the weather changes significantly at the station
        // If !_prev_display, the radio had been detuned for a while and our
        // "transmission" variable was lost when we were de-instantiated.
        if (genTransmission(!_prev_display, _attention))
        {
            // update output property
            treeOut(msg_OK);
            changed = true;
        }
    }

    if (changed || volume != old_volume) {
      // audio output enabled
      Render(transmission, volume, _name, true);
      old_volume = volume;
    }
    _prev_display = _display;
  } else {
    // silence
    NoRender(_name);
    _prev_display = false;
  }
  _attention = false;

  FGATC::update(dt);
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
  
  www = simgear::strutils::uppercase(www);
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
bool Apt_US_CA(const string id)
{
    // Assume all IDs have length 3 or 4.
    // No counterexamples have been seen.
    if (id.length() == 4) {
        if (id.substr(0,1) == "K") return true;
        if (id.substr(0,2) == "CY") return true;
    }
    for (string::const_iterator ptr = id.begin(); ptr != id.end();  ptr++) {
        if (isdigit(*ptr)) return true;
    }
    return false;
}

// voice spacers
static const string BRK = ".\n";
static const string PAUSE = " / ";

/** Generate the actual broadcast ATIS transmission.
*   'regen' triggers a regeneration of the /current/ transmission.
*   'forceUpdate' generates a new transmission, with a new sequence.
*   Returns 1 if we actually generated something.
*/
bool FGATIS::genTransmission(const int regen, bool forceUpdate)
{
    using namespace lex;

    // ATIS updated hourly, AWOS updated more frequently
    int interval = _type == ATIS ? ATIS_interval : 2*minute;

    // check if pressure has changed significantly and we need to update ATIS
    double Psl = fgGetDouble("/environment/pressure-sea-level-inhg");
    if (fabs(Psl-_report.psl) >= 0.15)
        forceUpdate = true;

    FGAirport* apt = FGAirport::findByIdent(ident);
    int sequence = apt->getDynamics()->updateAtisSequence(interval, forceUpdate);
    if (!regen && sequence > LTRS) {
        //xx      if (msg_OK) cout << "ATIS:  no change: " << sequence << endl;
        //xx    msg_time = cur_time;
        return false;   // no change since last time
    }

    _report.psl = Psl;
    transmission = "";

    // collect data and create report
    createReport(apt);

    // add facility name
    genFacilityInfo();

    if (_type == ATIS) {
        // ATIS phraseology starts with "... airport information"
        transmission += airport_information + " ";
    } else {
        // AWOS
        transmission += Automated_weather_observation + " ";
    }

    string phonetic_seq_string = GetPhoneticLetter(sequence);  // Add the sequence letter
    transmission += phonetic_seq_string + BRK;

    genTimeInfo();

    // some warnings may appear at the beginning
    genWarnings(-1);

    if (_type == ATIS) // as opposed to AWOS
        genRunwayInfo(apt);

    // some warnings may appear after runway info
    genWarnings(0);

    // transition level
    genTransitionLevel(apt);

    // weather
    if (!_report.concise)
        transmission += Weather + BRK;

    genWindInfo();

    // clouds and visibility
    {
        string vis_info, cloud_info;
        bool v = genVisibilityInfo(vis_info);
        bool c = genCloudInfo(cloud_info);
        _report.cavok = !(v || c);
        if (!_report.cavok)
        {
            // there is some visibility or cloud restriction
            transmission += vis_info + cloud_info;
        }
        else
        {
            // Abbreviation CAVOK vs full "clouds and visibility..." does not really depend on
            // US vs rest of the world, it really seems to depend on the airport. Just use
            // it as a heuristic.
            if ((_report.US_CA)||(_report.concise))
                transmission += cav_ok + BRK;
            else
                transmission += clouds_and_visibility_OK + BRK;
        }
    }

    // precipitation
    genPrecipitationInfo();

    // temperature
    genTemperatureInfo();

    // pressure
    genPressureInfo();

    // TODO check whether "no significant change" applies - somehow...
    transmission += No_sig + BRK; // sounds better with festival than "nosig"

    // some warnings may appear at the very end
    genWarnings(1);

    if ((!_report.concise)|| _report.US_CA)
        transmission += Advise_on_initial_contact_you_have_information;
    else
        transmission += information;
    transmission += " " + phonetic_seq_string + ".";

    if (!_report.US_CA)
    {
        // non-US ATIS ends with "out!"
        transmission += " " + out;
    }

    // Pause in between two messages must be 3-5 seconds
    transmission += " / / / / / / / / ";

    /////////////////////////////////////////////////////////
    // post-processing
    /////////////////////////////////////////////////////////
    transmission_readable = transmission;

    // Take the previous readable string and munge it to
    // be relatively-more acceptable to the primitive tts system.
    // Note that : ; and . are among the token-delimiters recognized
    // by the tts system.
    for (size_t where;;) {
        where = transmission.find_first_of(":.");
        if (where == string::npos) break;
        transmission.replace(where, 1, PAUSE);
    }

    return true;
}

/** Collect (most of) the data and create report.
 */
void FGATIS::createReport(const FGAirport* apt)
{
    // check country
    _report.US_CA = Apt_US_CA(ident);

    // switch to enable brief ATIS message (really depends on the airport)
    _report.concise = fgGetBool("/sim/atis/concise-reports", false);

    _report.ils = false;

    // time information
    string time_str = fgGetString("sim/time/gmt-string");
    // Warning - this is fragile if the time string format changes
    _report.hours = time_str.substr(0,2).c_str();
    _report.mins  = time_str.substr(3,2).c_str();

    // pressure/temperature
    {
        double press, temp;
        double Tsl = fgGetDouble("/environment/temperature-sea-level-degc");
        tie(press, temp) = PT_vs_hpt(_geod.getElevationM(), _report.psl*atmodel::inHg, Tsl + atmodel::freezing);
  #if 0
        SG_LOG(SG_ATC, SG_ALERT, "Field P: " << press << "  T: " << temp);
        SG_LOG(SG_ATC, SG_ALERT, "based on elev " << elev
                                  << "  Psl: " << Psl
                                  << "  Tsl: " << Tsl);
  #endif
        _report.qnh = FGAtmo().QNH(_geod.getElevationM(), press);
        _report.temp = int(SGMiscd::round(FGAtmo().fake_T_vs_a_us(_geod.getElevationFt(), Tsl)));
    }

    // dew point
    double dpsl = fgGetDouble("/environment/dewpoint-sea-level-degc");
    _report.dewpoint = int(SGMiscd::round(FGAtmo().fake_dp_vs_a_us(dpsl, _geod.getElevationFt())));

    // precipitation
    _report.rain_norm = fgGetDouble("environment/rain-norm");
    _report.snow_norm = fgGetDouble("environment/snow-norm");

    // NOTAMs
    _report.notam = 0;
    if (fgGetBool("/sim/atis/random-notams", true))
    {
        _report.notam = fgGetInt("/sim/atis/notam-id", 0); // fixed NOTAM for testing/debugging only
        if (!_report.notam)
        {
            // select pseudo-random NOTAM (changes every hour, differs for each airport)
            char cksum = 0;
            string name = apt->getName();
            for(string::iterator p = name.begin(); p != name.end(); p++)
            {
                cksum += *p;
            }
            cksum ^= atoi(_report.hours.c_str());
            _report.notam = cksum % 12; // 12 intentionally higher than number of available NOTAMs, so they don't appear too often
            // debugging
            //fgSetInt("/sim/atis/selected-notam", _report.notam);
        }
    }
}

void FGATIS::genPrecipitationInfo(void)
{
    using namespace lex;

    double rain_norm = _report.rain_norm;
    double snow_norm = _report.snow_norm;

    // report rain or snow - which ever is worse
    if (rain_norm > 0.7)
        transmission += heavy    + " " + rain + BRK;
    else
    if (snow_norm > 0.7)
        transmission += heavy    + " " + snow + BRK;
    else
    if (rain_norm > 0.4)
        transmission += moderate + " " + rain + BRK;
    else
    if (snow_norm > 0.4)
        transmission += moderate + " " + snow + BRK;
    else
    if (rain_norm > 0.2)
        transmission += light    + " " + rain + BRK;
    else
    if (snow_norm > 0.05)
        transmission += light    + " " + snow + BRK;
    else
    if (rain_norm > 0.05)
        transmission += light    + " " + drizzle + BRK;
}

void FGATIS::genTimeInfo(void)
{
    using namespace lex;

    if (!_report.concise)
        transmission += Time + " ";

    // speak each digit separately:
    transmission += ConvertNumToSpokenDigits(_report.hours + _report.mins);
    transmission += " " + zulu + BRK;
}

bool FGATIS::genVisibilityInfo(string& vis_info)
{
    using namespace lex;

    double visibility = fgGetDouble("/environment/config/boundary/entry[0]/visibility-m");
    bool IsMax = false;
    bool USE_KM = !_report.US_CA;

    vis_info += Visibility + ": ";
    if (USE_KM)
    {
        visibility /= 1000.0;    // convert to statute miles
        // integer kilometers
        if (visibility >= 9.5)
        {
            visibility = 10;
            IsMax = true;
        }
        snprintf(buf, sizeof(buf), "%i", int(.5 + visibility));
        // "kelometers" instead of "kilometers" since the festival language generator doesn't get it right otherwise
        vis_info += ConvertNumToSpokenDigits(buf) + " " + kelometers;
    }
    else
    {
        visibility /= atmodel::sm;    // convert to statute miles
        if (visibility < 0.25) {
          vis_info += less_than_one_quarter;
        } else if (visibility < 0.5) {
          vis_info += one_quarter;
        } else if (visibility < 0.75) {
          vis_info += one_half;
        } else if (visibility < 1.0) {
          vis_info += three_quarters;
        } else if (visibility >= 1.5 && visibility < 2.0) {
          vis_info += one_and_one_half;
        } else {
          // integer miles
          if (visibility > 9.5)
          {
              visibility = 10;
              IsMax = true;
          }
          snprintf(buf, sizeof(buf), "%i", int(.5 + visibility));
          vis_info += ConvertNumToSpokenDigits(buf);
        }
    }
    if (IsMax)
    {
        vis_info += " " + or_more;
    }
    vis_info += BRK;
    return !IsMax;
}

void FGATIS::addTemperature(int Temp)
{
    if (Temp < 0)
        transmission += lex::minus + " ";
    else
    if (Temp > 0)
    {
        transmission += lex::plus + " ";
    }
    snprintf(buf, sizeof(buf), "%i", abs(Temp));
    transmission += ConvertNumToSpokenDigits(buf);
    if (_report.US_CA)
        transmission += " " + lex::Celsius;
}

void FGATIS::genTemperatureInfo()
{
    // temperature
    transmission += lex::Temperature + ": ";
    addTemperature(_report.temp);

    // dewpoint
    transmission += BRK + lex::Dewpoint + ": ";
    addTemperature(_report.dewpoint);

    transmission += BRK;
}

bool FGATIS::genCloudInfo(string& cloud_info)
{
    using namespace lex;

    bool did_some = false;
    bool did_ceiling = false;

    for (int layer = 0; layer <= 4; layer++) {
      snprintf(buf, sizeof(buf), "/environment/clouds/layer[%i]/coverage", layer);
      string coverage = fgGetString(buf);
      if (coverage == clear)
          continue;
      snprintf(buf, sizeof(buf), "/environment/clouds/layer[%i]/thickness-ft", layer);
      if (fgGetDouble(buf) == 0)
          continue;
      snprintf(buf, sizeof(buf), "/environment/clouds/layer[%i]/elevation-ft", layer);
      double ceiling = int(fgGetDouble(buf) - _geod.getElevationFt());
      if (ceiling > 12000)
          continue;

  // BEWARE:  At the present time, the environment system has no
  // way (so far as I know) to represent a "thin broken" or
  // "thin overcast" layer.  If/when such things are implemented
  // in the environment system, code will have to be written here
  // to handle them.

      // First, do the prefix if any:
      if (coverage == scattered || coverage == few) {
        if (!did_some) {
          if (_report.concise)
              cloud_info += Clouds + ": ";
          else
              cloud_info += Sky_condition + ": ";
          did_some = true;
        }
      } else /* must be a ceiling */  if (!did_ceiling) {
        cloud_info += "   " + Ceiling + ": ";
        did_ceiling = true;
        did_some = true;
      } else {
        cloud_info += "   ";    // no prefix required
      }
      int cig00  = int(SGMiscd::round(ceiling/100));  // hundreds of feet
      if (cig00) {
        int cig000 = cig00/10;
        cig00 -= cig000*10;       // just the hundreds digit
        if (cig000) {
          snprintf(buf, sizeof(buf), "%i", cig000);
          cloud_info += ConvertNumToSpokenDigits(buf);
          cloud_info += " " + thousand + " ";
        }
        if (cig00) {
          snprintf(buf, sizeof(buf), "%i", cig00);
          cloud_info += ConvertNumToSpokenDigits(buf);
          cloud_info += " " + hundred + " ";
        }
      } else {
        // Should this be "sky obscured?"
        cloud_info += " " + zero + " ";     // not "zero hundred"
      }
      cloud_info += coverage + BRK;
    }
    if (!did_some)
        cloud_info += "   " + Sky + " " + clear + BRK;
    return did_some;
}

void FGATIS::genFacilityInfo(void)
{
    if ((!_report.US_CA)&&(!_report.concise))
    {
        // UK CAA radiotelephony manual indicates ATIS transmissions start
        // with "This is ...", while US just starts with airport name.
        transmission += lex::This_is + " ";
    }

    // SG_LOG(SG_ATC, SG_ALERT, "ATIS: facility name: " << name);

    // Note that at this point, multi-word facility names
    // will sometimes contain hyphens, not spaces.
    std::vector<std::string> name_words;
    boost::split(name_words, name, boost::is_any_of(" -"));

    for( std::vector<string>::const_iterator wordp = name_words.begin();
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
}

void FGATIS::genWindInfo(void)
{
    using namespace lex;

    transmission += Wind + ": ";

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
        snprintf(buf, sizeof(buf), "%03.0f", 5*SGMiscd::round(wind_dir/5));
        transmission += ConvertNumToSpokenDigits(buf);
        if (!_report.concise)
            transmission += " " + degrees;
        transmission += " ";
        snprintf(buf, sizeof(buf), "%1.0f", wind_speed);
        transmission += at + " " + ConvertNumToSpokenDigits(buf);
        if (!_report.concise)
            transmission += " " + knots;
    }
    transmission += BRK;
}

void FGATIS::genTransitionLevel(const FGAirport* apt)
{
    double hPa = _report.qnh/atmodel::mbar;

    /* Transition level is the flight level above which aircraft must use standard pressure and below
     * which airport pressure settings must be used.
     * Following definitions are taken from German ATIS:
     *      QNH <=  977 hPa: TRL 80
     *      QNH <= 1013 hPa: TRL 70
     *      QNH >  1013 hPa: TRL 60
     * (maybe differs slightly for other countries...)
     */
    int tl = 60;
    if (hPa <= 977)
        tl = 80;
    else
    if (hPa <= 1013)
        tl = 70;

    // add an offset to the transition level for high altitude airports (just guessing here,
    // seems reasonable)
    double elevationFt = apt->getElevation();
    int e = int(elevationFt / 1000.0);
    if (e >= 3)
    {
        // TL steps in 10(00)ft
        tl += (e-2)*10;
    }

    snprintf(buf, sizeof(buf), "%02i", tl);
    transmission += lex::Transition_level + ": " + ConvertNumToSpokenDigits(buf) + BRK;
}

void FGATIS::genPressureInfo(void)
{
    using namespace lex;

    // hectopascal for most of the world (not US, not CA)
    if(!_report.US_CA) {
        double hPa = _report.qnh/atmodel::mbar;
        transmission += QNH + ": ";
        snprintf(buf, sizeof(buf), "%03.0f", _report.qnh / atmodel::mbar);
        transmission += ConvertNumToSpokenDigits(buf);
        // "hectopascal" replaced "millibars" in new ATIS standard since 2011
        if  ((!_report.concise)||(hPa < 1000))
            transmission += " " + hectopascal; // "hectopascal" must be provided for values below 1000 (to avoid confusion with inHg)

        // Many (European) airports (with lots of US traffic?) provide both, hPa and inHg announcements.
        // Europeans keep the "decimal" in inHg readings to make the distinction to hPa even more apparent.
        // hPa/inHg separated by "equals" or "or" with some airports
        if (_report.concise)
            transmission += " " + equals + " ";
        else
            transmission += " " + Or + " ";
        snprintf(buf, sizeof(buf), "%04.2f", _report.qnh / atmodel::inHg);
        transmission += ConvertNumToSpokenDigits(buf);
        if (!_report.concise)
            transmission += " " + inches;
        transmission += BRK;
    } else {
        // use inches of mercury for US/CA
        transmission += Altimeter + ": ";
        double asetting = _report.qnh / atmodel::inHg;
        // shift two decimal places, US/CA airports omit the "decimal" in inHg settings
        asetting *= 100.;
        snprintf(buf, sizeof(buf), "%04.0f", asetting);
        transmission += ConvertNumToSpokenDigits(buf);
    }

    transmission += BRK;
}

void FGATIS::genRunwayInfo(const FGAirport* apt)
{
    using namespace lex;

    if (!apt)
        return;

    FGRunway* rwy = apt->getActiveRunwayForUsage();
    if (!rwy)
        return;

    string rwy_no = rwy->ident();
    if(rwy_no != "NN")
    {
        FGNavRecord* ils = rwy->ILS();
        if (ils)
        {
            _report.ils = true;
            transmission += Expect_I_L_S_approach + " "+ runway + " "+ConvertRwyNumToSpokenString(rwy_no) + BRK;
            if (fgGetBool("/sim/atis/announce-ils-frequency", false))
            {
                // this is handy - but really non-standard (so disabled by default)
                snprintf(buf, sizeof(buf), "%5.2f", ils->get_freq()/100.0);
                transmission += I_L_S + " " + ConvertNumToSpokenDigits(buf) + BRK;
            }
        }
        else
        {
            transmission += Expect_visual_approach + " "+ runway + " "+ConvertRwyNumToSpokenString(rwy_no) + BRK;
        }

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

void FGATIS::genWarnings(int position)
{
    using namespace lex;
    bool dayTime = (fgGetDouble("/sim/time/sun-angle-rad") < 1.57);

    if (position == -1) // warnings at beginning of ATIS
    {
        // bird related warnings at day-time only (birds are VFR-only! ;-) )
        if (dayTime)
        {
            if (_report.notam == 1)
                transmission += Attention + ": " + flock_of_birds + " " + in_the_vicinity_of_the_airport + BRK;
            else
            if (_report.notam == 2)
                transmission += Attention + ": " + bird_activity + " " + in_the_vicinity_of_the_airport + BRK;
        }
    }
    else
    if (position == 0)  // warnings after runway messages
    {
        if ((_report.notam == 3)&&(_report.ils))
        {
            // "__I_LS_" necessary to trick the language generator into pronouncing it properly
            transmission += Attention + ": " + short_time__I_LS_interference_possible_by_taxiing_aircraft + BRK;
        }
    }
    else
    if (position == 1) // warnings at the end of the report
    {
        // "runway wet-wet-wet" warning in heavy rain
        if (_report.rain_norm > 0.6)
        {
            // "wet" is repeated 3 times in ATIS warnings, since the word is difficult
            // to understand over radio - but the message is important.
            transmission += runway_wet + " " + wet + " " + wet + BRK;
        }

        if (_report.notam == 4)
        {
            // intentional: "reed" instead of "read" since festival gets it wrong otherwise
            transmission += reed_back_all_runway_hold_instructions + BRK;
        }
        else
        if ((_report.notam == 5)&& _report.cavok && dayTime &&
            (_report.rain_norm == 0) && (_report.snow_norm == 0)) // ;-)
        {
            transmission += Attention + ": " + glider_operation_in_sector + BRK;
        }
    }
}

// Put the transmission into the property tree.
// You can see it by pointing a web browser
// at the property tree.  The second comm radio is:
// http://localhost:5400/instrumentation/comm[1]
//
// (Also, if in debug mode, dump it to the console.)
void FGATIS::treeOut(int msg_OK)
{
    _atis->setStringValue("<pre>\n" + transmission_readable + "</pre>\n");
    SG_LOG(SG_ATC, SG_DEBUG, "**** ATIS active on: " << _name <<
           "transmission: " << transmission_readable);
}


class RangeFilter : public CommStation::Filter
{
public:
    RangeFilter( const SGGeod & pos ) :
      CommStation::Filter(),
      _cart(SGVec3d::fromGeod(pos)),
      _pos(pos)
    {
    }

    virtual bool pass(FGPositioned* aPos) const
    {
        flightgear::CommStation * stn = dynamic_cast<flightgear::CommStation*>(aPos);
        if( NULL == stn )
            return false;

        // do the range check in cartesian space, since the distances are potentially
        // large enough that the geodetic functions become unstable
        // (eg, station on opposite side of the planet)
        double rangeM = SGMiscd::max( stn->rangeNm(), 10.0 ) * SG_NM_TO_METER;
        double d2 = distSqr( aPos->cart(), _cart);

        return d2 <= (rangeM * rangeM);
    }

    virtual CommStation::Type minType() const
    {
      return CommStation::FREQ_ATIS;
    }

    virtual CommStation::Type maxType() const
    {
      return CommStation::FREQ_AWOS;
    }

private:
    SGVec3d _cart;
    SGGeod _pos;
};

// Search for ATC stations by frequency
bool FGATIS::search(double dt)
{
    double frequency = _freq->getDoubleValue();

    // Note:  122.375 must be rounded DOWN to 122370
    // in order to be consistent with apt.dat et cetera.
    int freqKhz = 10 * static_cast<int>(frequency * 100 + 0.25);

    // only search tuned frequencies when necessary
    _time_before_search_sec -= dt;

    // throttle frequency searches
    if ((freqKhz == _last_frequency)&&(_time_before_search_sec > 0))
        return false;

    _last_frequency = freqKhz;
    _time_before_search_sec = 4.0;

    // Position of the Users Aircraft
    SGGeod aircraftPos = SGGeod::fromDegFt(_lon_node->getDoubleValue(),
                                           _lat_node->getDoubleValue(),
                                           _elev_node->getDoubleValue());

    RangeFilter rangeFilter(aircraftPos );
    CommStation* sta = CommStation::findByFreq(freqKhz, aircraftPos, &rangeFilter );
    SetStation(sta);
    if (sta && sta->airport())
    {
        SG_LOG(SG_ATC, SG_DEBUG, "FGATIS " << _name << ": " << sta->airport()->name());
    }
    else
    {
        SG_LOG(SG_ATC, SG_DEBUG, "FGATIS " << _name << ": no station.");
    }

    return true;
}
