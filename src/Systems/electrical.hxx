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


#define FG_UNKNOWN  -1
#define FG_SUPPLIER  0
#define FG_BUS       1
#define FG_OUTPUT    2
#define FG_CONNECTOR 3

// Base class for other electrical components
class FGElectricalComponent {

    typedef vector<FGElectricalComponent *> comp_list;
    typedef vector<string> string_list;

public:

    FGElectricalComponent() {}
    virtual ~FGElectricalComponent() {}

    virtual string get_name() { return ""; }

    int kind;
    inline int get_kind() { return kind; }
};


// Electrical supplier
class FGElectricalSupplier : public FGElectricalComponent {

    enum FGSupplierType {
        FG_BATTERY = 0,
        FG_ALTERNATOR = 1,
        FG_EXTERNAL = 2
    };

    string name;
    int model;
    double volts;
    double amps;

    comp_list outputs;

public:

    FGElectricalSupplier ( string _name, string _model,
                           double _volts, double _amps );
    ~FGElectricalSupplier () {}

    void add_output( FGElectricalComponent *c ) {
        outputs.push_back( c );
    }

    string get_name() const { return name; }
};


// Electrical bus (can take multiple inputs and provide multiple
// outputs)
class FGElectricalBus : public FGElectricalComponent {

    string name;
    comp_list inputs;
    comp_list outputs;

public:

    FGElectricalBus ( string _name );
    ~FGElectricalBus () {}

    void add_input( FGElectricalComponent *c ) {
        inputs.push_back( c );
    }

    void add_output( FGElectricalComponent *c ) {
        outputs.push_back( c );
    }

    string get_name() const { return name; }
};


// A lot like an FGElectricalBus, but here for convenience and future
// flexibility
class FGElectricalOutput : public FGElectricalComponent {

    string name;
    comp_list inputs;

public:

    FGElectricalOutput ( string _name );
    ~FGElectricalOutput () {}

    void add_input( FGElectricalComponent *c ) {
        inputs.push_back( c );
    }

    string get_name() const { return name; }
};


// Connects multiple sources to multiple destinations with optional
// switches/fuses/circuit breakers inline
class FGElectricalConnector : public FGElectricalComponent {

    comp_list inputs;
    comp_list outputs;
    string_list switches;

public:

    FGElectricalConnector ();
    ~FGElectricalConnector () {}

    void add_input( FGElectricalComponent *c ) {
        inputs.push_back( c );
    }

    void add_output( FGElectricalComponent *c ) {
        outputs.push_back( c );
    }

    void add_switch( const string &s ) {
        switches.push_back( s );
    }

    string get_name() const { return ""; }
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
    FGElectricalComponent *find ( const string &name );

private:

    SGPropertyNode *config_props;
    // SGPropertyNode_ptr _serviceable_node;

    bool enabled;

    typedef vector<FGElectricalComponent *> comp_list;

    comp_list suppliers;
    comp_list buses;
    comp_list outputs;
    comp_list connectors;
};


#endif // _SYSTEMS_ELECTRICAL_HXX
