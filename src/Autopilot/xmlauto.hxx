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

/* 
Torsten Dreyer:
I'd like to deprecate the so called autopilot helper function
(which is now part of the AutopilotGroup::update() method).
Every property calculated within this helper can be calculated
using filters defined in an external autopilot definition file.
The complete set of calculations may be extracted into a separate
configuration file. The current implementation is able to hande 
multiple config files and autopilots. The helper doubles code
and writes properties used only by a few aircraft.
*/
// FIXME: this should go into config.h and/or configure
// or removed along with the "helper" one day.
#define XMLAUTO_USEHELPER

#include <simgear/compiler.h>

#include <string>
#include <vector>
#include <deque>

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

template<typename T>
class SGExpression;

typedef SGExpression<double> SGExpressiond;
class SGCondition;

typedef SGSharedPtr<class FGXMLAutoInput> FGXMLAutoInput_ptr;
typedef SGSharedPtr<class FGPeriodicalValue> FGPeriodicalValue_ptr;

class FGPeriodicalValue : public SGReferenced {
private:
     FGXMLAutoInput_ptr minPeriod; // The minimum value of the period
     FGXMLAutoInput_ptr maxPeriod; // The maximum value of the period
public:
     FGPeriodicalValue( SGPropertyNode_ptr node );
     double normalize( double value );
};

class FGXMLAutoInput : public SGReferenced {
private:
     double             value;    // The value as a constant or initializer for the property
     bool               abs;      // return absolute value
     SGPropertyNode_ptr property; // The name of the property containing the value
     FGXMLAutoInput_ptr offset;   // A fixed offset, defaults to zero
     FGXMLAutoInput_ptr scale;    // A constant scaling factor defaults to one
     FGXMLAutoInput_ptr min;      // A minimum clip defaults to no clipping
     FGXMLAutoInput_ptr max;      // A maximum clip defaults to no clipping
     FGPeriodicalValue_ptr  periodical; //
     SGSharedPtr<const SGCondition> _condition;
     SGSharedPtr<SGExpressiond> _expression;  ///< expression to generate the value
     
public:
    FGXMLAutoInput( SGPropertyNode_ptr node = NULL, double value = 0.0, double offset = 0.0, double scale = 1.0 );
    
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

class FGXMLAutoInputList : public std::vector<FGXMLAutoInput_ptr> {
  public:
    FGXMLAutoInput_ptr get_active() {
      for (iterator it = begin(); it != end(); ++it) {
        if( (*it)->is_enabled() )
          return *it;
      }
      return NULL;
    }

    double get_value( double def = 0.0 ) {
      FGXMLAutoInput_ptr input = get_active();
      return input == NULL ? def : input->get_value();
    }

};

/**
 * Base class for other autopilot components
 */

class FGXMLAutoComponent : public SGReferenced {

private:
    simgear::PropertyList output_list;

    SGSharedPtr<const SGCondition> _condition;
    SGPropertyNode_ptr enable_prop;
    std::string * enable_value;

    SGPropertyNode_ptr passive_mode;
    bool honor_passive;

    std::string name;

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
    FGXMLAutoComponent();
    
    /*
     * Parse a component specification read from a property-list.
     * Calls the hook methods below to allow derived classes to
     * specialise parsing bevaiour.
     */
    void parseNode(SGPropertyNode* aNode);

    /**
     * Helper to parse the config section
     */
    void parseConfig(SGPropertyNode* aConfig);

    /*
     * Over-rideable hook method to allow derived classes to refine top-level
     * node parsing. Return true if the node was handled, false otherwise.
     */
    virtual bool parseNodeHook(const std::string& aName, SGPropertyNode* aNode);
    
    /**
     * Over-rideable hook method to allow derived classes to refine config
     * node parsing. Return true if the node was handled, false otherwise.
     */
    virtual bool parseConfigHook(const std::string& aName, SGPropertyNode* aNode);

    FGXMLAutoInputList valueInput;
    FGXMLAutoInputList referenceInput;
    FGXMLAutoInputList uminInput;
    FGXMLAutoInputList umaxInput;
    FGPeriodicalValue_ptr periodical;
    // debug flag
    bool debug;
    bool enabled;

    
    inline void do_feedback() {
        if( feedback_if_disabled ) do_feedback_if_disabled();
    }

public:
    
    virtual ~FGXMLAutoComponent();

    virtual void update (double dt)=0;
    
    inline const std::string& get_name() { return name; }

    double clamp( double value );

    inline void set_output_value( double value ) {
        // passive_ignore == true means that we go through all the
        // motions, but drive the outputs.  This is analogous to
        // running the autopilot with the "servos" off.  This is
        // helpful for things like flight directors which position
        // their vbars from the autopilot computations.
        if ( honor_passive && passive_mode->getBoolValue() ) return;
        for( simgear::PropertyList::iterator it = output_list.begin();
             it != output_list.end(); ++it)
          (*it)->setDoubleValue( clamp( value ) );
    }

    inline void set_output_value( bool value ) {
        // passive_ignore == true means that we go through all the
        // motions, but drive the outputs.  This is analogous to
        // running the autopilot with the "servos" off.  This is
        // helpful for things like flight directors which position
        // their vbars from the autopilot computations.
        if ( honor_passive && passive_mode->getBoolValue() ) return;
        for( simgear::PropertyList::iterator it = output_list.begin();
             it != output_list.end(); ++it)
          (*it)->setBoolValue( value ); // don't use clamp here, bool is clamped anyway
    }

    inline double get_output_value() {
      return output_list.size() == 0 ? 0.0 : clamp(output_list[0]->getDoubleValue());
    }

    inline bool get_bool_output_value() {
      return output_list.size() == 0 ? false : output_list[0]->getBoolValue();
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

typedef SGSharedPtr<FGXMLAutoComponent> FGXMLAutoComponent_ptr;


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
    

protected:
  bool parseConfigHook(const std::string& aName, SGPropertyNode* aNode);
    
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

protected:
  bool parseConfigHook(const std::string& aName, SGPropertyNode* aNode);

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
    double average;
    FGXMLAutoInputList seconds;
    FGXMLAutoInputList filter_gain;

protected:
  bool parseNodeHook(const std::string& aName, SGPropertyNode* aNode);

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

    std::deque <double> output;
    std::deque <double> input;
    enum FilterTypes { exponential, doubleExponential, movingAverage,
                       noiseSpike, gain, reciprocal, differential, none };
    FilterTypes filterType;

protected:
  bool parseNodeHook(const std::string& aName, SGPropertyNode* aNode);
  
public:
    FGDigitalFilter(SGPropertyNode *node);
    ~FGDigitalFilter() {}

    void update(double dt);
};

class FGXMLAutoLogic : public FGXMLAutoComponent
{
private:
    SGSharedPtr<SGCondition> input;
    bool inverted;

protected:
    bool parseNodeHook(const std::string& aName, SGPropertyNode* aNode);

public:
    FGXMLAutoLogic(SGPropertyNode * node );
    ~FGXMLAutoLogic() {}

    void update(double dt);
};

class FGXMLAutoFlipFlop : public FGXMLAutoComponent
{
private:
protected:
    SGSharedPtr<SGCondition> sInput;
    SGSharedPtr<SGCondition> rInput;
    SGSharedPtr<SGCondition> clockInput;
    SGSharedPtr<SGCondition> jInput;
    SGSharedPtr<SGCondition> kInput;
    SGSharedPtr<SGCondition> dInput;
    bool inverted;
    FGXMLAutoFlipFlop( SGPropertyNode * node );
    bool parseNodeHook(const std::string& aName, SGPropertyNode* aNode);

    void update( double dt );

public:
    ~FGXMLAutoFlipFlop() {};
    virtual bool getState( bool & result ) = 0;
};

/**
 * Model an autopilot system.
 * 
 */

class FGXMLAutopilotGroup : public SGSubsystemGroup
{
public:
    FGXMLAutopilotGroup();
    void init();
    void reinit();
    void update( double dt );
private:
    std::vector<std::string> _autopilotNames;

#ifdef XMLAUTO_USEHELPER
    double average;
    double v_last;
    double last_static_pressure;

    SGPropertyNode_ptr vel;
    SGPropertyNode_ptr lookahead5;
    SGPropertyNode_ptr lookahead10;
    SGPropertyNode_ptr bug;
    SGPropertyNode_ptr mag_hdg;
    SGPropertyNode_ptr bug_error;
    SGPropertyNode_ptr fdm_bug_error;
    SGPropertyNode_ptr target_true;
    SGPropertyNode_ptr true_hdg;
    SGPropertyNode_ptr true_error;
    SGPropertyNode_ptr target_nav1;
    SGPropertyNode_ptr true_nav1;
    SGPropertyNode_ptr true_track_nav1;
    SGPropertyNode_ptr nav1_course_error;
    SGPropertyNode_ptr nav1_selected_course;
    SGPropertyNode_ptr vs_fps;
    SGPropertyNode_ptr vs_fpm;
    SGPropertyNode_ptr static_pressure;
    SGPropertyNode_ptr pressure_rate;
    SGPropertyNode_ptr track;
#endif
};

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


    bool build( SGPropertyNode_ptr );
protected:
    typedef std::vector<FGXMLAutoComponent_ptr> comp_list;

private:
    bool serviceable;
    comp_list components;
    
};


#endif // _XMLAUTO_HXX
