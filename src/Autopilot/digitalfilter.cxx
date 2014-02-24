// digitalfilter.cxx - a selection of digital filters
//
// Written by Torsten Dreyer
// Based heavily on work created by Curtis Olson, started January 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
// Copyright (C) 2010  Torsten Dreyer - Torsten (at) t3r (dot) de
//
// Washout/high-pass filter, lead-lag filter and integrator added.
// low-pass and lag aliases added to Exponential filter, 
// rate-limit added.   A J Teeder 2013
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

#include "digitalfilter.hxx"
#include <deque>

namespace FGXMLAutopilot
{

/**
 *
 *
 */
class DigitalFilterImplementation:
  public SGReferenced
{
  public:
    virtual ~DigitalFilterImplementation() {}
    DigitalFilterImplementation();
    virtual void   initialize( double initvalue ) {}
    virtual double compute( double dt, double input ) = 0;
    virtual bool configure( SGPropertyNode& cfg_node,
                            const std::string& cfg_name,
                            SGPropertyNode& prop_root ) = 0;

    void setDigitalFilter( DigitalFilter * digitalFilter ) { _digitalFilter = digitalFilter; }

  protected:
    DigitalFilter * _digitalFilter;
};

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */
class GainFilterImplementation : public DigitalFilterImplementation {
protected:
  InputValueList _gainInput;
  bool configure( SGPropertyNode& cfg_node,
                  const std::string& cfg_name,
                  SGPropertyNode& prop_root );
public:
  GainFilterImplementation() : _gainInput(1.0) {}
  double compute(  double dt, double input );
};

class ReciprocalFilterImplementation : public GainFilterImplementation {
public:
  double compute(  double dt, double input );
};

class DerivativeFilterImplementation : public GainFilterImplementation {
  InputValueList _TfInput;
  double _input_1;
  bool configure( SGPropertyNode& cfg_node,
                  const std::string& cfg_name,
                  SGPropertyNode& prop_root );
public:
  DerivativeFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double initvalue );
};

class ExponentialFilterImplementation : public GainFilterImplementation {
protected:
  InputValueList _TfInput;
  bool configure( SGPropertyNode& cfg_node,
                  const std::string& cfg_name,
                  SGPropertyNode& prop_root );
  bool _isSecondOrder;
  double _output_1, _output_2;
public:
  ExponentialFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double initvalue );
};

class MovingAverageFilterImplementation : public DigitalFilterImplementation {
protected:
  InputValueList _samplesInput;
  double _output_1;
  std::deque <double> _inputQueue;
  bool configure( SGPropertyNode& cfg_node,
                  const std::string& cfg_name,
                  SGPropertyNode& prop_root );
public:
  MovingAverageFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double initvalue );
};

class NoiseSpikeFilterImplementation : public DigitalFilterImplementation {
protected:
  double _output_1;
  InputValueList _rateOfChangeInput;
  bool configure( SGPropertyNode& cfg_node,
                  const std::string& cfg_name,
                  SGPropertyNode& prop_root );
public:
  NoiseSpikeFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double initvalue );
};

class RateLimitFilterImplementation : public DigitalFilterImplementation {
protected:
  double _output_1;
  InputValueList _rateOfChangeMax;
  InputValueList _rateOfChangeMin ;
  bool configure( SGPropertyNode& cfg_node,
                  const std::string& cfg_name,
                  SGPropertyNode& prop_root );
public:
  RateLimitFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double initvalue );
};

class IntegratorFilterImplementation : public GainFilterImplementation {
protected:
  InputValueList _TfInput;
  InputValueList _minInput;
  InputValueList _maxInput;
  double _input_1;
  double _output_1;
  bool configure( SGPropertyNode& cfg_node,
                  const std::string& cfg_name,
                  SGPropertyNode& prop_root );
public:
  IntegratorFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double initvalue );
};

class HighPassFilterImplementation : public GainFilterImplementation {
protected:
  InputValueList _TfInput;
  double _input_1;
  double _output_1;
  bool configure( SGPropertyNode& cfg_node,
                  const std::string& cfg_name,
                  SGPropertyNode& prop_root );
public:
  HighPassFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double initvalue );
};
class LeadLagFilterImplementation : public GainFilterImplementation {
protected:
  InputValueList _TfaInput;
  InputValueList _TfbInput;
  double _input_1;
  double _output_1;
  bool configure( SGPropertyNode& cfg_node,
                  const std::string& cfg_name,
                  SGPropertyNode& prop_root );
public:
  LeadLagFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double initvalue );
};
/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */

} // namespace FGXMLAutopilot

using namespace FGXMLAutopilot;

//------------------------------------------------------------------------------
DigitalFilterImplementation::DigitalFilterImplementation() :
  _digitalFilter(NULL)
{

}

//------------------------------------------------------------------------------
double GainFilterImplementation::compute(  double dt, double input )
{
  return _gainInput.get_value() * input;
}

bool GainFilterImplementation::configure( SGPropertyNode& cfg_node,
                                          const std::string& cfg_name,
                                          SGPropertyNode& prop_root )
{
  if (cfg_name == "gain" ) {
    _gainInput.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }

  return false;
}

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */

double ReciprocalFilterImplementation::compute(  double dt, double input )
{
  if( input >= -SGLimitsd::min() && input <= SGLimitsd::min() )
    return SGLimitsd::max();

  return _gainInput.get_value() / input;

}

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */

DerivativeFilterImplementation::DerivativeFilterImplementation() :
  _input_1(0.0)
{
}

void DerivativeFilterImplementation::initialize( double initvalue )
{
  _input_1 = initvalue;
}

//------------------------------------------------------------------------------
bool DerivativeFilterImplementation::configure( SGPropertyNode& cfg_node,
                                                const std::string& cfg_name,
                                                SGPropertyNode& prop_root )
{
  if( GainFilterImplementation::configure(cfg_node, cfg_name, prop_root) )
    return true;

  if (cfg_name == "filter-time" ) {
    _TfInput.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }

  return false;
}

double DerivativeFilterImplementation::compute(  double dt, double input )
{
  double output = (input - _input_1) * _TfInput.get_value() * _gainInput.get_value() / dt;
  _input_1 = input;
  return output;

}

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */

MovingAverageFilterImplementation::MovingAverageFilterImplementation() :
  _output_1(0.0)
{
}

void MovingAverageFilterImplementation::initialize( double initvalue )
{
  _output_1 = initvalue;
}

double MovingAverageFilterImplementation::compute(  double dt, double input )
{
  typedef std::deque<double>::size_type size_type;
  size_type samples = _samplesInput.get_value();

  if (_inputQueue.size() != samples) {
    // For constant size filters, this code executed once.
    bool shrunk = _inputQueue.size() > samples;
    _inputQueue.resize(samples, _output_1);
    if (shrunk) {
      _output_1 = 0.0;
      for (size_type ii = 0; ii < samples; ii++)
      _output_1 += _inputQueue[ii];
      _output_1 /= samples;
    }
  }

  double output_0 = _output_1 + (input - _inputQueue.back()) / samples;

  _output_1 = output_0;
  _inputQueue.pop_back();
  _inputQueue.push_front(input);
  return output_0;
}

bool MovingAverageFilterImplementation::configure( SGPropertyNode& cfg_node,
                                                   const std::string& cfg_name,
                                                   SGPropertyNode& prop_root )
{
  if (cfg_name == "samples" ) {
    _samplesInput.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }

  return false;
}

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */

NoiseSpikeFilterImplementation::NoiseSpikeFilterImplementation() :
  _output_1(0.0)
{
}

void NoiseSpikeFilterImplementation::initialize( double initvalue )
{
  _output_1 = initvalue;
}

double NoiseSpikeFilterImplementation::compute(  double dt, double input )
{
  double delta = input - _output_1;
  if( fabs(delta) <= SGLimitsd::min() ) return input; // trivial

  double maxChange = _rateOfChangeInput.get_value() * dt;
  const PeriodicalValue * periodical = _digitalFilter->getPeriodicalValue();
  if( periodical ) delta = periodical->normalizeSymmetric( delta );

  if( fabs(delta) <= maxChange )
    return (_output_1 = input);
  else
    return (_output_1 = _output_1 + copysign( maxChange, delta ));
}

//------------------------------------------------------------------------------
bool NoiseSpikeFilterImplementation::configure( SGPropertyNode& cfg_node,
                                                const std::string& cfg_name,
                                                SGPropertyNode& prop_root )
{
  if (cfg_name == "max-rate-of-change" ) {
    _rateOfChangeInput.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }

  return false;
}

/* --------------------------------------------------------------------------------- */

RateLimitFilterImplementation::RateLimitFilterImplementation() :
  _output_1(0.0)
{
}

void RateLimitFilterImplementation::initialize( double initvalue )
{
  _output_1 = initvalue;
}

double RateLimitFilterImplementation::compute(  double dt, double input )
{
  double delta = input - _output_1;
  double output;

  if( fabs(delta) <= SGLimitsd::min() ) return input; // trivial

  double maxChange = _rateOfChangeMax.get_value() * dt;
  double minChange = _rateOfChangeMin.get_value() * dt;
//  const PeriodicalValue * periodical = _digitalFilter->getPeriodicalValue();
//  if( periodical ) delta = periodical->normalizeSymmetric( delta );

  output = input;
  if(delta >= maxChange ) output = _output_1 + maxChange;
  if(delta <= minChange ) output = _output_1 + minChange;
  _output_1 = output;

  return (output);
}

bool RateLimitFilterImplementation::configure( SGPropertyNode& cfg_node,
                                               const std::string& cfg_name,
                                               SGPropertyNode& prop_root )
{
  std::cout << "RateLimitFilterImplementation " << cfg_name << std::endl;
  if (cfg_name == "max-rate-of-change" ) {
    _rateOfChangeMax.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }
  if (cfg_name == "min-rate-of-change" ) {
    _rateOfChangeMin.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }

  return false;
}

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */

ExponentialFilterImplementation::ExponentialFilterImplementation()
  : _isSecondOrder(false),
    _output_1(0.0),
    _output_2(0.0)
{
}

void ExponentialFilterImplementation::initialize( double initvalue )
{
  _output_1 = _output_2 = initvalue;
}

double ExponentialFilterImplementation::compute(  double dt, double input )
{
  input = GainFilterImplementation::compute( dt, input );
  double tf = _TfInput.get_value();

  double output_0;

  // avoid negative filter times 
  // and div by zero if -tf == dt

  double alpha = tf > 0.0 ? 1 / ((tf/dt) + 1) : 1.0;
 
  if(_isSecondOrder) {
    output_0 = alpha * alpha * input + 
               2 * (1 - alpha) * _output_1 -
              (1 - alpha) * (1 - alpha) * _output_2;
  } else {
    output_0 = alpha * input + (1 - alpha) * _output_1;
  }
  _output_2 = _output_1;
  return (_output_1 = output_0);
}

//------------------------------------------------------------------------------
bool ExponentialFilterImplementation::configure( SGPropertyNode& cfg_node,
                                                 const std::string& cfg_name,
                                                 SGPropertyNode& prop_root )
{
  if( GainFilterImplementation::configure(cfg_node, cfg_name, prop_root) )
    return true;

  if (cfg_name == "filter-time" ) {
    _TfInput.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }

  if (cfg_name == "type" ) {
    std::string type(cfg_node.getStringValue());
    _isSecondOrder = type == "double-exponential";
  }

  return false;
}

/* --------------------------------------------------------------------------------- */

IntegratorFilterImplementation::IntegratorFilterImplementation() :
  _input_1(0.0),
  _output_1(0.0)
{
}

void IntegratorFilterImplementation::initialize( double initvalue )
{
  _input_1 = _output_1 = initvalue;
}

//------------------------------------------------------------------------------
bool IntegratorFilterImplementation::configure( SGPropertyNode& cfg_node,
                                                const std::string& cfg_name,
                                                SGPropertyNode& prop_root )
{
  if( GainFilterImplementation::configure(cfg_node, cfg_name, prop_root) )
    return true;

  if (cfg_name == "u_min" ) {
    _minInput.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }
  if (cfg_name == "u_max" ) {
    _maxInput.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }
  return false;
}

double IntegratorFilterImplementation::compute(  double dt, double input )
{
  double output = _output_1 + input *  _gainInput.get_value() * dt;
  double u_min = _minInput.get_value();
  double u_max = _maxInput.get_value();
  if (output >= u_max) output = u_max; // clamping inside "::compute" prevents integrator wind-up
  if (output <= u_min) output = u_min;
  _input_1 = input;
  _output_1 = output;
  return output;

}

/* --------------------------------------------------------------------------------- */

HighPassFilterImplementation::HighPassFilterImplementation() :
  _input_1(0.0),
  _output_1(0.0)

{
}

void HighPassFilterImplementation::initialize( double initvalue )
{
  _input_1 = initvalue;
  _output_1 = initvalue;
}

double HighPassFilterImplementation::compute(  double dt, double input )
{
  input = GainFilterImplementation::compute( dt, input );
  double tf = _TfInput.get_value();

  double output;

  // avoid negative filter times 
  // and div by zero if -tf == dt

  double alpha = tf > 0.0 ? 1 / ((tf/dt) + 1) : 1.0;
  output = (1 - alpha) * (input - _input_1 +  _output_1);
  _input_1 = input;
  _output_1 = output;
  return output;
}

//------------------------------------------------------------------------------
bool HighPassFilterImplementation::configure( SGPropertyNode& cfg_node,
                                              const std::string& cfg_name,
                                              SGPropertyNode& prop_root )
{
  if( GainFilterImplementation::configure(cfg_node, cfg_name, prop_root) )
    return true;

  if (cfg_name == "filter-time" ) {
    _TfInput.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }
 
  return false;
}

/* --------------------------------------------------------------------------------- */

LeadLagFilterImplementation::LeadLagFilterImplementation() :
  _input_1(0.0),
  _output_1(0.0)

{
}

void LeadLagFilterImplementation::initialize( double initvalue )
{
  _input_1 = initvalue;
  _output_1 = initvalue;
}

double LeadLagFilterImplementation::compute(  double dt, double input )
{
  input = GainFilterImplementation::compute( dt, input );
  double tfa = _TfaInput.get_value();
  double tfb = _TfbInput.get_value();

  double output;

  // avoid negative filter times 
  // and div by zero if -tf == dt

  double alpha = tfa > 0.0 ? 1 / ((tfa/dt) + 1) : 1.0;
  double beta = tfb > 0.0 ? 1 / ((tfb/dt) + 1) : 1.0;
  output = (1 - beta) * (input / (1 - alpha) - _input_1 + _output_1);
  _input_1 = input;
  _output_1 = output;
  return output;
}

//------------------------------------------------------------------------------
bool LeadLagFilterImplementation::configure( SGPropertyNode& cfg_node,
                                             const std::string& cfg_name,
                                             SGPropertyNode& prop_root )
{
  if( GainFilterImplementation::configure(cfg_node, cfg_name, prop_root) )
    return true;

  if (cfg_name == "filter-time-a" ) {
    _TfaInput.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }
  if (cfg_name == "filter-time-b" ) {
    _TfbInput.push_back( new InputValue(prop_root, cfg_node, 1) );
    return true;
  }
  return false;
}
/* -------------------------------------------------------------------------- */
/* Digital Filter Component Implementation                                    */
/* -------------------------------------------------------------------------- */

DigitalFilter::DigitalFilter() :
    AnalogComponent(),
    _initializeTo(INITIALIZE_INPUT)
{
}

DigitalFilter::~DigitalFilter()
{
}

//------------------------------------------------------------------------------
template<class DigitalFilterType>
DigitalFilterImplementation* digitalFilterFactory()
{
  return new DigitalFilterType();
}

typedef std::map<std::string, DigitalFilterImplementation*(*)()>
DigitalFilterMap;
static DigitalFilterMap componentForge;

//------------------------------------------------------------------------------
bool DigitalFilter::configure( SGPropertyNode& prop_root,
                               SGPropertyNode& cfg )
{
  if( componentForge.empty() )
  {
    componentForge["gain"               ] = digitalFilterFactory<GainFilterImplementation>;
    componentForge["exponential"        ] = digitalFilterFactory<ExponentialFilterImplementation>;
    componentForge["double-exponential" ] = digitalFilterFactory<ExponentialFilterImplementation>;
    componentForge["moving-average"     ] = digitalFilterFactory<MovingAverageFilterImplementation>;
    componentForge["noise-spike"        ] = digitalFilterFactory<NoiseSpikeFilterImplementation>;
    componentForge["rate-limit"         ] = digitalFilterFactory<RateLimitFilterImplementation>;
    componentForge["reciprocal"         ] = digitalFilterFactory<ReciprocalFilterImplementation>;
    componentForge["derivative"         ] = digitalFilterFactory<DerivativeFilterImplementation>;
    componentForge["high-pass"          ] = digitalFilterFactory<HighPassFilterImplementation>;
    componentForge["lead-lag"           ] = digitalFilterFactory<LeadLagFilterImplementation>;
    componentForge["integrator"         ] = digitalFilterFactory<IntegratorFilterImplementation>;
  }

  const std::string type = cfg.getStringValue("type");
  DigitalFilterMap::iterator component_factory = componentForge.find(type);
  if( component_factory == componentForge.end() )
  {
    SG_LOG(SG_AUTOPILOT, SG_WARN, "unhandled filter type '" << type << "'");
    return false;
  }

  _implementation = (*component_factory->second)();
  _implementation->setDigitalFilter( this );

  for( int i = 0; i < cfg.nChildren(); ++i )
  {
    SGPropertyNode_ptr child = cfg.getChild(i);
    std::string cname(child->getName());

    if(    !_implementation->configure(*child, cname, prop_root)
        && !configure(*child, cname, prop_root)
        && cname != "type"
        && cname != "params" ) // 'params' is usually used to specify parameters
                               // in PropertList files.
      SG_LOG
      (
        SG_AUTOPILOT,
        SG_WARN,
        "DigitalFilter: unknown config node: " << cname
      );
  }

  return true;
}

//------------------------------------------------------------------------------
bool DigitalFilter::configure( SGPropertyNode& cfg_node,
                               const std::string& cfg_name,
                               SGPropertyNode& prop_root )
{
  if( cfg_name == "initialize-to" )
  {
    std::string s( cfg_node.getStringValue() );
    if( s == "input" )
      _initializeTo = INITIALIZE_INPUT;
    else if( s == "output" )
      _initializeTo = INITIALIZE_OUTPUT;
    else if( s == "none" )
      _initializeTo = INITIALIZE_NONE;
    else
      SG_LOG
      (
        SG_AUTOPILOT,
        SG_WARN, "DigitalFilter: initialize-to (" << s << ") ignored"
      );

    return true;
  }

  return AnalogComponent::configure(cfg_node, cfg_name, prop_root);
}

//------------------------------------------------------------------------------
void DigitalFilter::update( bool firstTime, double dt)
{
  if( _implementation == NULL ) return;

  if( firstTime ) {
    switch( _initializeTo ) {

      case INITIALIZE_INPUT:
        SG_LOG(SG_AUTOPILOT,SG_DEBUG, "First time initialization of " << get_name() << " to " << _valueInput.get_value() );
        _implementation->initialize( _valueInput.get_value() );
        break;

      case INITIALIZE_OUTPUT:
        SG_LOG(SG_AUTOPILOT,SG_DEBUG, "First time initialization of " << get_name() << " to " << get_output_value() );
        _implementation->initialize( get_output_value() );
        break;

      default:
        SG_LOG(SG_AUTOPILOT,SG_DEBUG, "First time initialization of " << get_name() << " to (uninitialized)" );
        break;
    }
  }

  double input = _valueInput.get_value() - _referenceInput.get_value();
  double output = _implementation->compute( dt, input );

  set_output_value( output );

  if(_debug) {
    std::cout << "input:" << input
              << "\toutput:" << output << std::endl;
  }
}
