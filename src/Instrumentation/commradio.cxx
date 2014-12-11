// commradio.cxx -- class to manage a nav radio instance
//
// Written by Torsten Dreyer, February 2014
//
// Copyright (C) 2000 - 2011  Curtis L. Olson - http://www.flightgear.org/~curt
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
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "commradio.hxx"

#include <assert.h>
#include <boost/foreach.hpp>

#include <simgear/sg_inlines.h>
#include <simgear/props/propertyObject.hxx>
#include <simgear/misc/strutils.hxx>

#include <ATC/CommStation.hxx>
#include <ATC/MetarPropertiesATISInformationProvider.hxx>
#include <ATC/CurrentWeatherATISInformationProvider.hxx>
#include <Airports/airport.hxx>
#include <Main/fg_props.hxx>
#include <Navaids/navlist.hxx>

#if defined(ENABLE_FLITE)
#include <Sound/soundmanager.hxx>
#include <simgear/sound/sample_group.hxx>
#include <Sound/VoiceSynthesizer.hxx>
#endif

#include "frequencyformatter.hxx"

namespace Instrumentation {

using simgear::PropertyObject;
using std::string;

#if defined(ENABLE_FLITE)
class AtisSpeaker: public SGPropertyChangeListener, SoundSampleReadyListener {
public:
  AtisSpeaker();
  virtual ~AtisSpeaker();
  virtual void valueChanged(SGPropertyNode * node);
  virtual void SoundSampleReady(SGSharedPtr<SGSoundSample>);

  bool hasSpokenAtis()
  {
    return _spokenAtis.empty() == false;
  }

  SGSharedPtr<SGSoundSample> getSpokenAtis()
  {
    return _spokenAtis.pop();
  }

  void setStationId(const string & stationId)
  {
    _stationId = stationId;
  }


private:
  SynthesizeRequest _synthesizeRequest;
  SGLockedQueue<SGSharedPtr<SGSoundSample> > _spokenAtis;
  string _stationId;
};

AtisSpeaker::AtisSpeaker()
{
  _synthesizeRequest.listener = this;
}

AtisSpeaker::~AtisSpeaker()
{

}
void AtisSpeaker::valueChanged(SGPropertyNode * node)
{
  if (!fgGetBool("/sim/sound/working", false))
    return;

  string newText = node->getStringValue();
  if (_synthesizeRequest.text == newText) return;

  _synthesizeRequest.text = newText;

  string voice = "cmu_us_arctic_slt";

  if (!_stationId.empty()) {
    // lets play a bit with the voice so not every airports atis sounds alike
    // but every atis of an airport has the same voice

    // create a simple hash from the last two letters of the airport's id
    unsigned char hash = 0;
    string::iterator i = _stationId.end() - 1;
    hash += *i;

    if( i != _stationId.begin() ) {
      --i;
      hash += *i;
    }

    _synthesizeRequest.speed = (hash % 16) / 16.0;
    _synthesizeRequest.pitch = (hash % 16) / 16.0;

    // pick a voice
    voice = FLITEVoiceSynthesizer::getVoicePath(
        static_cast<FLITEVoiceSynthesizer::voice_t>(hash % FLITEVoiceSynthesizer::VOICE_UNKNOWN) );
  }


  FGSoundManager * smgr = dynamic_cast<FGSoundManager*>(globals->get_soundmgr());
  assert(smgr != NULL);

  SG_LOG(SG_INSTR,SG_INFO,"AtisSpeaker voice is " << voice );
  FLITEVoiceSynthesizer * synthesizer = dynamic_cast<FLITEVoiceSynthesizer*>(smgr->getSynthesizer(voice));

  synthesizer->synthesize(_synthesizeRequest);
}

void AtisSpeaker::SoundSampleReady(SGSharedPtr<SGSoundSample> sample)
{
  // we are now in the synthesizers worker thread!
  _spokenAtis.push(sample);
}
#endif

SignalQualityComputer::~SignalQualityComputer()
{
}

class SimpleDistanceSquareSignalQualityComputer: public SignalQualityComputer {
public:
  SimpleDistanceSquareSignalQualityComputer()
      : _altitudeAgl_ft(fgGetNode("/position/altitude-agl-ft", true))
  {
  }

  ~SimpleDistanceSquareSignalQualityComputer()
  {
  }

  double computeSignalQuality(double distance_nm) const
  {
    // Very simple line of sight propagation model. It's cheap but it does the trick for now.
    // assume transmitter and receiver antennas are at some elevation above ground
    // so we have at least a range of 5NM. Add the approx. distance to the horizon.
    double range_nm = 5.0 + 1.23 * ::sqrt(SGMiscd::max(.0, _altitudeAgl_ft));
    return distance_nm < range_nm ? 1.0 : (range_nm * range_nm / distance_nm / distance_nm);
  }

private:
  PropertyObject<double> _altitudeAgl_ft;
};

class OnExitHandler {
public:
  virtual void onExit() = 0;
  virtual ~OnExitHandler()
  {
  }
};

class OnExit {
public:
  OnExit(OnExitHandler * onExitHandler)
      : _onExitHandler(onExitHandler)
  {
  }
  ~OnExit()
  {
    _onExitHandler->onExit();
  }
private:
  OnExitHandler * _onExitHandler;
};

class OutputProperties: public OnExitHandler {
public:
  OutputProperties(SGPropertyNode_ptr rootNode)
      : _rootNode(rootNode), _signalQuality_norm(0.0), _slantDistance_m(0.0), _trueBearingTo_deg(0.0), _trueBearingFrom_deg(0.0), _trackDistance_m(
          0.0), _heightAboveStation_ft(0.0),

      _PO_stationType(rootNode->getNode("station-type", true)), _PO_stationName(rootNode->getNode("station-name", true)), _PO_airportId(
          rootNode->getNode("airport-id", true)), _PO_signalQuality_norm(rootNode->getNode("signal-quality-norm", true)), _PO_slantDistance_m(
          rootNode->getNode("slant-distance-m", true)), _PO_trueBearingTo_deg(rootNode->getNode("true-bearing-to-deg", true)), _PO_trueBearingFrom_deg(
          rootNode->getNode("true-bearing-from-deg", true)), _PO_trackDistance_m(rootNode->getNode("track-distance-m", true)), _PO_heightAboveStation_ft(
          rootNode->getNode("height-above-station-ft", true))
  {
  }
  virtual ~OutputProperties()
  {
  }

protected:
  SGPropertyNode_ptr _rootNode;

  std::string _stationType;
  std::string _stationName;
  std::string _airportId;
  double _signalQuality_norm;
  double _slantDistance_m;
  double _trueBearingTo_deg;
  double _trueBearingFrom_deg;
  double _trackDistance_m;
  double _heightAboveStation_ft;

private:
  PropertyObject<string> _PO_stationType;
  PropertyObject<string> _PO_stationName;
  PropertyObject<string> _PO_airportId;
  PropertyObject<double> _PO_signalQuality_norm;
  PropertyObject<double> _PO_slantDistance_m;
  PropertyObject<double> _PO_trueBearingTo_deg;
  PropertyObject<double> _PO_trueBearingFrom_deg;
  PropertyObject<double> _PO_trackDistance_m;
  PropertyObject<double> _PO_heightAboveStation_ft;

  virtual void onExit()
  {
    _PO_stationType = _stationType;
    _PO_stationName = _stationName;
    _PO_airportId = _airportId;
    _PO_signalQuality_norm = _signalQuality_norm;
    _PO_slantDistance_m = _slantDistance_m;
    _PO_trueBearingTo_deg = _trueBearingTo_deg;
    _PO_trueBearingFrom_deg = _trueBearingFrom_deg;
    _PO_trackDistance_m = _trackDistance_m;
    _PO_heightAboveStation_ft = _heightAboveStation_ft;
  }
};

/* ------------- The CommRadio implementation ---------------------- */

class MetarBridge: public SGReferenced, public SGPropertyChangeListener {
public:
  MetarBridge();
  ~MetarBridge();

  void bind();
  void unbind();
  void requestMetarForId(std::string & id);
  void clearMetar();
  void setMetarPropertiesRoot(SGPropertyNode_ptr n)
  {
    _metarPropertiesNode = n;
  }
  void setAtisNode(SGPropertyNode * n)
  {
    _atisNode = n;
  }

protected:
  virtual void valueChanged(SGPropertyNode *);

private:
  std::string _requestedId;
  SGPropertyNode_ptr _realWxEnabledNode;
  SGPropertyNode_ptr _metarPropertiesNode;
  SGPropertyNode * _atisNode;
  ATISEncoder _atisEncoder;
};
typedef SGSharedPtr<MetarBridge> MetarBridgeRef;

MetarBridge::MetarBridge()
    : _atisNode(0)
{
}

MetarBridge::~MetarBridge()
{
}

void MetarBridge::bind()
{
  _realWxEnabledNode = fgGetNode("/environment/realwx/enabled", true);
  _metarPropertiesNode->getNode("valid", true)->addChangeListener(this);
}

void MetarBridge::unbind()
{
  _metarPropertiesNode->getNode("valid", true)->removeChangeListener(this);
}

void MetarBridge::requestMetarForId(std::string & id)
{
  std::string uppercaseId = simgear::strutils::uppercase(id);
  if (_requestedId == uppercaseId) return;
  _requestedId = uppercaseId;

  if (_realWxEnabledNode->getBoolValue()) {
    //  trigger a METAR request for the associated metarproperties
    _metarPropertiesNode->getNode("station-id", true)->setStringValue(uppercaseId);
    _metarPropertiesNode->getNode("valid", true)->setBoolValue(false);
    _metarPropertiesNode->getNode("time-to-live", true)->setDoubleValue(0.0);
  } else {
    // use the present weather to generate the ATIS. 
    if ( NULL != _atisNode && false == _requestedId.empty()) {
      CurrentWeatherATISInformationProvider provider(_requestedId);
      _atisNode->setStringValue(_atisEncoder.encodeATIS(&provider));
    }
  }
}

void MetarBridge::clearMetar()
{
  string empty;
  requestMetarForId(empty);
}

void MetarBridge::valueChanged(SGPropertyNode * node)
{
  // check for raising edge of valid flag
  if ( NULL == node || false == node->getBoolValue() || false == _realWxEnabledNode->getBoolValue()) return;

  std::string responseId = simgear::strutils::uppercase(_metarPropertiesNode->getNode("station-id", true)->getStringValue());

  // unrequested metar!?
  if (responseId != _requestedId) return;

  if ( NULL != _atisNode) {
    MetarPropertiesATISInformationProvider provider(_metarPropertiesNode);
    _atisNode->setStringValue(_atisEncoder.encodeATIS(&provider));
  }
}

/* ------------- The CommRadio implementation ---------------------- */

class CommRadioImpl: public CommRadio, OutputProperties {

public:
  CommRadioImpl(SGPropertyNode_ptr node);
  virtual ~CommRadioImpl();

  virtual void update(double dt);
  virtual void init();
  void bind();
  void unbind();

private:
  string getSampleGroupRefname() const
  {
    return _rootNode->getPath();
  }

  int _num;
  MetarBridgeRef _metarBridge;
  #if defined(ENABLE_FLITE)
  AtisSpeaker _atisSpeaker;
  #endif
  FrequencyFormatter _useFrequencyFormatter;
  FrequencyFormatter _stbyFrequencyFormatter;
  const SignalQualityComputerRef _signalQualityComputer;

  double _stationTTL;
  double _frequency;
  flightgear::CommStationRef _commStationForFrequency;
  #if defined(ENABLE_FLITE)
  SGSharedPtr<SGSampleGroup> _sampleGroup;
  #endif

  PropertyObject<bool> _serviceable;
  PropertyObject<bool> _power_btn;
  PropertyObject<bool> _power_good;
  PropertyObject<double> _volume_norm;
  PropertyObject<string> _atis;
  PropertyObject<bool> _addNoise;
  PropertyObject<double> _cutoffSignalQuality;
};

CommRadioImpl::CommRadioImpl(SGPropertyNode_ptr node)
    : OutputProperties(
        fgGetNode("/instrumentation", true)->getNode(node->getStringValue("name", "comm"), node->getIntValue("number", 0), true)),
        _num(node->getIntValue("number", 0)),
        _metarBridge(new MetarBridge()),
        _useFrequencyFormatter(_rootNode->getNode("frequencies/selected-mhz", true),
            _rootNode->getNode("frequencies/selected-mhz-fmt", true), 0.025, 118.0, 137.0),

        _stbyFrequencyFormatter(_rootNode->getNode("frequencies/standby-mhz", true),
            _rootNode->getNode("frequencies/standby-mhz-fmt", true), 0.025, 118.0, 137.0),

        _signalQualityComputer(new SimpleDistanceSquareSignalQualityComputer()),

        _stationTTL(0.0),
        _frequency(-1.0),
        _commStationForFrequency(NULL),

        _serviceable(_rootNode->getNode("serviceable", true)),
        _power_btn(_rootNode->getNode("power-btn", true)),
        _power_good(_rootNode->getNode("power-good", true)),
        _volume_norm(_rootNode->getNode("volume", true)),
        _atis(_rootNode->getNode("atis", true)),
        _addNoise(_rootNode->getNode("add-noise", true)),
        _cutoffSignalQuality(_rootNode->getNode("cutoff-signal-quality", true))
{
}

CommRadioImpl::~CommRadioImpl()
{
}

void CommRadioImpl::bind()
{
  _metarBridge->setAtisNode(_atis.node());
#if defined(ENABLE_FLITE)
  _atis.node()->addChangeListener(&_atisSpeaker);
#endif
  // link the metar node. /environment/metar[3] is comm1 and /environment[4] is comm2.
  // see FGDATA/Environment/environment.xml
  _metarBridge->setMetarPropertiesRoot(fgGetNode("/environment", true)->getNode("metar", _num + 3, true));
  _metarBridge->bind();
}

void CommRadioImpl::unbind()
{
#if defined(ENABLE_FLITE)
  _atis.node()->removeChangeListener(&_atisSpeaker);
  if (_sampleGroup.valid()) {
    globals->get_soundmgr()->remove(getSampleGroupRefname());
  }
#endif
  _metarBridge->unbind();
}

void CommRadioImpl::init()
{
  // initialize power_btn to true if unset
  string s = _power_btn.node()->getStringValue();
  if (s.empty()) _power_btn = true;

  // initialize squelch to a sane value if unset
  s = _cutoffSignalQuality.node()->getStringValue();
  if (s.empty()) _cutoffSignalQuality = 0.4;

  // initialize add-noize to true if unset
  s = _addNoise.node()->getStringValue();
  if (s.empty()) _addNoise = true;
}

void CommRadioImpl::update(double dt)
{
  if (dt < SGLimitsd::min()) return;
  _stationTTL -= dt;

  // Ensure all output properties get written on exit of this method
  OnExit onExit(this);

  SGGeod position;
  try {
    position = globals->get_aircraft_position();
  }
  catch (std::exception &) {
    return;
  }

#if defined(ENABLE_FLITE)
  {
    static const char * atisSampleRefName = "atis";
    static const char * noiseSampleRefName = "noise";

    if (_atisSpeaker.hasSpokenAtis()) {
      // the speaker has created a new atis sample
      if (!_sampleGroup.valid()) {
        // create a sample group for our instrument on the fly
        _sampleGroup = globals->get_soundmgr()->find(getSampleGroupRefname(), true);
        _sampleGroup->tie_to_listener();
        if (_addNoise) {
          SGSharedPtr<SGSoundSample> noise = new SGSoundSample("Sounds/radionoise.wav", globals->get_fg_root());
          _sampleGroup->add(noise, noiseSampleRefName);
          _sampleGroup->play_looped(noiseSampleRefName);
        }

      }
      // remove previous atis sample
      _sampleGroup->remove(atisSampleRefName);
      // add and play the new atis sample
      SGSharedPtr<SGSoundSample> sample = _atisSpeaker.getSpokenAtis();
      _sampleGroup->add(sample, atisSampleRefName);
      _sampleGroup->play_looped(atisSampleRefName);
    }

    if (_sampleGroup.valid()) {
      if (_addNoise) {
        // if noise is configured and there is a noise sample
        // scale noise and signal volume by signalQuality.
        SGSoundSample * s = _sampleGroup->find(noiseSampleRefName);
        if ( NULL != s) {
          s->set_volume(1.0 - _signalQuality_norm);
          s = _sampleGroup->find(atisSampleRefName);
          s->set_volume(_signalQuality_norm);
        }
      }
      // master volume for radio, mute on bad signal quality
      _sampleGroup->set_volume(_signalQuality_norm >= _cutoffSignalQuality ? _volume_norm : 0.0);
    }
  }
#endif

  if (false == (_power_btn)) {
    _metarBridge->clearMetar();
    _atis = "";
    _stationTTL = 0.0;
    return;
  }

  if (_frequency != _useFrequencyFormatter.getFrequency()) {
    _frequency = _useFrequencyFormatter.getFrequency();
    _stationTTL = 0.0;
  }

  if (_stationTTL <= 0.0) {
    _stationTTL = 30.0;

    // Note:  122.375 must be rounded DOWN to 122370
    // in order to be consistent with apt.dat et cetera.
    int freqKhz = 10 * static_cast<int>(_frequency * 100 + 0.25);
    _commStationForFrequency = flightgear::CommStation::findByFreq(freqKhz, position, NULL);

  }

  if (false == _commStationForFrequency.valid()) return;

  _slantDistance_m = dist(_commStationForFrequency->cart(), SGVec3d::fromGeod(position));

  SGGeodesy::inverse(position, _commStationForFrequency->geod(), _trueBearingTo_deg, _trueBearingFrom_deg, _trackDistance_m);

  _heightAboveStation_ft = SGMiscd::max(0.0, position.getElevationFt() - _commStationForFrequency->airport()->elevation());

  _signalQuality_norm = _signalQualityComputer->computeSignalQuality(_slantDistance_m * SG_METER_TO_NM);
  _stationType = _commStationForFrequency->nameForType(_commStationForFrequency->type());
  _stationName = _commStationForFrequency->ident();
  _airportId = _commStationForFrequency->airport()->getId();

#if defined(ENABLE_FLITE)
  _atisSpeaker.setStationId(_airportId);
#endif
  switch (_commStationForFrequency->type()) {
    case FGPositioned::FREQ_ATIS:
      case FGPositioned::FREQ_AWOS: {
      if (_signalQuality_norm > 0.01) {
        _metarBridge->requestMetarForId(_airportId);
      } else {
        _metarBridge->clearMetar();
        _atis = "";
      }
    }
      break;

    default:
      _metarBridge->clearMetar();
      _atis = "";
      break;
  }
}

SGSubsystem * CommRadio::createInstance(SGPropertyNode_ptr rootNode)
{
  return new CommRadioImpl(rootNode);
}

} // namespace Instrumentation

