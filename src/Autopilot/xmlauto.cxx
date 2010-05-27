// xmlauto.cxx - a more flexible, generic way to build autopilots
//
// Written by Curtis Olson, started January 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>

#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/props/props_io.hxx>

#include <simgear/structure/SGExpression.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/util.hxx>

#include "xmlauto.hxx"

using std::cout;
using std::endl;

using simgear::PropertyList;




FGPeriodicalValue::FGPeriodicalValue( SGPropertyNode_ptr root )
{
  SGPropertyNode_ptr minNode = root->getChild( "min" );
  SGPropertyNode_ptr maxNode = root->getChild( "max" );
  if( minNode == NULL || maxNode == NULL ) {
    SG_LOG(SG_AUTOPILOT, SG_ALERT, "periodical defined, but no <min> and/or <max> tag. Period ignored." );
  } else {
    minPeriod = new FGXMLAutoInput( minNode );
    maxPeriod = new FGXMLAutoInput( maxNode );
  }
}

double FGPeriodicalValue::normalize( double value )
{
  if( !(minPeriod && maxPeriod )) return value;

  double p1 = minPeriod->get_value();
  double p2 = maxPeriod->get_value();

  double min = std::min<double>(p1,p2);
  double max = std::max<double>(p1,p2);
  double phase = fabs(max - min);

  if( phase > SGLimitsd::min() ) {
    while( value < min )  value += phase;
    while( value >= max ) value -= phase;
  } else {
    value = min; // phase is zero
  }

  return value;
}

FGXMLAutoInput::FGXMLAutoInput( SGPropertyNode_ptr node, double value, double offset, double scale) :
  value(0.0),
  abs(false)
{
  parse( node, value, offset, scale );
}


void FGXMLAutoInput::parse( SGPropertyNode_ptr node, double aValue, double aOffset, double aScale )
{
    value = aValue;
    property = NULL; 
    offset = NULL;
    scale = NULL;
    min = NULL;
    max = NULL;
    periodical = NULL;

    if( node == NULL )
        return;

    SGPropertyNode * n;

    if( (n = node->getChild("condition")) != NULL ) {
        _condition = sgReadCondition(fgGetNode("/"), n);
    }

    if( (n = node->getChild( "scale" )) != NULL ) {
        scale = new FGXMLAutoInput( n, aScale );
    }

    if( (n = node->getChild( "offset" )) != NULL ) {
        offset = new FGXMLAutoInput( n, aOffset );
    }

    if( (n = node->getChild( "max" )) != NULL ) {
        max = new FGXMLAutoInput( n );
    }

    if( (n = node->getChild( "min" )) != NULL ) {
        min = new FGXMLAutoInput( n );
    }

    if( (n = node->getChild( "abs" )) != NULL ) {
      abs = n->getBoolValue();
    }

    if( (n = node->getChild( "period" )) != NULL ) {
      periodical = new FGPeriodicalValue( n );
    }

    SGPropertyNode *valueNode = node->getChild( "value" );
    if ( valueNode != NULL ) {
        value = valueNode->getDoubleValue();
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
        property = fgGetNode(  n->getStringValue(), true );
        if ( valueNode != NULL ) {
            // initialize property with given value 
            // if both <prop> and <value> exist
            double s = get_scale();
            if( s != 0 )
              property->setDoubleValue( (value - get_offset())/s );
            else
              property->setDoubleValue( 0 ); // if scale is zero, value*scale is zero
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
        value = strtod( textnode, &endp );
        if( endp == textnode ) {
          property = fgGetNode( textnode, true );
        }
    }
}

void FGXMLAutoInput::set_value( double aValue ) 
{
    if (!property)
      return;
      
    double s = get_scale();
    if( s != 0 )
        property->setDoubleValue( (aValue - get_offset())/s );
    else
        property->setDoubleValue( 0 ); // if scale is zero, value*scale is zero
}

double FGXMLAutoInput::get_value() 
{
    if (_expression) {
        // compute the expression value
        value = _expression->getValue(NULL);
    } else if( property != NULL ) {
        value = property->getDoubleValue();
    }
    
    if( scale ) 
        value *= scale->get_value();

    if( offset ) 
        value += offset->get_value();

    if( min ) {
        double m = min->get_value();
        if( value < m )
            value = m;
    }

    if( max ) {
        double m = max->get_value();
        if( value > m )
            value = m;
    }

    if( periodical ) {
      value = periodical->normalize( value );
    }
    
    return abs ? fabs(value) : value;
}

FGXMLAutoComponent::FGXMLAutoComponent() :
      _condition( NULL ),
      enable_prop( NULL ),
      enable_value( NULL ),
      passive_mode( fgGetNode("/autopilot/locks/passive-mode", true) ),
      honor_passive( false ),
      name(""),
      feedback_if_disabled( false ),
      debug(false),
      enabled( false )
{
}

FGXMLAutoComponent::~FGXMLAutoComponent() 
{
    delete enable_value;
}

void FGXMLAutoComponent::parseNode(SGPropertyNode* aNode)
{
  SGPropertyNode *prop; 
  for (int i = 0; i < aNode->nChildren(); ++i ) {
    SGPropertyNode *child = aNode->getChild(i);
    string cname(child->getName());
    
    if (parseNodeHook(cname, child)) {
      // derived class handled it, fine
    } else if ( cname == "name" ) {
      name = child->getStringValue();
    } else if ( cname == "feedback-if-disabled" ) {
      feedback_if_disabled = child->getBoolValue();
    } else if ( cname == "debug" ) {
      debug = child->getBoolValue();
    } else if ( cname == "enable" ) {
      if( (prop = child->getChild("condition")) != NULL ) {
        _condition = sgReadCondition(fgGetNode("/"), prop);
      } else {
         if ( (prop = child->getChild( "property" )) != NULL ) {
             enable_prop = fgGetNode( prop->getStringValue(), true );
         }
         
         if ( (prop = child->getChild( "prop" )) != NULL ) {
             enable_prop = fgGetNode( prop->getStringValue(), true );
         }

         if ( (prop = child->getChild( "value" )) != NULL ) {
             delete enable_value;
             enable_value = new string(prop->getStringValue());
         }
      }
      if ( (prop = child->getChild( "honor-passive" )) != NULL ) {
          honor_passive = prop->getBoolValue();
      }
    } else if ( cname == "input" ) {
      valueInput.push_back( new FGXMLAutoInput( child ) );
    } else if ( cname == "reference" ) {
      referenceInput.push_back( new FGXMLAutoInput( child ) );
    } else if ( cname == "output" ) {
      // grab all <prop> and <property> childs
      int found = 0;
      // backwards compatibility: allow <prop> elements
      for( int i = 0; (prop = child->getChild("prop", i)) != NULL; i++ ) { 
          SGPropertyNode *tmp = fgGetNode( prop->getStringValue(), true );
          output_list.push_back( tmp );
          found++;
      }
      for( int i = 0; (prop = child->getChild("property", i)) != NULL; i++ ) { 
          SGPropertyNode *tmp = fgGetNode( prop->getStringValue(), true );
          output_list.push_back( tmp );
          found++;
      }

      // no <prop> elements, text node of <output> is property name
      if( found == 0 )
          output_list.push_back( fgGetNode(child->getStringValue(), true ) );
    } else if ( cname == "config" ) {
      parseConfig(child);
    } else if ( cname == "min" ) {
        uminInput.push_back( new FGXMLAutoInput( child ) );
    } else if ( cname == "u_min" ) {
        uminInput.push_back( new FGXMLAutoInput( child ) );
    } else if ( cname == "max" ) {
        umaxInput.push_back( new FGXMLAutoInput( child ) );
    } else if ( cname == "u_max" ) {
        umaxInput.push_back( new FGXMLAutoInput( child ) );
    } else if ( cname == "period" ) {
      periodical = new FGPeriodicalValue( child );
    } else {
      SG_LOG(SG_AUTOPILOT, SG_ALERT, "malformed autopilot definition - unrecognized node:" 
        << cname << " in section " << name);
      throw sg_io_exception("XMLAuto: unrecognized component node:" + cname, "Section=" + name);
    }
  } // of top-level iteration
}

void FGXMLAutoComponent::parseConfig(SGPropertyNode* aConfig)
{
  for (int i = 0; i < aConfig->nChildren(); ++i ) {
    SGPropertyNode *child = aConfig->getChild(i);
    string cname(child->getName());
    
    if (parseConfigHook(cname, child)) {
      // derived class handled it, fine
    } else if ( cname == "min" ) {
        uminInput.push_back( new FGXMLAutoInput( child ) );
    } else if ( cname == "u_min" ) {
        uminInput.push_back( new FGXMLAutoInput( child ) );
    } else if ( cname == "max" ) {
        umaxInput.push_back( new FGXMLAutoInput( child ) );
    } else if ( cname == "u_max" ) {
        umaxInput.push_back( new FGXMLAutoInput( child ) );
    } else {
      SG_LOG(SG_AUTOPILOT, SG_ALERT, "malformed autopilot definition - unrecognized config node:" 
        << cname << " in section " << name);
      throw sg_io_exception("XMLAuto: unrecognized config node:" + cname, "Section=" + name);
    }
  } // of config iteration
}

bool FGXMLAutoComponent::parseNodeHook(const string& aName, SGPropertyNode* aNode)
{
  return false;
}

bool FGXMLAutoComponent::parseConfigHook(const string& aName, SGPropertyNode* aNode)
{
  return false;
}

bool FGXMLAutoComponent::isPropertyEnabled()
{
    if( _condition )
        return _condition->test();

    if( enable_prop ) {
        if( enable_value ) {
            return *enable_value == enable_prop->getStringValue();
        } else {
            return enable_prop->getBoolValue();
        }
    }
    return true;
}

void FGXMLAutoComponent::do_feedback_if_disabled()
{
    if( output_list.size() > 0 ) {    
        FGXMLAutoInput * input = valueInput.get_active();
        if( input != NULL )
            input->set_value( output_list[0]->getDoubleValue() );
    }
}

double FGXMLAutoComponent::clamp( double value )
{
    //If this is a periodical value, normalize it into our domain 
    // before clamping
    if( periodical )
      value = periodical->normalize( value );

    // clamp, if either min or max is defined
    if( uminInput.size() + umaxInput.size() > 0 ) {
        double d = umaxInput.get_value( 0.0 );
        if( value > d ) value = d;
        d = uminInput.get_value( 0.0 );
        if( value < d ) value = d;
    }
    return value;
}

FGPIDController::FGPIDController( SGPropertyNode *node ):
    FGXMLAutoComponent(),
    alpha( 0.1 ),
    beta( 1.0 ),
    gamma( 0.0 ),
    ep_n_1( 0.0 ),
    edf_n_1( 0.0 ),
    edf_n_2( 0.0 ),
    u_n_1( 0.0 ),
    desiredTs( 0.0 ),
    elapsedTime( 0.0 )
{
  parseNode(node);
}

bool FGPIDController::parseConfigHook(const string& aName, SGPropertyNode* aNode)
{
  if (aName == "Ts") {
    desiredTs = aNode->getDoubleValue();
  } else if (aName == "Kp") {
    Kp.push_back( new FGXMLAutoInput(aNode) );
  } else if (aName == "Ti") {
    Ti.push_back( new FGXMLAutoInput(aNode) );
  } else if (aName == "Td") {
    Td.push_back( new FGXMLAutoInput(aNode) );
  } else if (aName == "beta") {
    beta = aNode->getDoubleValue();
  } else if (aName == "alpha") {
    alpha = aNode->getDoubleValue();
  } else if (aName == "gamma") {
    gamma = aNode->getDoubleValue();
  } else {
    // unhandled by us, let the base class try it
    return false;
  }

  return true;
}

/*
 * Roy Vegard Ovesen:
 *
 * Ok! Here is the PID controller algorithm that I would like to see
 * implemented:
 *
 *   delta_u_n = Kp * [ (ep_n - ep_n-1) + ((Ts/Ti)*e_n)
 *               + (Td/Ts)*(edf_n - 2*edf_n-1 + edf_n-2) ]
 *
 *   u_n = u_n-1 + delta_u_n
 *
 * where:
 *
 * delta_u : The incremental output
 * Kp      : Proportional gain
 * ep      : Proportional error with reference weighing
 *           ep = beta * r - y
 *           where:
 *           beta : Weighing factor
 *           r    : Reference (setpoint)
 *           y    : Process value, measured
 * e       : Error
 *           e = r - y
 * Ts      : Sampling interval
 * Ti      : Integrator time
 * Td      : Derivator time
 * edf     : Derivate error with reference weighing and filtering
 *           edf_n = edf_n-1 / ((Ts/Tf) + 1) + ed_n * (Ts/Tf) / ((Ts/Tf) + 1)
 *           where:
 *           Tf : Filter time
 *           Tf = alpha * Td , where alpha usually is set to 0.1
 *           ed : Unfiltered derivate error with reference weighing
 *             ed = gamma * r - y
 *             where:
 *             gamma : Weighing factor
 * 
 * u       : absolute output
 * 
 * Index n means the n'th value.
 * 
 * 
 * Inputs:
 * enabled ,
 * y_n , r_n , beta=1 , gamma=0 , alpha=0.1 ,
 * Kp , Ti , Td , Ts (is the sampling time available?)
 * u_min , u_max
 * 
 * Output:
 * u_n
 */

void FGPIDController::update( double dt ) {
    double e_n;             // error
    double edf_n;
    double delta_u_n = 0.0; // incremental output
    double u_n = 0.0;       // absolute output
    double Ts;              // sampling interval (sec)

    double u_min = uminInput.get_value();
    double u_max = umaxInput.get_value();

    elapsedTime += dt;
    if ( elapsedTime <= desiredTs ) {
        // do nothing if time step is not positive (i.e. no time has
        // elapsed)
        return;
    }
    Ts = elapsedTime;
    elapsedTime = 0.0;

    if ( isPropertyEnabled() ) {
        if ( !enabled ) {
            // first time being enabled, seed u_n with current
            // property tree value
            u_n = get_output_value();
            u_n_1 = u_n;
        }
        enabled = true;
    } else {
        enabled = false;
        do_feedback();
    }

    if ( enabled && Ts > 0.0) {
        if ( debug ) cout << "Updating " << get_name()
                          << " Ts " << Ts << endl;

        double y_n = valueInput.get_value();
        double r_n = referenceInput.get_value();
                      
        if ( debug ) cout << "  input = " << y_n << " ref = " << r_n << endl;

        // Calculates proportional error:
        double ep_n = beta * r_n - y_n;
        if ( debug ) cout << "  ep_n = " << ep_n;
        if ( debug ) cout << "  ep_n_1 = " << ep_n_1;

        // Calculates error:
        e_n = r_n - y_n;
        if ( debug ) cout << " e_n = " << e_n;

        double td = Td.get_value();
        if ( td > 0.0 ) { // do we need to calcluate derivative error?

          // Calculates derivate error:
            double ed_n = gamma * r_n - y_n;
            if ( debug ) cout << " ed_n = " << ed_n;

            // Calculates filter time:
            double Tf = alpha * td;
            if ( debug ) cout << " Tf = " << Tf;

            // Filters the derivate error:
            edf_n = edf_n_1 / (Ts/Tf + 1)
                + ed_n * (Ts/Tf) / (Ts/Tf + 1);
            if ( debug ) cout << " edf_n = " << edf_n;
        } else {
            edf_n_2 = edf_n_1 = edf_n = 0.0;
        }

        // Calculates the incremental output:
        double ti = Ti.get_value();
        if ( ti > 0.0 ) {
            delta_u_n = Kp.get_value() * ( (ep_n - ep_n_1)
                               + ((Ts/ti) * e_n)
                               + ((td/Ts) * (edf_n - 2*edf_n_1 + edf_n_2)) );

          if ( debug ) {
              cout << " delta_u_n = " << delta_u_n << endl;
              cout << "P:" << Kp.get_value() * (ep_n - ep_n_1)
                   << " I:" << Kp.get_value() * ((Ts/ti) * e_n)
                   << " D:" << Kp.get_value() * ((td/Ts) * (edf_n - 2*edf_n_1 + edf_n_2))
                   << endl;
        }
        }

        // Integrator anti-windup logic:
        if ( delta_u_n > (u_max - u_n_1) ) {
            delta_u_n = u_max - u_n_1;
            if ( debug ) cout << " max saturation " << endl;
        } else if ( delta_u_n < (u_min - u_n_1) ) {
            delta_u_n = u_min - u_n_1;
            if ( debug ) cout << " min saturation " << endl;
        }

        // Calculates absolute output:
        u_n = u_n_1 + delta_u_n;
        if ( debug ) cout << "  output = " << u_n << endl;

        // Updates indexed values;
        u_n_1   = u_n;
        ep_n_1  = ep_n;
        edf_n_2 = edf_n_1;
        edf_n_1 = edf_n;

        set_output_value( u_n );
    } else if ( !enabled ) {
        ep_n_1 = 0.0;
        edf_n_2 = edf_n_1 = edf_n = 0.0;
    }
}


FGPISimpleController::FGPISimpleController( SGPropertyNode *node ):
    FGXMLAutoComponent(),
    int_sum( 0.0 )
{
  parseNode(node);
}

bool FGPISimpleController::parseConfigHook(const string& aName, SGPropertyNode* aNode)
{
  if (aName == "Kp") {
    Kp.push_back( new FGXMLAutoInput(aNode) );
  } else if (aName == "Ki") {
    Ki.push_back( new FGXMLAutoInput(aNode) );
  } else {
    // unhandled by us, let the base class try it
    return false;
  }

  return true;
}

void FGPISimpleController::update( double dt ) {

    if ( isPropertyEnabled() ) {
        if ( !enabled ) {
            // we have just been enabled, zero out int_sum
            int_sum = 0.0;
        }
        enabled = true;
    } else {
        enabled = false;
        do_feedback();
    }

    if ( enabled ) {
        if ( debug ) cout << "Updating " << get_name() << endl;
        double y_n = valueInput.get_value();
        double r_n = referenceInput.get_value();
                      
        double error = r_n - y_n;
        if ( debug ) cout << "input = " << y_n
                          << " reference = " << r_n
                          << " error = " << error
                          << endl;

        double prop_comp = clamp(error * Kp.get_value());
        int_sum += error * Ki.get_value() * dt;


        double output = prop_comp + int_sum;
        double clamped_output = clamp( output );
        if( output != clamped_output ) // anti-windup
          int_sum = clamped_output - prop_comp;

        if ( debug ) cout << "prop_comp = " << prop_comp
                          << " int_sum = " << int_sum << endl;

        set_output_value( clamped_output );
        if ( debug ) cout << "output = " << clamped_output << endl;
    }
}


FGPredictor::FGPredictor ( SGPropertyNode *node ):
    FGXMLAutoComponent(),
    average(0.0)
{
  parseNode(node);
}

bool FGPredictor::parseNodeHook(const string& aName, SGPropertyNode* aNode)
{
  if (aName == "seconds") {
    seconds.push_back( new FGXMLAutoInput( aNode, 0 ) );
  } else if (aName == "filter-gain") {
    filter_gain.push_back( new FGXMLAutoInput( aNode, 0 ) );
  } else {
    return false;
  }
  
  return true;
}

void FGPredictor::update( double dt ) {
    /*
       Simple moving average filter converts input value to predicted value "seconds".

       Smoothing as described by Curt Olson:
         gain would be valid in the range of 0 - 1.0
         1.0 would mean no filtering.
         0.0 would mean no input.
         0.5 would mean (1 part past value + 1 part current value) / 2
         0.1 would mean (9 parts past value + 1 part current value) / 10
         0.25 would mean (3 parts past value + 1 part current value) / 4

    */

    double ivalue = valueInput.get_value();

    if ( isPropertyEnabled() ) {
        if ( !enabled ) {
            // first time being enabled
            last_value = ivalue;
        }
        enabled = true;
    } else {
        enabled = false;
        do_feedback();
    }

    if ( enabled ) {

        if ( dt > 0.0 ) {
            double current = (ivalue - last_value)/dt; // calculate current error change (per second)
            average = dt < 1.0 ? ((1.0 - dt) * average + current * dt) : current;

            // calculate output with filter gain adjustment
            double output = ivalue + 
               (1.0 - filter_gain.get_value()) * (average * seconds.get_value()) + 
                       filter_gain.get_value() * (current * seconds.get_value());
            output = clamp( output );
            set_output_value( output );
        }
        last_value = ivalue;
    }
}


FGDigitalFilter::FGDigitalFilter(SGPropertyNode *node):
    FGXMLAutoComponent(),
    filterType(none)
{
    parseNode(node);

    output.resize(2, 0.0);
    input.resize(samplesInput.get_value() + 1, 0.0);
}


bool FGDigitalFilter::parseNodeHook(const string& aName, SGPropertyNode* aNode)
{
  if (aName == "type" ) {
    string val(aNode->getStringValue());
    if ( val == "exponential" ) {
      filterType = exponential;
    } else if (val == "double-exponential") {
      filterType = doubleExponential;
    } else if (val == "moving-average") {
      filterType = movingAverage;
    } else if (val == "noise-spike") {
      filterType = noiseSpike;
    } else if (val == "gain") {
      filterType = gain;
    } else if (val == "reciprocal") {
      filterType = reciprocal;
    } else if (val == "differential") {
      filterType = differential;
      // use a constant of two samples for current and previous input value
      samplesInput.push_back( new FGXMLAutoInput(NULL, 2.0 ) ); 
    }
  } else if (aName == "filter-time" ) {
    TfInput.push_back( new FGXMLAutoInput( aNode, 1.0 ) );
    if( filterType == none ) filterType = exponential;
  } else if (aName == "samples" ) {
    samplesInput.push_back( new FGXMLAutoInput( aNode, 1 ) );
    if( filterType == none ) filterType = movingAverage;
  } else if (aName == "max-rate-of-change" ) {
    rateOfChangeInput.push_back( new FGXMLAutoInput( aNode, 1 ) );
    if( filterType == none ) filterType = noiseSpike;
  } else if (aName == "gain" ) {
    gainInput.push_back( new FGXMLAutoInput( aNode, 1 ) );
    if( filterType == none ) filterType = gain;
  } else {
    return false; // not handled by us, let the base class try
  }
  
  return true;
}

void FGDigitalFilter::update(double dt)
{
    if ( isPropertyEnabled() ) {

        input.push_front(valueInput.get_value()-referenceInput.get_value());
        input.resize(samplesInput.get_value() + 1, 0.0);

        if ( !enabled ) {
            // first time being enabled, initialize output to the
            // value of the output property to avoid bumping.
            output.push_front(get_output_value());
        }

        enabled = true;
    } else {
        enabled = false;
        do_feedback();
    }

    if ( !enabled || dt < SGLimitsd::min() ) 
        return;

    /*
     * Exponential filter
     *
     * Output[n] = alpha*Input[n] + (1-alpha)*Output[n-1]
     *
     */
     if( debug ) cout << "Updating " << get_name()
                      << " dt " << dt << endl;

    if (filterType == exponential)
    {
        double alpha = 1 / ((TfInput.get_value()/dt) + 1);
        output.push_front(alpha * input[0] + 
                          (1 - alpha) * output[0]);
    } 
    else if (filterType == doubleExponential)
    {
        double alpha = 1 / ((TfInput.get_value()/dt) + 1);
        output.push_front(alpha * alpha * input[0] + 
                          2 * (1 - alpha) * output[0] -
                          (1 - alpha) * (1 - alpha) * output[1]);
    }
    else if (filterType == movingAverage)
    {
        output.push_front(output[0] + 
                          (input[0] - input.back()) / samplesInput.get_value());
    }
    else if (filterType == noiseSpike)
    {
        double maxChange = rateOfChangeInput.get_value() * dt;

        if ((output[0] - input[0]) > maxChange)
        {
            output.push_front(output[0] - maxChange);
        }
        else if ((output[0] - input[0]) < -maxChange)
        {
            output.push_front(output[0] + maxChange);
        }
        else if (fabs(input[0] - output[0]) <= maxChange)
        {
            output.push_front(input[0]);
        }
    }
    else if (filterType == gain)
    {
        output[0] = gainInput.get_value() * input[0];
    }
    else if (filterType == reciprocal)
    {
        if (input[0] != 0.0) {
            output[0] = gainInput.get_value() / input[0];
        }
    }
    else if (filterType == differential)
    {
        if( dt > SGLimitsd::min() ) {
            output[0] = (input[0]-input[1]) * TfInput.get_value() / dt;
        }
    }

    output[0] = clamp(output[0]) ;
    set_output_value( output[0] );

    output.resize(2);

    if (debug)
    {
        cout << "input:" << input[0] 
             << "\toutput:" << output[0] << endl;
    }
}

FGXMLAutoLogic::FGXMLAutoLogic(SGPropertyNode * node ) :
    FGXMLAutoComponent(),
    inverted(false)
{
    parseNode(node);
}

bool FGXMLAutoLogic::parseNodeHook(const std::string& aName, SGPropertyNode* aNode)
{
    if (aName == "input") {
        input = sgReadCondition( fgGetNode("/"), aNode );
    } else if (aName == "inverted") {
        inverted = aNode->getBoolValue();
    } else {
        return false;
    }
  
    return true;
}

void FGXMLAutoLogic::update(double dt)
{
    if ( isPropertyEnabled() ) {
        if ( !enabled ) {
            // we have just been enabled
        }
        enabled = true;
    } else {
        enabled = false;
        do_feedback();
    }

    if ( !enabled || dt < SGLimitsd::min() ) 
        return;

    if( input == NULL ) {
        if ( debug ) cout << "No input for " << get_name() << endl;
        return;
    }

    bool i = input->test();

    if ( debug ) cout << "Updating " << get_name() << ": " << (inverted ? !i : i) << endl;

    set_output_value( i );
}

class FGXMLAutoRSFlipFlop : public FGXMLAutoFlipFlop {
private:
  bool _rs;
public:
  FGXMLAutoRSFlipFlop( SGPropertyNode * node ) :
    FGXMLAutoFlipFlop( node ) {
      // type exists here, otherwise we were not constructed
      string val = node->getNode( "type" )->getStringValue();
      _rs = (val == "RS");
    }

  void updateState( double dt ) {

    if( sInput == NULL ) {
        if ( debug ) cout << "No set (S) input for " << get_name() << endl;
        return;
    }

    if( rInput == NULL ) {
        if ( debug ) cout << "No reset (R) input for " << get_name() << endl;
        return;
    }

    bool s = sInput->test();
    bool r = rInput->test();

    // s == false && q == false: no change, keep state
    if( s || r ) {
      bool q = false;
      if( _rs ) { // RS: reset is dominant
        if( s ) q = true; // set
        if( r ) q = false; // reset
      } else { // SR: set is dominant
        if( r ) q = false; // reset
        if( s ) q = true; // set
      }
      if( inverted ) q = !q;

      if ( debug ) cout << "Updating " << get_name() << ":" 
        << " s=" << s
        << ",r=" << r
        << ",q=" << q << endl;
      set_output_value( q );
    } else {
      if ( debug ) cout << "Updating " << get_name() << ":" 
        << " s=" << s
        << ",r=" << r
        << ",q=unchanged" << endl;
    }
  }
};

class FGXMLAutoJKFlipFlop : public FGXMLAutoFlipFlop {
private: 
  bool clock;
public:
  FGXMLAutoJKFlipFlop( SGPropertyNode * node ) :
    FGXMLAutoFlipFlop( node ), 
    clock(false) {}

  void updateState( double dt ) {

    if( jInput == NULL ) {
        if ( debug ) cout << "No set (j) input for " << get_name() << endl;
        return;
    }

    if( kInput == NULL ) {
        if ( debug ) cout << "No reset (k) input for " << get_name() << endl;
        return;
    }

    bool j = jInput->test();
    bool k = kInput->test();
    /*
      if the user provided a clock input, use it. 
      Otherwise use framerate as clock
      This JK operates on the raising edge. 
    */
    bool c = clockInput ? clockInput->test() : false;
    bool raisingEdge = clockInput ? (c && !clock) : true;
    clock = c;

    if( !raisingEdge ) return;
    
    bool q = get_bool_output_value();
    // j == false && k == false: no change, keep state
    if( (j || k) ) {
      if( j && k ) {
        q = !q; // toggle
      } else {
        if( j ) q = true; // set
        if( k ) q = false; // reset
      }
      if( inverted ) q = !q;

      if ( debug ) cout << "Updating " << get_name() << ":" 
        << " j=" << j
        << ",k=" << k
        << ",q=" << q << endl;
      set_output_value( q );
    } else {
      if ( debug ) cout << "Updating " << get_name() << ":" 
        << " j=" << j
        << ",k=" << k
        << ",q=unchanged" << endl;
    }
  }
};

class FGXMLAutoDFlipFlop : public FGXMLAutoFlipFlop {
private: 
  bool clock;
public:
  FGXMLAutoDFlipFlop( SGPropertyNode * node ) :
    FGXMLAutoFlipFlop( node ), 
    clock(false) {}

  void updateState( double dt ) {

    if( clockInput == NULL ) {
        if ( debug ) cout << "No (clock) input for " << get_name() << endl;
        return;
    }

    if( dInput == NULL ) {
        if ( debug ) cout << "No (D) input for " << get_name() << endl;
        return;
    }

    bool d = dInput->test();

    // check the clock - raising edge
    bool c = clockInput->test();
    bool raisingEdge = c && !clock;
    clock = c;

    if( raisingEdge ) {
      bool q = d;
      if( inverted ) q = !q;

      if ( debug ) cout << "Updating " << get_name() << ":" 
        << " d=" << d
        << ",q=" << q << endl;
      set_output_value( q );
    } else {
      if ( debug ) cout << "Updating " << get_name() << ":" 
        << " d=" << d
        << ",q=unchanged" << endl;
    }
  }
};

class FGXMLAutoTFlipFlop : public FGXMLAutoFlipFlop {
private: 
  bool clock;
public:
  FGXMLAutoTFlipFlop( SGPropertyNode * node ) :
    FGXMLAutoFlipFlop( node ), 
    clock(false) {}

  void updateState( double dt ) {

    if( clockInput == NULL ) {
        if ( debug ) cout << "No (clock) input for " << get_name() << endl;
        return;
    }

    // check the clock - raising edge
    bool c = clockInput->test();
    bool raisingEdge = c && !clock;
    clock = c;

    if( raisingEdge ) {
      bool q = !get_bool_output_value(); // toggle
      if( inverted ) q = !q; // doesnt really make sense for a T-FF

      if ( debug ) cout << "Updating " << get_name() << ":" 
        << ",q=" << q << endl;
      set_output_value( q );
    } else {
      if ( debug ) cout << "Updating " << get_name() << ":" 
        << ",q=unchanged" << endl;
    }
  }
};

FGXMLAutoFlipFlop::FGXMLAutoFlipFlop(SGPropertyNode * node ) :
    FGXMLAutoComponent(),
    inverted(false)
{
    parseNode(node);
}

bool FGXMLAutoFlipFlop::parseNodeHook(const std::string& aName, SGPropertyNode* aNode)
{
    if (aName == "set"||aName == "S") {
        sInput = sgReadCondition( fgGetNode("/"), aNode );
    } else if (aName == "reset" || aName == "R" ) {
        rInput = sgReadCondition( fgGetNode("/"), aNode );
    } else if (aName == "J") {
        jInput = sgReadCondition( fgGetNode("/"), aNode );
    } else if (aName == "K") {
        kInput = sgReadCondition( fgGetNode("/"), aNode );
    } else if (aName == "T") {
        tInput = sgReadCondition( fgGetNode("/"), aNode );
    } else if (aName == "D") {
        dInput = sgReadCondition( fgGetNode("/"), aNode );
    } else if (aName == "clock") {
        clockInput = sgReadCondition( fgGetNode("/"), aNode );
    } else if (aName == "inverted") {
        inverted = aNode->getBoolValue();
    } else if (aName == "type") {
        // ignore element type, evaluated by loader
    } else {
        return false;
    }
  
    return true;
}

void FGXMLAutoFlipFlop::update(double dt)
{
    if ( isPropertyEnabled() ) {
        if ( !enabled ) {
            // we have just been enabled
            // initialize to a bool property
            set_output_value( get_bool_output_value() );
        }
        enabled = true;
    } else {
        enabled = false;
        do_feedback();
    }

    if ( !enabled || dt < SGLimitsd::min() ) 
        return;

    updateState( dt );
}


FGXMLAutopilotGroup::FGXMLAutopilotGroup() :
  SGSubsystemGroup()
#ifdef XMLAUTO_USEHELPER
  ,average(0.0), // average/filtered prediction
  v_last(0.0),  // last velocity
  last_static_pressure(0.0),
  vel(fgGetNode( "/velocities/airspeed-kt", true )),
  // Estimate speed in 5,10 seconds
  lookahead5(fgGetNode( "/autopilot/internal/lookahead-5-sec-airspeed-kt", true )),
  lookahead10(fgGetNode( "/autopilot/internal/lookahead-10-sec-airspeed-kt", true )),
  bug(fgGetNode( "/autopilot/settings/heading-bug-deg", true )),
  mag_hdg(fgGetNode( "/orientation/heading-magnetic-deg", true )),
  bug_error(fgGetNode( "/autopilot/internal/heading-bug-error-deg", true )),
  fdm_bug_error(fgGetNode( "/autopilot/internal/fdm-heading-bug-error-deg", true )),
  target_true(fgGetNode( "/autopilot/settings/true-heading-deg", true )),
  true_hdg(fgGetNode( "/orientation/heading-deg", true )),
  true_error(fgGetNode( "/autopilot/internal/true-heading-error-deg", true )),
  target_nav1(fgGetNode( "/instrumentation/nav[0]/radials/target-auto-hdg-deg", true )),
  true_nav1(fgGetNode( "/autopilot/internal/nav1-heading-error-deg", true )),
  true_track_nav1(fgGetNode( "/autopilot/internal/nav1-track-error-deg", true )),
  nav1_course_error(fgGetNode( "/autopilot/internal/nav1-course-error", true )),
  nav1_selected_course(fgGetNode( "/instrumentation/nav[0]/radials/selected-deg", true )),
  vs_fps(fgGetNode( "/velocities/vertical-speed-fps", true )),
  vs_fpm(fgGetNode( "/autopilot/internal/vert-speed-fpm", true )),
  static_pressure(fgGetNode( "/systems/static[0]/pressure-inhg", true )),
  pressure_rate(fgGetNode( "/autopilot/internal/pressure-rate", true )),
  track(fgGetNode( "/orientation/track-deg", true ))
#endif
{
}

void FGXMLAutopilotGroup::update( double dt )
{
    // update all configured autopilots
    SGSubsystemGroup::update( dt );
#ifdef XMLAUTO_USEHELPER
    // update helper values
    double v = vel->getDoubleValue();
    double a = 0.0;
    if ( dt > 0.0 ) {
        a = (v - v_last) / dt;

        if ( dt < 1.0 ) {
            average = (1.0 - dt) * average + dt * a;
        } else {
            average = a;
        }

        lookahead5->setDoubleValue( v + average * 5.0 );
        lookahead10->setDoubleValue( v + average * 10.0 );
        v_last = v;
    }

    // Calculate heading bug error normalized to +/- 180.0
    double diff = bug->getDoubleValue() - mag_hdg->getDoubleValue();
    SG_NORMALIZE_RANGE(diff, -180.0, 180.0);
    bug_error->setDoubleValue( diff );

    fdm_bug_error->setDoubleValue( diff );

    // Calculate true heading error normalized to +/- 180.0
    diff = target_true->getDoubleValue() - true_hdg->getDoubleValue();
    SG_NORMALIZE_RANGE(diff, -180.0, 180.0);
    true_error->setDoubleValue( diff );

    // Calculate nav1 target heading error normalized to +/- 180.0
    diff = target_nav1->getDoubleValue() - true_hdg->getDoubleValue();
    SG_NORMALIZE_RANGE(diff, -180.0, 180.0);
    true_nav1->setDoubleValue( diff );

    // Calculate true groundtrack
    diff = target_nav1->getDoubleValue() - track->getDoubleValue();
    SG_NORMALIZE_RANGE(diff, -180.0, 180.0);
    true_track_nav1->setDoubleValue( diff );

    // Calculate nav1 selected course error normalized to +/- 180.0
    diff = nav1_selected_course->getDoubleValue() - mag_hdg->getDoubleValue();
    SG_NORMALIZE_RANGE( diff, -180.0, 180.0 );
    nav1_course_error->setDoubleValue( diff );

    // Calculate vertical speed in fpm
    vs_fpm->setDoubleValue( vs_fps->getDoubleValue() * 60.0 );


    // Calculate static port pressure rate in [inhg/s].
    // Used to determine vertical speed.
    if ( dt > 0.0 ) {
        double current_static_pressure = static_pressure->getDoubleValue();
        double current_pressure_rate = 
            ( current_static_pressure - last_static_pressure ) / dt;

        pressure_rate->setDoubleValue(current_pressure_rate);
        last_static_pressure = current_static_pressure;
    }
#endif
}

void FGXMLAutopilotGroup::reinit()
{
    for( vector<string>::size_type i = 0; i < _autopilotNames.size(); i++ ) {
      FGXMLAutopilot * ap = (FGXMLAutopilot*)get_subsystem( _autopilotNames[i] );
      if( ap == NULL ) continue; // ?
      remove_subsystem( _autopilotNames[i] );
      delete ap;
    }
    _autopilotNames.clear();
    init();
}

void FGXMLAutopilotGroup::init()
{
    PropertyList autopilotNodes = fgGetNode( "/sim/systems", true )->getChildren("autopilot");
    if( autopilotNodes.size() == 0 ) {
        SG_LOG( SG_ALL, SG_WARN, "No autopilot configuration specified for this model!");
        return;
    }

    for( PropertyList::size_type i = 0; i < autopilotNodes.size(); i++ ) {
        SGPropertyNode_ptr pathNode = autopilotNodes[i]->getNode( "path" );
        if( pathNode == NULL ) {
            SG_LOG( SG_ALL, SG_WARN, "No autopilot configuration file specified for this autopilot!");
            continue;
        }

        string apName;
        SGPropertyNode_ptr nameNode = autopilotNodes[i]->getNode( "name" );
        if( nameNode != NULL ) {
            apName = nameNode->getStringValue();
        } else {
          std::ostringstream buf;
          buf <<  "unnamed_autopilot_" << i;
          apName = buf.str();
        }

        if( get_subsystem( apName.c_str() ) != NULL ) {
            SG_LOG( SG_ALL, SG_ALERT, "Duplicate autopilot configuration name " << apName << " ignored" );
            continue;
        }

        SGPath config( globals->get_fg_root() );
        config.append( pathNode->getStringValue() );

        SG_LOG( SG_ALL, SG_INFO, "Reading autopilot configuration from " << config.str() );
        // FGXMLAutopilot
        FGXMLAutopilot * ap = new FGXMLAutopilot;
        try {
            SGPropertyNode_ptr root = new SGPropertyNode();
            readProperties( config.str(), root );


            if ( ! ap->build( root ) ) {
                SG_LOG( SG_ALL, SG_ALERT,
                  "Detected an internal inconsistency in the autopilot configuration." << endl << " See earlier errors for details." );
                delete ap;
                continue;
            }        
        } catch (const sg_exception& e) {
            SG_LOG( SG_AUTOPILOT, SG_ALERT, "Failed to load autopilot configuration: "
                    << config.str() << ":" << e.getMessage() );
            delete ap;
            continue;
        }

        SG_LOG( SG_AUTOPILOT, SG_INFO, "adding  autopilot subsystem " << apName );
        set_subsystem( apName, ap );
        _autopilotNames.push_back( apName );
    }

    SGSubsystemGroup::init();
}

FGXMLAutopilot::FGXMLAutopilot() {
}


FGXMLAutopilot::~FGXMLAutopilot() {
}

 
/* read all /sim/systems/autopilot[n]/path properties, try to read the file specified therein
 * and configure/add the digital filters specified in that file
 */
void FGXMLAutopilot::init() 
{
}


void FGXMLAutopilot::reinit() {
    components.clear();
    init();
}


void FGXMLAutopilot::bind() {
}

void FGXMLAutopilot::unbind() {
}

bool FGXMLAutopilot::build( SGPropertyNode_ptr config_props ) {
    SGPropertyNode *node;
    int i;

    int count = config_props->nChildren();
    for ( i = 0; i < count; ++i ) {
        node = config_props->getChild(i);
        string name = node->getName();
        // cout << name << endl;
        SG_LOG( SG_AUTOPILOT, SG_BULK, "adding  autopilot component " << name );
        if ( name == "pid-controller" ) {
            components.push_back( new FGPIDController( node ) );
        } else if ( name == "pi-simple-controller" ) {
            components.push_back( new FGPISimpleController( node ) );
        } else if ( name == "predict-simple" ) {
            components.push_back( new FGPredictor( node ) );
        } else if ( name == "filter" ) {
            components.push_back( new FGDigitalFilter( node ) );
        } else if ( name == "logic" ) {
            components.push_back( new FGXMLAutoLogic( node ) );
        } else if ( name == "flipflop" ) {
            FGXMLAutoFlipFlop * flipFlop = NULL;
            SGPropertyNode_ptr typeNode = node->getNode( "type" );            
            string val;
            if( typeNode != NULL ) val = typeNode->getStringValue();
            val = simgear::strutils::strip(val);
            if( val == "RS" || val =="SR" ) flipFlop = new FGXMLAutoRSFlipFlop( node );
            else if( val == "JK" ) flipFlop = new FGXMLAutoJKFlipFlop( node );
            else if( val == "T" ) flipFlop  = new FGXMLAutoTFlipFlop( node );
            else if( val == "D" ) flipFlop  = new FGXMLAutoDFlipFlop( node );
            if( flipFlop == NULL ) {
              SG_LOG(SG_AUTOPILOT, SG_ALERT, "can't create flipflop of type: " << val);
              return false;
            }
            components.push_back( flipFlop );
        } else {
            SG_LOG( SG_AUTOPILOT, SG_WARN, "Unknown top level autopilot section: " << name );
//            return false;
        }
    }

    return true;
}

/*
 * Update the list of autopilot components
 */

void FGXMLAutopilot::update( double dt ) 
{
    unsigned int i;
    for ( i = 0; i < components.size(); ++i ) {
        components[i]->update( dt );
    }
}

