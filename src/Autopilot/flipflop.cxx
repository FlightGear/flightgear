// flipflop.hxx - implementation of multiple flip flop types
//
// Written by Torsten Dreyer
//
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

#include "flipflop.hxx"
#include "functor.hxx"
#include "inputvalue.hxx"
#include <Main/fg_props.hxx>

using namespace FGXMLAutopilot;

class RSFlipFlopImplementation : public FlipFlopImplementation {
protected:
  bool _rIsDominant;
public:
  RSFlipFlopImplementation( bool rIsDominant = true ) : _rIsDominant( rIsDominant ) {}
  virtual bool getState( double dt, DigitalComponent::InputMap input, bool & q );
};

class SRFlipFlopImplementation : public RSFlipFlopImplementation {
public:
  SRFlipFlopImplementation() : RSFlipFlopImplementation( false ) {}
};

class ClockedFlipFlopImplementation : public RSFlipFlopImplementation {
protected:
  bool _clock;
  virtual bool onRaisingEdge( DigitalComponent::InputMap input, bool & q ) = 0;
public:
  ClockedFlipFlopImplementation( bool rIsDominant = true ) : RSFlipFlopImplementation( rIsDominant ), _clock(false) {}
  virtual bool getState( double dt, DigitalComponent::InputMap input, bool & q );
};

class JKFlipFlopImplementation : public ClockedFlipFlopImplementation {
public:
  JKFlipFlopImplementation( bool rIsDominant = true ) : ClockedFlipFlopImplementation ( rIsDominant ) {}
  virtual bool onRaisingEdge( DigitalComponent::InputMap input, bool & q );
};

class DFlipFlopImplementation : public ClockedFlipFlopImplementation {
public:
  DFlipFlopImplementation( bool rIsDominant = true ) : ClockedFlipFlopImplementation ( rIsDominant ) {}
  virtual bool onRaisingEdge( DigitalComponent::InputMap input, bool & q ) {
    q = input.get_value("D");
    return true;
  }
};

class TFlipFlopImplementation : public ClockedFlipFlopImplementation {
public:
  TFlipFlopImplementation( bool rIsDominant = true ) : ClockedFlipFlopImplementation ( rIsDominant ) {}
  virtual bool onRaisingEdge( DigitalComponent::InputMap input, bool & q ) {
    q = !q;
    return true;
  }
};

class MonoFlopImplementation : public JKFlipFlopImplementation {
protected:
  virtual bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode );
  InputValueList _time;
  double _t;
public:
  MonoFlopImplementation( bool rIsDominant = true ) : JKFlipFlopImplementation( rIsDominant ) {}
  virtual bool getState( double dt, DigitalComponent::InputMap input, bool & q );
};

bool MonoFlopImplementation::configure( const std::string & nodeName, SGPropertyNode_ptr configNode )
{
  if( JKFlipFlopImplementation::configure( nodeName, configNode ) )
    return true;

  if (nodeName == "time") {
    _time.push_back( new InputValue( configNode ) );
    return true;
  } 

  return false;
}

bool MonoFlopImplementation::getState( double dt, DigitalComponent::InputMap input, bool & q )
{
  if( JKFlipFlopImplementation::getState( dt, input, q ) ) {
    _t = q ? _time.get_value() : 0;
    return true;
  }

  _t -= dt;
  if( _t <= 0.0 ) {
    q = 0;
    return true;
  }

  return false;
}


bool RSFlipFlopImplementation::getState( double dt, DigitalComponent::InputMap input, bool & q )
{
  bool s = input.get_value("S");
  bool r = input.get_value("R");

  // s == false && q == false: no change, keep state
  if( s || r ) {
    if( _rIsDominant ) { // RS: reset is dominant
      if( s ) q = true; // set
      if( r ) q = false; // reset
    } else { // SR: set is dominant
      if( r ) q = false; // reset
      if( s ) q = true; // set
    }
    return true; // signal state changed
  }
  return false; // signal state unchagned
}

bool ClockedFlipFlopImplementation::getState( double dt, DigitalComponent::InputMap input, bool & q )
{
  if( RSFlipFlopImplementation::getState( dt, input, q ) )
    return true;

  bool c = input.get_value("clock");
  bool raisingEdge = c && !_clock;
    
  _clock = c;

  if( !raisingEdge ) return false; //signal no change
  return onRaisingEdge( input, q );
}

bool JKFlipFlopImplementation::onRaisingEdge( DigitalComponent::InputMap input, bool & q )
{
  bool j = input.get_value("J");
  bool k = input.get_value("K");
    
  // j == false && k == false: no change, keep state
  if( (j || k) ) {
    if( j && k ) {
      q = !q; // toggle
    } else {
      if( j ) q = true;  // set
      if( k ) q = false; // reset
    }
    return true; // signal state changed
  }

  return false; // signal no change
}

bool FlipFlopImplementation::configure( SGPropertyNode_ptr configNode )
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


static map<string,FunctorBase<FlipFlopImplementation> *> componentForge;

bool FlipFlop::configure( const std::string & nodeName, SGPropertyNode_ptr configNode ) 
{ 
  if( componentForge.empty() ) {
    componentForge["RS"] = new CreateAndConfigureFunctor<RSFlipFlopImplementation,FlipFlopImplementation>();
    componentForge["SR"] = new CreateAndConfigureFunctor<SRFlipFlopImplementation,FlipFlopImplementation>();
    componentForge["JK"] = new CreateAndConfigureFunctor<JKFlipFlopImplementation,FlipFlopImplementation>();
    componentForge["D"]  = new CreateAndConfigureFunctor<DFlipFlopImplementation, FlipFlopImplementation>();
    componentForge["T"]  = new CreateAndConfigureFunctor<TFlipFlopImplementation, FlipFlopImplementation>();
    componentForge["monostable"]  = new CreateAndConfigureFunctor<MonoFlopImplementation, FlipFlopImplementation>();
  }

  if( DigitalComponent::configure( nodeName, configNode ) )
    return true;

  if( nodeName == "type" ) {
    string type(configNode->getStringValue());
    if( componentForge.count(type) == 0 ) {
      SG_LOG( SG_AUTOPILOT, SG_BULK, "unhandled flip-flop type <" << type << ">" << endl );
      return true;
    }
    _implementation = (*componentForge[type])( configNode->getParent() );
    return true;
  }

  if (nodeName == "set"||nodeName == "S") {
    _input["S"] = sgReadCondition( fgGetNode("/"), configNode );
    return true;
  }

  if (nodeName == "reset" || nodeName == "R" ) {
    _input["R"] = sgReadCondition( fgGetNode("/"), configNode );
    return true;
  } 

  if (nodeName == "J") {
    _input["J"] = sgReadCondition( fgGetNode("/"), configNode );
    return true;
  } 

  if (nodeName == "K") {
    _input["K"] = sgReadCondition( fgGetNode("/"), configNode );
    return true;
  } 

  if (nodeName == "D") {
    _input["D"] = sgReadCondition( fgGetNode("/"), configNode );
    return true;
  } 

  if (nodeName == "clock") {
    _input["clock"] = sgReadCondition( fgGetNode("/"), configNode );
    return true;
  }

  return false; 
}

void FlipFlop::update( bool firstTime, double dt )
{
  if( _implementation == NULL ) {
    SG_LOG( SG_AUTOPILOT, SG_ALERT, "No flip-flop implementation for " << get_name() << endl );
    return;
  }

  bool q0, q;

  q0 = q = get_output();

  if( _implementation->getState( dt, _input, q ) ) {
    set_output( q );

    if(_debug) {
      cout << "updating flip-flop \"" << get_name() << "\"" << endl;
      cout << "prev. Output:" << q0 << endl;
      for( InputMap::const_iterator it = _input.begin(); it != _input.end(); it++ ) 
        cout << "Input \"" << (*it).first << "\":" << (*it).second->test() << endl;
      cout << "new Output:" << q << endl;
    }
  }
}


