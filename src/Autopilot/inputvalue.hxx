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

#ifndef _INPUTVALUE_HXX
#define _INPUTVALUE_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include <simgear/structure/SGExpression.hxx>

namespace FGXMLAutopilot {

typedef SGSharedPtr<class InputValue> InputValue_ptr;
typedef SGSharedPtr<class PeriodicalValue> PeriodicalValue_ptr;

/**
 * @brief Model a periodical value like angular values
 *
 * Most common use for periodical values are angular values.
 * If y = f(x) = f(x + n*period), this is a periodical function
 */
class PeriodicalValue : public SGReferenced {
private:
     InputValue_ptr minPeriod; // The minimum value of the period
     InputValue_ptr maxPeriod; // The maximum value of the period
public:
     PeriodicalValue( SGPropertyNode_ptr node );
     double normalize( double value ) const;
     double normalizeSymmetric( double value ) const;
};

/**
 * @brief A input value for analog autopilot components
 *
 * Input values may be constants, property values, transformed with a scale
 * and/or offset, clamped to min/max values, be periodical, bound to 
 * conditions or evaluated from expressions.
 */
class InputValue : public SGReferenced {
private:
     double             _value;    // The value as a constant or initializer for the property
     bool               _abs;      // return absolute value
     SGPropertyNode_ptr _property; // The name of the property containing the value
     InputValue_ptr _offset;   // A fixed offset, defaults to zero
     InputValue_ptr _scale;    // A constant scaling factor defaults to one
     InputValue_ptr _min;      // A minimum clip defaults to no clipping
     InputValue_ptr _max;      // A maximum clip defaults to no clipping
     PeriodicalValue_ptr  _periodical; //
     SGSharedPtr<const SGCondition> _condition;
     SGSharedPtr<SGExpressiond> _expression;  ///< expression to generate the value
     
public:
    InputValue( SGPropertyNode_ptr node = NULL, double value = 0.0, double offset = 0.0, double scale = 1.0 );
    
    void parse( SGPropertyNode_ptr, double value = 0.0, double offset = 0.0, double scale = 1.0 );

    /* get the value of this input, apply scale and offset and clipping */
    double get_value() const;

    /* set the input value after applying offset and scale */
    void set_value( double value );

    inline double get_scale() const {
      return _scale == NULL ? 1.0 : _scale->get_value();
    }

    inline double get_offset() const {
      return _offset == NULL ? 0.0 : _offset->get_value();
    }

    inline bool is_enabled() const {
      return _condition == NULL ? true : _condition->test();
    }

};

/**
 * @brief A chained list of InputValues
 *
 * Many compoments support InputValueLists as input. Each InputValue may be bound to
 * a condition. This list supports the get_value() function to retrieve the value
 * of the first InputValue in this list that has a condition evaluating to true.
 */
class InputValueList : public std::vector<InputValue_ptr> {
  public:
    InputValueList( double def = 0.0 ) : _def(def) { }

    InputValue_ptr get_active() const {
      for (const_iterator it = begin(); it != end(); ++it) {
        if( (*it)->is_enabled() )
          return *it;
      }
      return NULL;
    }

    double get_value() const {
      InputValue_ptr input = get_active();
      return input == NULL ? _def : input->get_value();
    }
  private:

    double _def;

};

}

#endif
