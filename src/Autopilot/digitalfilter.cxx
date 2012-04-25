// digitalfilter.cxx - a selection of digital filters
//
// Written by Torsten Dreyer
// Based heavily on work created by Curtis Olson, started January 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
// Copyright (C) 2010  Torsten Dreyer - Torsten (at) t3r (dot) de
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
#include "functor.hxx"
#include <deque>

using std::map;
using std::string;
using std::endl;
using std::cout;

namespace FGXMLAutopilot {

/**
 *
 *
 */
class DigitalFilterImplementation : public SGReferenced {
protected:
  virtual bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode) = 0;
public:
  virtual ~DigitalFilterImplementation() {}
  DigitalFilterImplementation();
  virtual void   initialize( double output ) {}
  virtual double compute( double dt, double input ) = 0;
  bool configure( SGPropertyNode_ptr configNode );

  void setDigitalFilter( DigitalFilter * digitalFilter ) { _digitalFilter = digitalFilter; }

protected:
  DigitalFilter * _digitalFilter;
};

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */
class GainFilterImplementation : public DigitalFilterImplementation {
protected:
  InputValueList _gainInput;
  bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode );
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
  bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode );
public:
  DerivativeFilterImplementation();
  double compute(  double dt, double input );
};

class ExponentialFilterImplementation : public GainFilterImplementation {
protected:
  InputValueList _TfInput;
  bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode );
  bool _isSecondOrder;
  double output_1, output_2;
public:
  ExponentialFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double output );
};

class MovingAverageFilterImplementation : public DigitalFilterImplementation {
protected:
  InputValueList _samplesInput;
  double _output_1;
  std::deque <double> _inputQueue;
  bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode );
public:
  MovingAverageFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double output );
};

class NoiseSpikeFilterImplementation : public DigitalFilterImplementation {
protected:
  double _output_1;
  InputValueList _rateOfChangeInput;
  bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode );
public:
  NoiseSpikeFilterImplementation();
  double compute(  double dt, double input );
  virtual void initialize( double output );
};

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */

} // namespace FGXMLAutopilot

using namespace FGXMLAutopilot;

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */
DigitalFilterImplementation::DigitalFilterImplementation() :
  _digitalFilter(NULL)
{
}

bool DigitalFilterImplementation::configure( SGPropertyNode_ptr configNode )
{
  for (int i = 0; i < configNode->nChildren(); ++i ) {
    SGPropertyNode_ptr prop;

    SGPropertyNode_ptr child = configNode->getChild(i);
    string cname(child->getName());

    if( configure( cname, child ) )
      continue;

  } // for configNode->nChildren()

  return true;
}

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */

double GainFilterImplementation::compute(  double dt, double input )
{
  return _gainInput.get_value() * input;
}

bool GainFilterImplementation::configure( const std::string & nodeName, SGPropertyNode_ptr configNode )
{
  if (nodeName == "gain" ) {
    _gainInput.push_back( new InputValue( configNode, 1 ) );
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

bool DerivativeFilterImplementation::configure( const std::string & nodeName, SGPropertyNode_ptr configNode )
{
  if( GainFilterImplementation::configure( nodeName, configNode ) )
    return true;

  if (nodeName == "filter-time" ) {
    _TfInput.push_back( new InputValue( configNode, 1 ) );
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

void MovingAverageFilterImplementation::initialize( double output )
{
  _output_1 = output;
}

double MovingAverageFilterImplementation::compute(  double dt, double input )
{
  std::deque<double>::size_type samples = _samplesInput.get_value();
  _inputQueue.resize(samples+1, 0.0);

  double output_0 = _output_1 + (input - _inputQueue.back()) / samples;

  _output_1 = output_0;
  _inputQueue.push_front(input);
  return output_0;
}

bool MovingAverageFilterImplementation::configure( const std::string & nodeName, SGPropertyNode_ptr configNode )
{
  if (nodeName == "samples" ) {
    _samplesInput.push_back( new InputValue( configNode, 1 ) );
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

void NoiseSpikeFilterImplementation::initialize( double output )
{
  _output_1 = output;
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

bool NoiseSpikeFilterImplementation::configure( const std::string & nodeName, SGPropertyNode_ptr configNode )
{
  if (nodeName == "max-rate-of-change" ) {
    _rateOfChangeInput.push_back( new InputValue( configNode, 1 ) );
    return true;
  }

  return false;
}

/* --------------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------------- */

ExponentialFilterImplementation::ExponentialFilterImplementation()
  : _isSecondOrder(false),
    output_1(0.0),
    output_2(0.0)
{
}

void ExponentialFilterImplementation::initialize( double output )
{
  output_1 = output_2 = output;
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
               2 * (1 - alpha) * output_1 -
              (1 - alpha) * (1 - alpha) * output_2;
  } else {
    output_0 = alpha * input + (1 - alpha) * output_1;
  }
  output_2 = output_1;
  return (output_1 = output_0);
}

bool ExponentialFilterImplementation::configure( const std::string & nodeName, SGPropertyNode_ptr configNode )
{
  if( GainFilterImplementation::configure( nodeName, configNode ) )
    return true;

  if (nodeName == "filter-time" ) {
    _TfInput.push_back( new InputValue( configNode, 1 ) );
    return true;
  }

  if (nodeName == "type" ) {
    string type(configNode->getStringValue());
    _isSecondOrder = type == "double-exponential";
  }

  return false;
}

/* --------------------------------------------------------------------------------- */
/* Digital Filter Component Implementation                                           */
/* --------------------------------------------------------------------------------- */

DigitalFilter::DigitalFilter() :
    AnalogComponent(),
    _initializeTo(INITIALIZE_INPUT)
{
}

DigitalFilter::~DigitalFilter()
{
}


static map<string,FunctorBase<DigitalFilterImplementation> *> componentForge;

bool DigitalFilter::configure(const string& nodeName, SGPropertyNode_ptr configNode)
{
  if( componentForge.empty() ) {
    componentForge["gain"] = new CreateAndConfigureFunctor<GainFilterImplementation,DigitalFilterImplementation>();
    componentForge["exponential"] = new CreateAndConfigureFunctor<ExponentialFilterImplementation,DigitalFilterImplementation>();
    componentForge["double-exponential"] = new CreateAndConfigureFunctor<ExponentialFilterImplementation,DigitalFilterImplementation>();
    componentForge["moving-average"] = new CreateAndConfigureFunctor<MovingAverageFilterImplementation,DigitalFilterImplementation>();
    componentForge["noise-spike"] = new CreateAndConfigureFunctor<NoiseSpikeFilterImplementation,DigitalFilterImplementation>();
    componentForge["reciprocal"] = new CreateAndConfigureFunctor<ReciprocalFilterImplementation,DigitalFilterImplementation>();
    componentForge["derivative"] = new CreateAndConfigureFunctor<DerivativeFilterImplementation,DigitalFilterImplementation>();
  }

  SG_LOG( SG_AUTOPILOT, SG_BULK, "DigitalFilter::configure(" << nodeName << ")" << endl );
  if( AnalogComponent::configure( nodeName, configNode ) )
    return true;

  if (nodeName == "type" ) {
    string type( configNode->getStringValue() );
    if( componentForge.count(type) == 0 ) {
      SG_LOG( SG_AUTOPILOT, SG_BULK, "unhandled filter type <" << type << ">" << endl );
      return true;
    }
    _implementation = (*componentForge[type])( configNode->getParent() );
    _implementation->setDigitalFilter( this );
    return true;
  }

  if( nodeName == "initialize-to" ) {
    string s( configNode->getStringValue() );
    if( s == "input" ) {
      _initializeTo = INITIALIZE_INPUT;
    } else if( s == "output" ) {
      _initializeTo = INITIALIZE_OUTPUT;
    } else if( s == "none" ) {
      _initializeTo = INITIALIZE_NONE;
    } else {
      SG_LOG( SG_AUTOPILOT, SG_WARN, "unhandled initialize-to value '" << s << "' ignored" );
    }
    return true;
  }

  SG_LOG( SG_AUTOPILOT, SG_BULK, "DigitalFilter::configure(" << nodeName << ") [unhandled]" << endl );
  return false; // not handled by us, let the base class try
}

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
    cout << "input:" << input
         << "\toutput:" << output << endl;
  }
}
