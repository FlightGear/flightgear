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

#include <simgear/sg_inlines.h>
#include <simgear/props/propertyObject.hxx>
#include <simgear/misc/strutils.hxx>

#include <ATC/CommStation.hxx>
#include <ATC/MetarPropertiesATISInformationProvider.hxx>
#include <ATC/CurrentWeatherATISInformationProvider.hxx>
#include <Airports/airport.hxx>
#include <Main/fg_props.hxx>
#include <Navaids/navlist.hxx>

#include <Sound/soundmanager.hxx>
#include <simgear/sound/sample_group.hxx>
#include <Sound/VoiceSynthesizer.hxx>

#include "frequencyformatter.hxx"

namespace Instrumentation {

using simgear::PropertyObject;
using std::string;

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
  if (!fgHasNode("/sim/atis/speed"))    fgSetDouble("/sim/atis/speed", 1);
  if (!fgHasNode("/sim/atis/pitch"))    fgSetDouble("/sim/atis/pitch", 1);
  if (!fgHasNode("/sim/atis/enabled"))  fgSetBool("/sim/atis/enabled", true);
}

AtisSpeaker::~AtisSpeaker()
{

}
void AtisSpeaker::valueChanged(SGPropertyNode * node)
{
  using namespace simgear::strutils;

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

    _synthesizeRequest.speed = (hash % 16) / 16.0 * fgGetDouble("/sim/atis/speed", 1);
    _synthesizeRequest.pitch = (hash % 16) / 16.0 * fgGetDouble("/sim/atis/pitch", 1);
    
    if( starts_with( _stationId, "K" ) || starts_with( _stationId, "C" ) ||
            starts_with( _stationId, "P" ) ) {
        voice = FLITEVoiceSynthesizer::getVoicePath("cmu_us_arctic_slt");
    } else if ( starts_with( _stationId, "EG" ) ) {
        voice = FLITEVoiceSynthesizer::getVoicePath("cstr_uk_female");
    } else {
        // Pick a random voice from the available voices
        voice = FLITEVoiceSynthesizer::getVoicePath(
            static_cast<FLITEVoiceSynthesizer::voice_t>(hash % FLITEVoiceSynthesizer::VOICE_UNKNOWN) );
    }
  }

  FGSoundManager * smgr = globals->get_subsystem<FGSoundManager>();
  assert(smgr != NULL);

  SG_LOG(SG_INSTR, SG_DEBUG,"node->getPath()=" << node->getPath() << " AtisSpeaker voice is " << voice );
  FLITEVoiceSynthesizer * synthesizer = dynamic_cast<FLITEVoiceSynthesizer*>(smgr->getSynthesizer(voice));

  synthesizer->synthesize(_synthesizeRequest);
}

void AtisSpeaker::SoundSampleReady(SGSharedPtr<SGSoundSample> sample)
{
  // we are now in the synthesizers worker thread!
  _spokenAtis.push(sample);
}

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

  void bind(SGPropertyNode* rn)
  {
      _rootNode = rn;

      _PO_stationType = PropertyObject<string>(_rootNode->getNode("station-type", true));
      _PO_stationName = PropertyObject<string>(_rootNode->getNode("station-name", true));
      _PO_airportId = PropertyObject<string>(_rootNode->getNode("airport-id", true));
      _PO_signalQuality_norm = PropertyObject<double>(_rootNode->getNode("signal-quality-norm", true));
      _PO_slantDistance_m = PropertyObject<double>(_rootNode->getNode("slant-distance-m", true));
      _PO_trueBearingTo_deg = PropertyObject<double>(_rootNode->getNode("true-bearing-to-deg", true));
      _PO_trueBearingFrom_deg = PropertyObject<double>(_rootNode->getNode("true-bearing-from-deg", true));
      _PO_trackDistance_m = PropertyObject<double>(_rootNode->getNode("track-distance-m", true));
      _PO_heightAboveStation_ft = PropertyObject<double>(_rootNode->getNode("height-above-station-ft", true));
  }

  virtual ~OutputProperties()
  {
  }

protected:
  SGPropertyNode_ptr _rootNode;

  std::string _stationType;
  std::string _stationName;
  std::string _airportId;
  double _signalQuality_norm = 0.0;
  double _slantDistance_m = 0.0;
  double _trueBearingTo_deg = 0.0;
  double _trueBearingFrom_deg = 0.0;
  double _trackDistance_m = 0.0;
  double _heightAboveStation_ft = 0.0;

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
    SGPropertyNode * _atisNode = nullptr;
  ATISEncoder _atisEncoder;
};

typedef SGSharedPtr<MetarBridge> MetarBridgeRef;

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

/* ------------- 8.3kHz Channel implementation ---------------------- */

class EightPointThreeFrequencyFormatter :
		public FrequencyFormatterBase,
		public SGPropertyChangeListener {
public:
	EightPointThreeFrequencyFormatter( SGPropertyNode_ptr root,
			const char * channel,
			const char * fmt,
			const char * width,
			const char * frq,
			const char * cnum ) :
		_channel( root, channel ),
		_frequency( root, frq ),
		_channelSpacing( root, width ),
		_formattedChannel( root, fmt ),
		_channelNum( root, cnum )
	{
		// ensure properties exist.
		_channel.node(true);
		_frequency.node(true);
		_channelSpacing.node(true);
		_channelNum.node(true);
		_formattedChannel.node(true);

		_channel.node()->addChangeListener( this, true );
		_channelNum.node()->addChangeListener( this, true );
	}

	virtual ~EightPointThreeFrequencyFormatter()
	{
		  _channel.node()->removeChangeListener( this );
		  _channelNum.node()->removeChangeListener( this );
	}

private:
	EightPointThreeFrequencyFormatter( const EightPointThreeFrequencyFormatter & );
	EightPointThreeFrequencyFormatter & operator = ( const EightPointThreeFrequencyFormatter & );

	void valueChanged (SGPropertyNode * prop) {
		if( prop == _channel.node() )
			setFrequency(prop->getDoubleValue());
		else if( prop == _channelNum.node() )
			setChannel(prop->getIntValue());
	}

	void setChannel( int channel ) {
		channel %= 3040;
		if( channel < 0 ) channel += 3040;
		double f = 118.000 + 0.025*(channel/4) + 0.005*(channel%4);
		if( f != _channel )	_channel = f;
	}

	void setFrequency( double channel ) {
		// format as fixed decimal "nnn.nnn"
		std::ostringstream buf;
		buf << std::fixed
		    << std::setw(6)
		    << std::setfill('0')
		    << std::setprecision(3)
		    << _channel;
		_formattedChannel = buf.str();

		// sanitize range and round to nearest kHz.
		unsigned c = static_cast<int>(SGMiscd::round(SGMiscd::clip( channel, 118.0, 136.99 ) * 1000));

		if ( (c % 25)  == 0 ) {
			// legacy 25kHz channels continue to be just that.
			_channelSpacing = 25.0;
			_frequency = c / 1000.0;

			int channelNum = (c-118000)/25*4;
			if( channelNum != _channelNum ) _channelNum = channelNum;

			if( _frequency != channel ) {
				_channel = _frequency; //triggers recursion
			}
		} else {
			_channelSpacing = 8.33;

			// 25kHz base frequency: xxx.000, xxx.025, xxx.050, xxx.075
			unsigned base25 = (c/25) * 25;

			// add n*8.33 to the 25kHz frequency
			unsigned subChannel = SGMisc<unsigned>::clip((c - base25)/5-1, 0, 2 );

			_frequency = (base25 + 8.33 * subChannel)/1000.0;

			int channelNum = (base25-118000)/25*4 + subChannel+1;
			if( channelNum != _channelNum ) _channelNum = channelNum;

			// set to correct channel on bogous input
			double sanitizedChannel = (base25 + 5*(subChannel+1))/1000.0;
			if( sanitizedChannel != channel ) {
				_channel = sanitizedChannel; // triggers recursion
			}
		}
	}

	double getFrequency() const {
		return _channel;
	}


    PropertyObject<double> _channel;
    PropertyObject<double> _frequency;
    PropertyObject<double> _channelSpacing;
    PropertyObject<string> _formattedChannel;
    PropertyObject<int>   _channelNum;

};


/* ------------- The CommRadio implementation ---------------------- */

class CommRadioImpl: public CommRadio,
                     OutputProperties
{
public:
    CommRadioImpl(SGPropertyNode_ptr node);
    virtual ~CommRadioImpl();

    // Subsystem API.
    void bind() override;
    void init() override;
    void unbind() override;
    void update(double dt) override;

private:
    bool _useEightPointThree = false;
    MetarBridgeRef _metarBridge;
    AtisSpeaker _atisSpeaker;
    SGSharedPtr<FrequencyFormatterBase> _useFrequencyFormatter;
    SGSharedPtr<FrequencyFormatterBase> _stbyFrequencyFormatter;
    const SignalQualityComputerRef _signalQualityComputer;

    double _stationTTL = 0.0;
    double _frequency = -1.0;
    flightgear::CommStationRef _commStationForFrequency;

    PropertyObject<double> _volume_norm;
    PropertyObject<string> _atis;
    PropertyObject<bool> _addNoise;
    PropertyObject<double> _cutoffSignalQuality;

    SGPropertyNode_ptr          _atis_enabled_node;
    bool                        _atis_enabled_prev;
    SGSharedPtr<SGSoundSample>  _atis_sample;

    std::string _soundPrefix;
    void stopAudio();
    void updateAudio();

    SGSampleGroup* _sampleGroup = nullptr;
};

CommRadioImpl::CommRadioImpl(SGPropertyNode_ptr node) :
  _metarBridge(new MetarBridge),
  _signalQualityComputer(new SimpleDistanceSquareSignalQualityComputer)
{
  // set a special value to indicate we don't require a power supply node
  // by default
  setDefaultPowerSupplyPath("NO_DEFAULT");
  readConfig(node, "comm");
  _soundPrefix = name() + "_" + std::to_string(number()) + "_";
  _useEightPointThree = node->getBoolValue("eight-point-three", false );
}

CommRadioImpl::~CommRadioImpl()
{
}

void CommRadioImpl::bind()
{
  SGPropertyNode_ptr n = fgGetNode(nodePath(), true);
  OutputProperties::bind(n);

  _volume_norm = PropertyObject<double>(_rootNode->getNode("volume", true));
  _atis = PropertyObject<string>(_rootNode->getNode("atis", true));
  if (!fgHasNode("/sim/atis/enabled")) fgSetBool("/sim/atis/enabled", true);
  _atis_enabled_node = fgGetNode("/sim/atis/enabled");
  _addNoise = PropertyObject<bool>(_rootNode->getNode("add-noise", true));
  _cutoffSignalQuality = PropertyObject<double>(_rootNode->getNode("cutoff-signal-quality", true));

  _metarBridge->setAtisNode(_atis.node());
  _atis.node()->addChangeListener(&_atisSpeaker);

  // link the metar node. /environment/metar[3] is comm1 and /environment[4] is comm2.
  // see FGDATA/Environment/environment.xml
  _metarBridge->setMetarPropertiesRoot(fgGetNode("/environment", true)->getNode("metar", number() + 3, true));
  _metarBridge->bind();

  if (_useEightPointThree) {
    _useFrequencyFormatter = new EightPointThreeFrequencyFormatter(
                                                                   _rootNode->getNode("frequencies", true),
                                                                   "selected-mhz",
                                                                   "selected-mhz-fmt",
                                                                   "selected-channel-width-khz",
                                                                   "selected-real-frequency-mhz",
                                                                   "selected-channel"
                                                                   );
    _stbyFrequencyFormatter = new EightPointThreeFrequencyFormatter(
                                                                    _rootNode->getNode("frequencies", true),
                                                                    "standby-mhz",
                                                                    "standby-mhz-fmt",
                                                                    "standby-channel-width-khz",
                                                                    "standby-real-frequency-mhz",
                                                                    "standby-channel"
                                                                    );
  } else {
    _useFrequencyFormatter = new FrequencyFormatter(
                                                    _rootNode->getNode("frequencies/selected-mhz", true),
                                                    _rootNode->getNode("frequencies/selected-mhz-fmt", true),
                                                    0.025, 118.0, 137.0);

    _stbyFrequencyFormatter = new FrequencyFormatter(
                                                     _rootNode->getNode("frequencies/standby-mhz", true),
                                                     _rootNode->getNode("frequencies/standby-mhz-fmt", true),
                                                     0.025, 118.0, 137.0);


  }
}

void CommRadioImpl::unbind()
{
  _atis.node()->removeChangeListener(&_atisSpeaker);
  
  stopAudio();
  
  _metarBridge->unbind();
  AbstractInstrument::unbind();
}

void CommRadioImpl::init()
{
  initServicePowerProperties(_rootNode);

  string s;
  // initialize squelch to a sane value if unset
  s = _cutoffSignalQuality.node()->getStringValue();
  if (s.empty()) _cutoffSignalQuality = 0.4;

  // initialize add-noize to true if unset
  s = _addNoise.node()->getStringValue();
  if (s.empty()) _addNoise = true;
  
  auto soundManager = globals->get_subsystem<SGSoundMgr>();
  if (soundManager) {
    _sampleGroup = soundManager->find("atc", false);
  }
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

  if (!isServiceableAndPowered()) {
    _metarBridge->clearMetar();
    _atis = "";
    _stationTTL = 0.0;
    stopAudio();
    return;
  }

  if (_frequency != _useFrequencyFormatter->getFrequency()) {
    _frequency = _useFrequencyFormatter->getFrequency();
    _stationTTL = 0.0;
  }

  if (_stationTTL <= 0.0) {
    _stationTTL = 30.0;

    // Note:  122.375 must be rounded DOWN to 122370
    // in order to be consistent with apt.dat et cetera.
    int freqKhz = 10 * static_cast<int>(_frequency * 100 + 0.25);
    _commStationForFrequency = flightgear::CommStation::findByFreq(freqKhz, position, NULL);

  }

  if (false == _commStationForFrequency.valid()) {
    stopAudio();
    return;
  }
  
  _slantDistance_m = dist(_commStationForFrequency->cart(), SGVec3d::fromGeod(position));

  SGGeodesy::inverse(position, _commStationForFrequency->geod(), _trueBearingTo_deg, _trueBearingFrom_deg, _trackDistance_m);

  _heightAboveStation_ft = SGMiscd::max(0.0, position.getElevationFt() - _commStationForFrequency->airport()->elevation());

  _signalQuality_norm = _signalQualityComputer->computeSignalQuality(_slantDistance_m * SG_METER_TO_NM);
  _stationType = _commStationForFrequency->nameForType(_commStationForFrequency->type());
  _stationName = _commStationForFrequency->ident();
  _airportId = _commStationForFrequency->airport()->getId();

  _atisSpeaker.setStationId(_airportId);

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
  
  updateAudio();
}

void CommRadioImpl::updateAudio()
{
  if (!_sampleGroup)
    return;
  
  const string noiseRef = _soundPrefix + "_noise";
  const string atisRef = _soundPrefix + "_atis";

  SGSoundSample* noiseSample = _sampleGroup->find(noiseRef);
  
  // create noise sample if necessary, and play forever
  if (_addNoise && !noiseSample) {
    SGSharedPtr<SGSoundSample> noise = new SGSoundSample("Sounds/radionoise.wav", globals->get_fg_root());
    _sampleGroup->add(noise, noiseRef);
    _sampleGroup->play_looped(noiseRef);
    noiseSample = noise;
  }
  
  bool atis_enabled = _atis_enabled_node->getBoolValue();
  int atis_delta = 0;
  if (atis_enabled && !_atis_enabled_prev) atis_delta = 1;
  if (!atis_enabled && _atis_enabled_prev) atis_delta = -1;
  
  if (_atisSpeaker.hasSpokenAtis()) {
    // the speaker has created a new atis sample
    // remove previous atis sample
    _sampleGroup->remove(atisRef);
    if (!atis_delta && atis_enabled) atis_delta = 1;
  }
  if (atis_delta == 1) {
    // Start play of atis text. We store the most recent sample in _atis_sample
    // so that we can resume if /sim/atis/enabled is changed from false to
    // true.
    SGSharedPtr<SGSoundSample> sample = _atisSpeaker.getSpokenAtis();
    if (sample) _atis_sample = sample;
    else sample = _atis_sample;
    if (sample) {
      SG_LOG(SG_INSTR, SG_DEBUG, "starting looped play of atis sample.");
      _sampleGroup->add(sample, atisRef);
      _sampleGroup->play_looped(atisRef);
    }
    else {
      SG_LOG(SG_INSTR, SG_DEBUG, "no atis sample available");
    }
  }
  if (atis_delta == -1) {
    // Stop play of atis text.
    _sampleGroup->remove(atisRef);
  }
  _atis_enabled_prev = atis_enabled;
  
  // adjust volumes
  const bool doSquelch = (_signalQuality_norm < _cutoffSignalQuality);
  double atisVolume = doSquelch ? 0.0 : _volume_norm;
  if (_addNoise) {
    const double noiseVol = (1.0 - _signalQuality_norm) * _volume_norm;
    atisVolume = _signalQuality_norm * _volume_norm;
    noiseSample->set_volume(doSquelch ? 0.0: noiseVol);
  }
  
  SGSoundSample* s = _sampleGroup->find(atisRef);
  if (s) {
    s->set_volume(atisVolume);
  }
}
  
void CommRadioImpl::stopAudio()
{
  if (_sampleGroup) {
    _sampleGroup->remove(_soundPrefix + "_noise");
    _sampleGroup->remove(_soundPrefix + "_atis");
  }
}

SGSubsystem * CommRadio::createInstance(SGPropertyNode_ptr rootNode)
{
  return new CommRadioImpl(rootNode);
}


// Register the subsystem.
#if 0
SGSubsystemMgr::InstancedRegistrant<CommRadio> registrantCommRadio(
    SGSubsystemMgr::FDM,
    {{"instrumentation", SGSubsystemMgr::Dependency::HARD}});
#endif

} // namespace Instrumentation

