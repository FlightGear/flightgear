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


class FGXMLAutoInput : public SGReferenced {
private:
     double             value;    // The value as a constant or initializer for the property
     SGPropertyNode_ptr property; // The name of the property containing the value
     SGSharedPtr<FGXMLAutoInput> offset;   // A fixed offset, defaults to zero
     SGSharedPtr<FGXMLAutoInput> scale;    // A constant scaling factor defaults to one
     SGSharedPtr<FGXMLAutoInput> min;      // A minimum clip defaults to no clipping
     SGSharedPtr<FGXMLAutoInput> max;      // A maximum clip defaults to no clipping
     SGSharedPtr<const SGCondition> _condition;

public:
    FGXMLAutoInput( SGPropertyNode_ptr node = NULL, double value = 0.0, double offset = 0.0, double scale = 1.0 ) :
      property(NULL),
      value(0.0),
      offset(NULL),
      scale(NULL),
      min(NULL),
      max(NULL),
      _condition(NULL) {
       parse( node, value, offset, scale );
     }

    void parse( SGPropertyNode_ptr, double value = 0.0, double offset = 0.0, double scale = 1.0 );

    /* get the value of this input, apply scale and offset and clipping */
    double get_value();

    /* set the input value after applying offset and scale */
    void set_value( double value );

    inline double get_scale() {
      return scale == NULL ? 1.0 : scale->get_value();
    }

    inline double get_offset() {
      return offset == NULL ? 0.0 : offset->get_value();
    }

    inline bool is_enabled() {
      return _condition == NULL ? true : _condition->test();
    }

};

class FGXMLAutoInputList : public vector<SGSharedPtr<FGXMLAutoInput> > {
  public:
    FGXMLAutoInput * get_active() {
      for (iterator it = begin(); it != end(); ++it) {
        if( (*it)->is_enabled() )
          return *it;
      }
      return NULL;
    }

    double get_value( double def = 0.0 ) {
      FGXMLAutoInput * input = get_active();
      return input == NULL ? def : input->get_value();
    }

};

/**
 * Base class for other autopilot components
 */

class FGXMLAutoComponent : public SGReferenced {

private:
    vector <SGPropertyNode_ptr> output_list;

    SGSharedPtr<const SGCondition> _condition;
    SGPropertyNode_ptr enable_prop;
    string * enable_value;

    SGPropertyNode_ptr passive_mode;
    bool honor_passive;

    string name;

    /* Feed back output property to input property if
       this filter is disabled. This is for multi-stage
       filter where one filter sits behind a pid-controller
       to provide changes of the overall output to the pid-
       controller.
       feedback is disabled by default.
     */
    bool feedback_if_disabled;
    void do_feedback_if_disabled();

protected:

    FGXMLAutoInputList valueInput;
    FGXMLAutoInputList referenceInput;
    FGXMLAutoInputList uminInput;
    FGXMLAutoInputList umaxInput;
    // debug flag
    bool debug;
    bool enabled;

    
    inline void do_feedback() {
        if( feedback_if_disabled ) do_feedback_if_disabled();
    }

public:

    FGXMLAutoComponent( SGPropertyNode *node);
    virtual ~FGXMLAutoComponent();

    virtual void update (double dt)=0;
    
    inline const string& get_name() { return name; }

    double clamp( double value );

    inline void set_output_value( double value ) {
        // passive_ignore == true means that we go through all the
        // motions, but drive the outputs.  This is analogous to
        // running the autopilot with the "servos" off.  This is
        // helpful for things like flight directors which position
        // their vbars from the autopilot computations.
        if ( honor_passive && passive_mode->getBoolValue() ) return;
        for( vector <SGPropertyNode_ptr>::iterator it = output_list.begin(); it != output_list.end(); ++it)
          (*it)->setDoubleValue( clamp( value ) );
    }

    inline double get_output_value() {
      return output_list.size() == 0 ? 0.0 : clamp(output_list[0]->getDoubleValue());
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
    FGXMLAutoInputList Kp;          // proportional gain
    FGXMLAutoInputList Ti;          // Integrator time (sec)
    FGXMLAutoInputList Td;          // Derivator time (sec)

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

    void update( double dt );
};


/**
 * A simplistic P [ + I ] PID controller
 */

class FGPISimpleController : public FGXMLAutoComponent {

private:

    // proportional component data
    FGXMLAutoInputList Kp;

    // integral component data
    FGXMLAutoInputList Ki;
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
    double last_value;
    FGXMLAutoInputList seconds;
    FGXMLAutoInputList filter_gain;

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
    FGXMLAutoInputList samplesInput; // Number of input samples to average
    FGXMLAutoInputList rateOfChangeInput;  // The maximum allowable rate of change [1/s]
    FGXMLAutoInputList gainInput;     // 
    FGXMLAutoInputList TfInput;            // Filter time [s]

    deque <double> output;
    deque <double> input;
    enum filterTypes { exponential, doubleExponential, movingAverage,
                       noiseSpike, gain, reciprocal, none };
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
