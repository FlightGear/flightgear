// electrical.hxx - a flexible, generic electrical system model.
//
// Written by Curtis Olson, started September 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _SYSTEMS_ELECTRICAL_HXX
#define _SYSTEMS_ELECTRICAL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include STL_STRING
#include <vector>

SG_USING_STD(string);
SG_USING_STD(vector);

#include <simgear/misc/props.hxx>
#include <Main/fgfs.hxx>


// Forward declaration
class FGElectricalSystem;


#define FG_UNKNOWN  -1
#define FG_SUPPLIER  0
#define FG_BUS       1
#define FG_OUTPUT    2
#define FG_CONNECTOR 3

// Base class for other electrical components
class FGElectricalComponent {

protected:

    typedef vector<FGElectricalComponent *> comp_list;
    typedef vector<string> string_list;

    int kind;
    string name;
    double value;

    comp_list inputs;
    comp_list outputs;
    string_list props;

public:

    FGElectricalComponent();
    virtual ~FGElectricalComponent() {}

    inline string get_name() { return name; }

    inline int get_kind() const { return kind; }
    inline double get_value() const { return value; }
    inline void set_value( double val ) { value = val; }

    inline int get_num_inputs() const { return outputs.size(); }
    inline FGElectricalComponent *get_input( const int i ) {
        return inputs[i];
    }
    inline void add_input( FGElectricalComponent *c ) {
        inputs.push_back( c );
    }

    inline int get_num_outputs() const { return outputs.size(); }
    inline FGElectricalComponent *get_output( const int i ) {
        return outputs[i];
    }
    inline void add_output( FGElectricalComponent *c ) {
        outputs.push_back( c );
    }

    inline int get_num_props() const { return props.size(); }
    inline string get_prop( const int i ) {
        return props[i];
    }
    inline void add_prop( const string &s ) {
        props.push_back( s );
    }

};


// Electrical supplier
class FGElectricalSupplier : public FGElectricalComponent {

    SGPropertyNode_ptr _rpm_node;

    enum FGSupplierType {
        FG_BATTERY = 0,
        FG_ALTERNATOR = 1,
        FG_EXTERNAL = 2
    };

    int model;
    double volts;
    double amps;

public:

    FGElectricalSupplier ( SGPropertyNode *node );
    ~FGElectricalSupplier () {}

    double get_output();
};


// Electrical bus (can take multiple inputs and provide multiple
// outputs)
class FGElectricalBus : public FGElectricalComponent {

public:

    FGElectricalBus ( SGPropertyNode *node );
    ~FGElectricalBus () {}
};


// A lot like an FGElectricalBus, but here for convenience and future
// flexibility
class FGElectricalOutput : public FGElectricalComponent {

public:

    FGElectricalOutput ( SGPropertyNode *node );
    ~FGElectricalOutput () {}
};


// Connects multiple sources to multiple destinations with optional
// switches/fuses/circuit breakers inline
class FGElectricalConnector : public FGElectricalComponent {

    comp_list inputs;
    comp_list outputs;
    typedef vector<SGPropertyNode *> switch_list;
    switch_list switches;

public:

    FGElectricalConnector ( SGPropertyNode *node, FGElectricalSystem *es );
    ~FGElectricalConnector () {}

    void add_switch( SGPropertyNode *node ) {
        switches.push_back( node );
    }

    // set all switches to the specified state
    void set_switches( bool state );

    bool get_state();
};


/**
 * Model an electrical system.  This is a simple system with the
 * alternator hardwired to engine[0]/rpm
 *
 * Input properties:
 *
 * /engines/engine[0]/rpm
 *
 * Output properties:
 *
 * 
 */

class FGElectricalSystem : public FGSubsystem
{

public:

    FGElectricalSystem ();
    virtual ~FGElectricalSystem ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

    bool build ();
    void propagate( FGElectricalComponent *node, double val, string s = "" );
    FGElectricalComponent *find ( const string &name );

protected:

    typedef vector<FGElectricalComponent *> comp_list;

private:

    SGPropertyNode *config_props;

    bool enabled;

    comp_list suppliers;
    comp_list buses;
    comp_list outputs;
    comp_list connectors;
};


#endif // _SYSTEMS_ELECTRICAL_HXX
