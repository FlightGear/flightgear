// inputvalue.hxx - provide input to autopilot components
//
// Written by Torsten Dreyer
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

#include <cstdlib>

#include "inputvalue.hxx"
#include <Main/fg_props.hxx>

using namespace FGXMLAutopilot;

PeriodicalValue::PeriodicalValue( SGPropertyNode_ptr root )
{
  SGPropertyNode_ptr minNode = root->getChild( "min" );
  SGPropertyNode_ptr maxNode = root->getChild( "max" );
  if( minNode == NULL || maxNode == NULL ) {
    SG_LOG(SG_AUTOPILOT, SG_ALERT, "periodical defined, but no <min> and/or <max> tag. Period ignored." );
  } else {
    minPeriod = new InputValue( minNode );
    maxPeriod = new InputValue( maxNode );
  }
}

double PeriodicalValue::normalize( double value ) const
{
  return SGMiscd::normalizePeriodic( minPeriod->get_value(), maxPeriod->get_value(), value );
}

double PeriodicalValue::normalizeSymmetric( double value ) const
{
  double minValue = minPeriod->get_value();
  double maxValue = maxPeriod->get_value();
  
  value = SGMiscd::normalizePeriodic( minValue, maxValue, value );
  double width_2 = (maxValue - minValue)/2;
  return value > width_2 ? width_2 - value : value;
}

InputValue::InputValue( SGPropertyNode_ptr node, double value, double offset, double scale) :
  _value(0.0),
  _abs(false)
{
  parse( node, value, offset, scale );
}


void InputValue::parse( SGPropertyNode_ptr node, double aValue, double aOffset, double aScale )
{
    _value = aValue;
    _property = NULL; 
    _offset = NULL;
    _scale = NULL;
    _min = NULL;
    _max = NULL;
    _periodical = NULL;

    if( node == NULL )
        return;

    SGPropertyNode * n;

    if( (n = node->getChild("condition")) != NULL ) {
        _condition = sgReadCondition(fgGetNode("/"), n);
    }

    if( (n = node->getChild( "scale" )) != NULL ) {
        _scale = new InputValue( n, aScale );
    }

    if( (n = node->getChild( "offset" )) != NULL ) {
        _offset = new InputValue( n, aOffset );
    }

    if( (n = node->getChild( "max" )) != NULL ) {
        _max = new InputValue( n );
    }

    if( (n = node->getChild( "min" )) != NULL ) {
        _min = new InputValue( n );
    }

    if( (n = node->getChild( "abs" )) != NULL ) {
      _abs = n->getBoolValue();
    }

    if( (n = node->getChild( "period" )) != NULL ) {
      _periodical = new PeriodicalValue( n );
    }

    SGPropertyNode *valueNode = node->getChild( "value" );
    if ( valueNode != NULL ) {
        _value = valueNode->getDoubleValue();
    }

    if ((n = node->getChild("expression")) != NULL) {
      _expression = SGReadDoubleExpression(fgGetNode("/"), n->getChild(0));
      return;
    }
    
    n = node->getChild( "property" );
    // if no <property> element, check for <prop> element for backwards
    // compatibility
    if(  n == NULL )
        n = node->getChild( "prop" );

    if (  n != NULL ) {
        _property = fgGetNode(  n->getStringValue(), true );
        if ( valueNode != NULL ) {
            // initialize property with given value 
            // if both <prop> and <value> exist
            double s = get_scale();
            if( s != 0 )
              _property->setDoubleValue( (_value - get_offset())/s );
            else
              _property->setDoubleValue( 0 ); // if scale is zero, value*scale is zero
        }
        
        return;
    } // of have a <property> or <prop>

    
    if (valueNode == NULL) {
        // no <value>, <prop> or <expression> element, use text node 
        const char * textnode = node->getStringValue();
        char * endp = NULL;
        // try to convert to a double value. If the textnode does not start with a number
        // endp will point to the beginning of the string. We assume this should be
        // a property name
        _value = strtod( textnode, &endp );
        if( endp == textnode ) {
          _property = fgGetNode( textnode, true );
        }
    }
}

void InputValue::set_value( double aValue ) 
{
    if (!_property)
      return;
      
    double s = get_scale();
    if( s != 0 )
        _property->setDoubleValue( (aValue - get_offset())/s );
    else
        _property->setDoubleValue( 0 ); // if scale is zero, value*scale is zero
}

double InputValue::get_value() const
{
    double value = _value;

    if (_expression) {
        // compute the expression value
        value = _expression->getValue(NULL);
    } else if( _property != NULL ) {
        value = _property->getDoubleValue();
    }
    
    if( _scale ) 
        value *= _scale->get_value();

    if( _offset ) 
        value += _offset->get_value();

    if( _min ) {
        double m = _min->get_value();
        if( value < m )
            value = m;
    }

    if( _max ) {
        double m = _max->get_value();
        if( value > m )
            value = m;
    }

    if( _periodical ) {
      value = _periodical->normalize( value );
    }
    
    return _abs ? fabs(value) : value;
}

