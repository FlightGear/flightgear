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

using std::map;
using std::string;
using std::endl;
using std::cout;

namespace FGXMLAutopilot {

/**
 * @brief Flip flop implementation for a RS flip flop with dominant RESET
 *
 * RS (reset-set) flip flops act as a fundamental latch. It has two input lines, 
 * S (set) and R (reset). Activating the set input sets the output while activating
 * the reset input resets the output. If both inputs are activated, the output
 * is deactivated, too. This is why the RESET line is called dominant. Use a
 * SRFlipFlopImplementation for a dominant SET line.
 *
 * <table>
 * <tr>
 * <td colspan="3">Logictable</td>
 * </tr>
 * <tr>
 *   <td>S</td><td>R</td><td>Q</td>
 * </tr>
 * <tr>
 *   <td>false</td><td>false</td><td>unchanged</td>
 * </tr>
 * <tr>
 *   <td>false</td><td>true</td><td>false</td>
 * </tr>
 * <tr>
 *   <td>true</td><td>false</td><td>true</td>
 * </tr>
 * <tr>
 *   <td>true</td><td>true</td><td>false</td>
 * </tr>
 * </table>
 */
class RSFlipFlopImplementation : public FlipFlopImplementation {
protected:
  bool _rIsDominant;
public:
  RSFlipFlopImplementation( bool rIsDominant = true ) : _rIsDominant( rIsDominant ) {}
  virtual bool getState( double dt, DigitalComponent::InputMap input, bool & q );
};

/**
 * @brief Flip flop implementation for a RS flip flop with dominant SET
 *
 * SR (set-reset) flip flops act as a fundamental latch. It has two input lines, 
 * S (set) and R (reset). Activating the set input sets the output while activating
 * the reset input resets the output. If both inputs are activated, the output
 * is activated, too. This is why the SET line is called dominant. Use a
 * RSFlipFlopImplementation for a dominant RESET line.
 * 
 * <table>
 * <tr>
 * <td colspan="3">Logictable</td>
 * </tr>
 * <tr>
 *   <td>S</td><td>R</td><td>Q</td>
 * </tr>
 * <tr>
 *   <td>false</td><td>false</td><td>unchanged</td>
 * </tr>
 * <tr>
 *   <td>false</td><td>true</td><td>false</td>
 * </tr>
 * <tr>
 *   <td>true</td><td>false</td><td>true</td>
 * </tr>
 * <tr>
 *   <td>true</td><td>true</td><td>true</td>
 * </tr>
 * </table>
 */
class SRFlipFlopImplementation : public RSFlipFlopImplementation {
public:
  SRFlipFlopImplementation() : RSFlipFlopImplementation( false ) {}
};

/**
 * @brief Base class for clocked flip flop implementation
 *
 * A clocked flip flop computes it's output on the raising edge (false/true transition)
 * of the clock input. If such a transition is detected, the onRaisingEdge method is called
 * by this implementation. All clocked flip flops inherit from the RS flip flop and may
 * be set or reset by the respective set/reset lines. Note that the RS implementation 
 * ignores the clock, The output is set immediately, regardless of the state of the clock
 * input. The "clock" input is mandatory for clocked flip flops.
 * 
 */
class ClockedFlipFlopImplementation : public RSFlipFlopImplementation {
private:
  /** 
   * @brief the previous state of the clock input 
   */
  bool _clock;
protected:

  /**
   * @brief pure virtual function to be implemented from the implementing class, gets called
   * from the update method if the raising edge of the clock input was detected. 
   * @param input a map of named input lines
   * @param q a reference to a boolean variable to receive the output state
   * @return true if the state has changed, false otherwise
   */
  virtual bool onRaisingEdge( DigitalComponent::InputMap input, bool & q ) = 0;
public:

  /**
   * @brief constructor for a ClockedFlipFlopImplementation
   * @param rIsDominant boolean flag to signal if RESET shall be dominant (true) or SET shall be dominant (false)
   */
  ClockedFlipFlopImplementation( bool rIsDominant = true ) : RSFlipFlopImplementation( rIsDominant ), _clock(false) {}

  /**
   * @brief evaluates the output state from the input lines. 
   * This method basically waits for a raising edge and calls onRaisingEdge
   * @param dt the elapsed time in seconds from since the last call
   * @param input a map of named input lines
   * @param q a reference to a boolean variable to receive the output state
   * @return true if the state has changed, false otherwise
   */
  virtual bool getState( double dt, DigitalComponent::InputMap input, bool & q );
};

/**
 * @brief Implements a JK flip flop as a clocked flip flop
 *
 * The JK flip flop has five input lines: R, S, clock, J and K. The R and S lines work as described
 * in the RS flip flop. Setting the J line to true sets the output to true on the next raising
 * edge of the clock line. Setting the K line to true sets the output to false on the next raising
 * edge of the clock line. If both, J and K are true, the output is toggled at with every raising
 * edge of the clock line. 
 *
 * Undefined inputs default to false.
 *
 * <table>
 * <tr>
 * <td colspan="7">Logictable</td>
 * </tr>
 * <tr>
 *   <td>S</td><td>R</td><td>J</td><td>K</td><td>clock</td><td>Q (previous)</td><td>Q</td>
 * </tr>
 * <tr>
 *   <td>false</td><td>false</td><td>false</td><td>false</td><td>any</td><td>any</td><td>unchanged</td>
 * </tr>
 * <tr>
 *   <td>true</td><td>false</td><td>any</td><td>any</td><td>any</td><td>any</td><td>true</td>
 * </tr>
 * <tr>
 *   <td>any</td><td>true</td><td>any</td><td>any</td><td>any</td><td>any</td><td>false</td>
 * </tr>
 * <tr>
 *   <td>false</td><td>false</td><td>true</td><td>false</td><td>^</td><td>any</td><td>true</td>
 * </tr>
 * <tr>
 *   <td>false</td><td>false</td><td>false</td><td>true</td><td>^</td><td>any</td><td>false</td>
 * </tr>
 * <tr>
 *   <td>false</td><td>false</td><td>true</td><td>true</td><td>^</td><td>false</td><td>true</td>
 * </tr>
 * <tr>
 *   <td>false</td><td>false</td><td>true</td><td>true</td><td>^</td><td>true</td><td>false</td>
 * </tr>
 * </table>
 */
class JKFlipFlopImplementation : public ClockedFlipFlopImplementation {
public:
  /**
   * @brief constructor for a JKFlipFlopImplementation
   * @param rIsDominant boolean flag to signal if RESET shall be dominant (true) or SET shall be dominant (false)
   */
  JKFlipFlopImplementation( bool rIsDominant = true ) : ClockedFlipFlopImplementation ( rIsDominant ) {}

  /**
   * @brief compute the output state according to the logic table on the raising edge of the clock
   * @param input a map of named input lines
   * @param q a reference to a boolean variable to receive the output state
   * @return true if the state has changed, false otherwise
   */
  virtual bool onRaisingEdge( DigitalComponent::InputMap input, bool & q );
};

/** 
 * @brief Implements a D (delay) flip flop.
 *
 */
class DFlipFlopImplementation : public ClockedFlipFlopImplementation {
public:
  /**
   * @brief constructor for a DFlipFlopImplementation
   * @param rIsDominant boolean flag to signal if RESET shall be dominant (true) or SET shall be dominant (false)
   */
  DFlipFlopImplementation( bool rIsDominant = true ) : ClockedFlipFlopImplementation ( rIsDominant ) {}

  /**
   * @brief compute the output state according to the logic table on the raising edge of the clock
   * @param input a map of named input lines
   * @param q a reference to a boolean variable to receive the output state
   * @return true if the state has changed, false otherwise
   */
  virtual bool onRaisingEdge( DigitalComponent::InputMap input, bool & q ) {
    q = input.get_value("D");
    return true;
  }
};

/** 
 * @brief Implements a T (toggle) flip flop.
 *
 */
class TFlipFlopImplementation : public ClockedFlipFlopImplementation {
public:
  /**
   * @brief constructor for a TFlipFlopImplementation
   * @param rIsDominant boolean flag to signal if RESET shall be dominant (true) or SET shall be dominant (false)
   */
  TFlipFlopImplementation( bool rIsDominant = true ) : ClockedFlipFlopImplementation ( rIsDominant ) {}

  /**
   * @brief compute the output state according to the logic table on the raising edge of the clock
   * @param input a map of named input lines
   * @param q a reference to a boolean variable to receive the output state
   * @return true if the state has changed, false otherwise
   */
  virtual bool onRaisingEdge( DigitalComponent::InputMap input, bool & q ) {
    q = !q;
    return true;
  }
};

/** 
 * @brief Implements a monostable flip flop
 *
 * The stable output state is false.
 *
 */
class MonoFlopImplementation : public JKFlipFlopImplementation {
protected:
  virtual bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode );
  InputValueList _time;
  double _t;
public:
  /**
   * @brief constructor for a MonoFlopImplementation
   * @param rIsDominant boolean flag to signal if RESET shall be dominant (true) or SET shall be dominant (false)
   */
  MonoFlopImplementation( bool rIsDominant = true ) : JKFlipFlopImplementation( rIsDominant ), _t(0.0) {}
  /**
   * @brief evaluates the output state from the input lines and returns to the stable state 
   * after expiry of the internal timer
   * @param dt the elapsed time in seconds from since the last call
   * @param input a map of named input lines
   * @param q a reference to a boolean variable to receive the output state
   * @return true if the state has changed, false otherwise
   */
  virtual bool getState( double dt, DigitalComponent::InputMap input, bool & q );
};

} // namespace

using namespace FGXMLAutopilot;

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
  bool c = input.get_value("clock");
  bool raisingEdge = c && !_clock;
    
  _clock = c;

  if( RSFlipFlopImplementation::getState( dt, input, q ) )
    return true;


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

  if( _implementation->getState( dt, _input, q ) && q0 != q ) {
    set_output( q );

    if(_debug) {
      cout << "updating flip-flop \"" << get_name() << "\"" << endl;
      cout << "prev. Output:" << q0 << endl;
      for( InputMap::const_iterator it = _input.begin(); it != _input.end(); ++it ) 
        cout << "Input \"" << (*it).first << "\":" << (*it).second->test() << endl;
      cout << "new Output:" << q << endl;
    }
  }
}


