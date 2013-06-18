// analogcomponent.hxx - Base class for analog autopilot components
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
#ifndef __ANALOGCOMPONENT_HXX
#define __ANALOGCOMPONENT_HXX 1

#include "inputvalue.hxx"
#include "component.hxx"

namespace FGXMLAutopilot {

/**
 * @brief Base class for analog autopilot components
 *
 * Each analog component has
 * <ul>
 *   <li>one value input</li>
 *   <li>one reference input</li>
 *   <li>one minimum clamp input</li>
 *   <li>one maximum clamp input</li>
 *   <li>an optional periodical definition</li>
 * </ul>
 */
class AnalogComponent : public Component {

private:
    /**
     * @brief a flag signalling that the output property value shall be fed back
     *        to the active input property if this component is disabled. This flag
     *        reflects the &lt;feedback-if-disabled&gt; boolean property.
     */
    bool _feedback_if_disabled;

protected:
    /**
     * @brief the value input
     */
    InputValueList _valueInput;

    /**
     * @brief the reference input
     */
    InputValueList _referenceInput;

    /**
     * @brief the minimum output clamp input
     */
    InputValueList _minInput;

    /**
     * @brief the maximum output clamp input
     */
    InputValueList _maxInput;

    /**
     * @brief the configuration for periodical outputs
     */
    PeriodicalValue_ptr _periodical;

    /**
     * @brief A constructor for an analog component. Call configure() to 
     * configure this component from a property node
     */
    AnalogComponent();

    /**
     * @brief This method configures this analog component from a property node. Gets
     *        called multiple times from the base class configure method for every 
              config node.
     * @param nodeName the name of the configuration node provided in configNode
     * @param configNode the configuration node itself
     * @return true if the node was handled, false otherwise.
     */
    virtual bool configure( const std::string & nodeName, SGPropertyNode_ptr configNode );

    /**
     * @brief clamp the given value if &lt;min&gt; and/or &lt;max&gt; inputs were given
     * @param the value to clamp
     * @return the clamped value
     */
    double clamp( double value ) const;

   /**
    * @brief overideable method being called from the update() method if this component
    *        is disabled. Analog components feed back it's output value to the active 
             input value if disabled and feedback-if-disabled is true 
    */
    virtual void disabled( double dt );

    /**
     * @brief return the current double value of the output property
     * @return the current value of the output property
     * If no output property is configured, a value of zero will be returned.
     * If more than one output property is configured, the value of the first output property
     * is returned. The current value of the output property will be clamped to the configured
     * values of &lt;min&gt; and/or &lt;max&gt;. 
     */
    inline double get_output_value() const {
      return _output_list.size() == 0 ? 0.0 : clamp(_output_list[0]->getDoubleValue());
    }

    simgear::PropertyList _output_list;
    SGPropertyNode_ptr _passive_mode;

    inline void set_output_value( double value ) {
        // passive_ignore == true means that we go through all the
        // motions, but drive the outputs.  This is analogous to
        // running the autopilot with the "servos" off.  This is
        // helpful for things like flight directors which position
        // their vbars from the autopilot computations.
        if ( _honor_passive && _passive_mode->getBoolValue() ) return;
        value = clamp( value );
        for( simgear::PropertyList::iterator it = _output_list.begin();
             it != _output_list.end(); ++it)
          (*it)->setDoubleValue( value );
    }

public:
    const PeriodicalValue * getPeriodicalValue() const { return _periodical; }
};

inline void AnalogComponent::disabled( double dt )
{
  if( _feedback_if_disabled && _output_list.size() > 0 ) {    
    InputValue * input;
    if( (input = _valueInput.get_active() ) != NULL )
      input->set_value( _output_list[0]->getDoubleValue() );
  }
}

}
#endif // ANALOGCOMPONENT_HXX
