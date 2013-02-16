// digitalcomponent.hxx - Base class for digital autopilot components
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
#ifndef __DIGITALCOMPONENT_HXX
#define __DIGITALCOMPONENT_HXX 1

#include "component.hxx"

#include <simgear/props/props.hxx>
#include <simgear/props/condition.hxx>

namespace FGXMLAutopilot {

/**
 * @brief Models a digital output bound to a property. May be an inverted output.
 */
class DigitalOutput : public SGReferenced {

private:
  bool _inverted;
  SGPropertyNode_ptr _node;

protected:

public:
  /**
   * @brief Constructs an empty, noninverting output
   */
  DigitalOutput();

  inline void setProperty( SGPropertyNode_ptr node );

  inline void setInverted( bool value ) { _inverted = value; }
  inline bool isInverted() const { return _inverted; }

  bool getValue() const;
  void setValue( bool value );
};

inline DigitalOutput::DigitalOutput() : _inverted(false) 
{
}

inline void DigitalOutput::setProperty( SGPropertyNode_ptr node ) 
{ 
  _node = node;
  _node->setBoolValue( node->getBoolValue() );
}

inline bool DigitalOutput::getValue() const 
{
  if( _node == NULL ) return false;
  bool nodeState = _node->getBoolValue();
  return _inverted ? !nodeState : nodeState;
}

inline void DigitalOutput::setValue( bool value ) 
{
  if( _node == NULL ) return;
  _node->setBoolValue( _inverted ? !value : value );
}

typedef SGSharedPtr<DigitalOutput> DigitalOutput_ptr;

/**
 * @brief Base class for digital autopilot components
 *
 * Each digital component has (at least)
 * <ul>
 *   <li>one value input</li>
 *   <li>any number of output properties</li>
 * </ul>
 */
class DigitalComponent : public Component {
public:
    DigitalComponent();

    class InputMap : public std::map<const std::string,SGSharedPtr<const SGCondition> > {
      public:
        bool get_value( const std::string & name ) const;
    };


//    typedef std::map<const std::string,SGSharedPtr<const SGCondition> > InputMap;
    typedef std::map<const std::string,DigitalOutput_ptr> OutputMap;
protected:

    /**
     * @brief Named input "pins"
     */
    InputMap _input;

    /**
     * @brief Named output "pins"
     */
    OutputMap _output;

    /**
     * @brief Global "inverted" flag for the outputs
     */
    bool _inverted;

    /**
     * @brief Over-rideable hook method to allow derived classes to refine top-level
     * node parsing. 
     * @param aName
     * @param aNode
     * @return true if the node was handled, false otherwise.
     */
    virtual bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode );
};

}
#endif // DIGITALCOMPONENT_HXX

