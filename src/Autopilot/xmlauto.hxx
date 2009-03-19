// xmlauto.hxx - a more flexible, generic way to build autopilots
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


#ifndef _XMLAUTO_HXX
#define _XMLAUTO_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <string>
#include <vector>
#include <deque>

using std::string;
using std::vector;
using std::deque;

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/condition.hxx>

#include <Main/fg_props.hxx>


class FGXMLAutoInput {
private:
     SGPropertyNode_ptr property; // The name of the property containing the value
     double             value;    // The value as a constant or initializer for the property
     double             offset;   // A fixed offset
     double             scale;    // A constant scaling factor

public:
    FGXMLAutoInput() :
      property(NULL),
      value(0.0),
      offset(0.0),
      scale(1.0) {}

    void   parse( SGPropertyNode_ptr, double value = 0.0, double offset = 0.0, double scale = 1.0 );
    inline double getValue() {
      if( property != NULL ) value = property->getDoubleValue();
      return value * scale + offset;
    }
};

/**
 * Base class for other autopilot components
 */

class FGXMLAutoComponent : public SGReferenced {

private:
    bool clamp;
    vector <SGPropertyNode_ptr> output_list;

    SGSharedPtr<const SGCondition> _condition;
    SGPropertyNode_ptr enable_prop;
    string * enable_value;

    SGPropertyNode_ptr passive_mode;
    bool honor_passive;

    string name;
protected:

    FGXMLAutoInput valueInput;
    FGXMLAutoInput referenceInput;
    FGXMLAutoInput uminInput;
    FGXMLAutoInput umaxInput;
    // debug flag
    bool debug;
    bool enabled;

public:

    FGXMLAutoComponent( SGPropertyNode *node);
    virtual ~FGXMLAutoComponent();

    virtual void update (double dt)=0;
    
    inline const string& get_name() { return name; }

    inline double Clamp( double value ) {
        if( clamp ) {
            double d = umaxInput.getValue();
            if( value > d ) value = d;
            d = uminInput.getValue();
            if( value < d ) value = d;
        }
        return value;
    }

    inline void setOutputValue( double value ) {
        // passive_ignore == true means that we go through all the
        // motions, but drive the outputs.  This is analogous to
        // running the autopilot with the "servos" off.  This is
        // helpful for things like flight directors which position
        // their vbars from the autopilot computations.
        if ( honor_passive && passive_mode->getBoolValue() ) return;
        for ( unsigned i = 0; i < output_list.size(); ++i ) {
            output_list[i]->setDoubleValue( Clamp(value) );
        }
    }

    inline double getOutputValue() {
      return output_list.size() == 0 ? 0.0 : Clamp(output_list[0]->getDoubleValue());
    }

    /* 
       Returns true if the enable-condition is true.

       If a <condition> is defined, this condition is evaluated, 
       <prop> and <value> tags are ignored.

       If a <prop> is defined and no <value> is defined, the property
       named in the <prop></prop> tags is evaluated as boolean.

       If a <prop> is defined a a <value> is defined, the property named
       in <prop></prop> is compared (as a string) to the value defined in
       <value></value>

       Returns true, if neither <condition> nor <prop> exists

       Examples:
       Using a <condition> tag
       <enable>
         <condition>
           <!-- any legal condition goes here and is evaluated -->
         </condition>
         <prop>This is ignored</prop>
         <value>This is also ignored</value>
       </enable>

       Using a single boolean property
       <enable>
         <prop>/some/property/that/is/evaluated/as/boolean</prop>
       </enable>

       Using <prop> == <value>
       This is the old style behaviour
       <enable>
         <prop>/only/true/if/this/equals/true</prop>
         <value>true<value>
       </enable>
    */
    bool isPropertyEnabled();
};


/**
 * Roy Ovesen's PID controller
 */

class FGPIDController : public FGXMLAutoComponent {

private:


    // Configuration values
    FGXMLAutoInput Kp;          // proportional gain
    FGXMLAutoInput Ti;          // Integrator time (sec)
    FGXMLAutoInput Td;          // Derivator time (sec)

    double alpha;               // low pass filter weighing factor (usually 0.1)
    double beta;                // process value weighing factor for
                                // calculating proportional error
                                // (usually 1.0)
    double gamma;               // process value weighing factor for
                                // calculating derivative error
                                // (usually 0.0)

    // Previous state tracking values
    double ep_n_1;              // ep[n-1]  (prop error)
    double edf_n_1;             // edf[n-1] (derivative error)
    double edf_n_2;             // edf[n-2] (derivative error)
    double u_n_1;               // u[n-1]   (output)
    double desiredTs;            // desired sampling interval (sec)
    double elapsedTime;          // elapsed time (sec)
    
    
    
public:

    FGPIDController( SGPropertyNode *node );
    FGPIDController( SGPropertyNode *node, bool old );
    ~FGPIDController() {}

    void update_old( double dt );
    void update( double dt );
};


/**
 * A simplistic P [ + I ] PID controller
 */

class FGPISimpleController : public FGXMLAutoComponent {

private:

    // proportional component data
    FGXMLAutoInput Kp;

    // integral component data
    FGXMLAutoInput Ki;
    double int_sum;


public:

    FGPISimpleController( SGPropertyNode *node );
    ~FGPISimpleController() {}

    void update( double dt );
};


/**
 * Predictor - calculates value in x seconds future.
 */

class FGPredictor : public FGXMLAutoComponent {

private:

    // proportional component data
    double last_value;
    double average;
    double seconds;
    double filter_gain;

    // Input values
    double ivalue;                 // input value
    
public:

    FGPredictor( SGPropertyNode *node );
    ~FGPredictor() {}

    void update( double dt );
};


/**
 * FGDigitalFilter - a selection of digital filters
 *
 * Exponential filter
 * Double exponential filter
 * Moving average filter
 * Noise spike filter
 *
 * All these filters are low-pass filters.
 *
 */

class FGDigitalFilter : public FGXMLAutoComponent
{
private:
    FGXMLAutoInput samplesInput; // Number of input samples to average
    FGXMLAutoInput rateOfChangeInput;  // The maximum allowable rate of change [1/s]
    FGXMLAutoInput gainInput;     // 
    FGXMLAutoInput TfInput;            // Filter time [s]

    deque <double> output;
    deque <double> input;
    enum filterTypes { exponential, doubleExponential, movingAverage,
                       noiseSpike, gain, reciprocal };
    filterTypes filterType;

public:
    FGDigitalFilter(SGPropertyNode *node);
    ~FGDigitalFilter() {}

    void update(double dt);
};

/**
 * Model an autopilot system.
 * 
 */

class FGXMLAutopilot : public SGSubsystem
{

public:

    FGXMLAutopilot();
    ~FGXMLAutopilot();

    void init();
    void reinit();
    void bind();
    void unbind();
    void update( double dt );

    bool build();

protected:

    typedef vector<SGSharedPtr<FGXMLAutoComponent> > comp_list;

private:

    bool serviceable;
    SGPropertyNode_ptr config_props;
    comp_list components;
};


#endif // _XMLAUTO_HXX
