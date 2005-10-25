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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

#include STL_STRING
#include <vector>
#include <deque>

SG_USING_STD(string);
SG_USING_STD(vector);
SG_USING_STD(deque);

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Base class for other autopilot components
 */

class FGXMLAutoComponent {

protected:

    string name;

    SGPropertyNode *enable_prop;
    string enable_value;
    bool enabled;

    SGPropertyNode *input_prop;
    SGPropertyNode *r_n_prop;
    double r_n_value;
    vector <SGPropertyNode *> output_list;

public:

    FGXMLAutoComponent() :
      enable_prop( NULL ),
      enable_value( "" ),
      enabled( false ),
      input_prop( NULL ),
      r_n_prop( NULL ),
      r_n_value( 0.0 )
    { }

    virtual ~FGXMLAutoComponent() {}

    virtual void update (double dt)=0;
    
    inline const string& get_name() { return name; }
};


/**
 * Roy Ovesen's PID controller
 */

class FGPIDController : public FGXMLAutoComponent {

private:

    // debug flag
    bool debug;

    // Input values
    double y_n;                 // measured process value
    double r_n;                 // reference (set point) value
    double y_scale;             // scale process input from property system
    double r_scale;             // scale reference input from property system
    double y_offset;
    double r_offset;

    // Configuration values
    double Kp;                  // proportional gain

    double alpha;               // low pass filter weighing factor (usually 0.1)
    double beta;                // process value weighing factor for
                                // calculating proportional error
                                // (usually 1.0)
    double gamma;               // process value weighing factor for
                                // calculating derivative error
                                // (usually 0.0)

    double Ti;                  // Integrator time (sec)
    double Td;                  // Derivator time (sec)

    double u_min;               // Minimum output clamp
    double u_max;               // Maximum output clamp

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
    bool proportional;
    double Kp;
    SGPropertyNode *offset_prop;
    double offset_value;

    // integral component data
    bool integral;
    double Ki;
    double int_sum;

    // post functions for output
    bool clamp;

    // debug flag
    bool debug;

    // Input values
    double y_n;                 // measured process value
    double r_n;                 // reference (set point) value
    double y_scale;             // scale process input from property system
    double r_scale;             // scale reference input from property system

    double u_min;               // Minimum output clamp
    double u_max;               // Maximum output clamp

    
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

    // debug flag
    bool debug;

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
    double Tf;            // Filter time [s]
    unsigned int samples; // Number of input samples to average
    double rateOfChange;  // The maximum allowable rate of change [1/s]
    deque <double> output;
    deque <double> input;
    enum filterTypes { exponential, doubleExponential, movingAverage, noiseSpike };
    filterTypes filterType;

    bool debug;

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

    typedef vector<FGXMLAutoComponent *> comp_list;

private:

    bool serviceable;
    SGPropertyNode *config_props;
    comp_list components;
};


#endif // _XMLAUTO_HXX
