// electrical.cxx - a flexible, generic electrical system model.
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


#include <simgear/misc/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "electrical.hxx"


FGElectricalSupplier::FGElectricalSupplier ( string _name, string _model,
                                             double _volts, double _amps )
{
    kind = FG_SUPPLIER;

    name = _name;
    if ( _model == "battery" ) {
        model = FG_BATTERY;
    } else if ( _model == "alternator" ) {
        model = FG_ALTERNATOR;
    } else if ( _model == "external" ) {
        model = FG_EXTERNAL;
    } else {
        model = FG_UNKNOWN;
    }
    volts = _volts;
    amps = _amps;
}  


FGElectricalBus::FGElectricalBus ( string _name )
{
    kind = FG_BUS;

    name = _name;
}  


FGElectricalOutput::FGElectricalOutput ( string _name )
{
    kind = FG_OUTPUT;

    name = _name;
}  


FGElectricalConnector::FGElectricalConnector ()
{
    kind = FG_CONNECTOR;
}  


FGElectricalSystem::FGElectricalSystem () :
    enabled(false)
{
}


FGElectricalSystem::~FGElectricalSystem () {
}


void FGElectricalSystem::init () {
    config_props = new SGPropertyNode;

    SGPath config( globals->get_fg_root() );
    config.append( fgGetString("/systems/electrical/path") );

    SG_LOG( SG_ALL, SG_ALERT, "Reading electrical system model from "
            << config.str() );
    try {
        readProperties( config.str(), config_props );

        if ( build() ) {
            enabled = true;
        } else {
            SG_LOG( SG_ALL, SG_ALERT,
                    "Detected an internal inconsistancy in the electrical" );
            SG_LOG( SG_ALL, SG_ALERT,
                    " system specification file.  See earlier errors for" );
            SG_LOG( SG_ALL, SG_ALERT,
                    " details.");
            exit(-1);
        }        
    } catch (const sg_exception& exc) {
        SG_LOG( SG_ALL, SG_ALERT, "Failed to load electrical system model: "
                << config.str() );
    }

    delete config_props;
}


void FGElectricalSystem::bind () {
}


void FGElectricalSystem::unbind () {
}


void FGElectricalSystem::update (double dt) {
}


bool FGElectricalSystem::build () {
    SGPropertyNode *node;
    int i, j;

    int count = config_props->nChildren();
    for ( i = 0; i < count; ++i ) {
        node = config_props->getChild(i);
        string name = node->getName();
        // cout << name << endl;
        if ( name == "supplier" ) {
            FGElectricalSupplier *s =
                new FGElectricalSupplier( node->getStringValue("name"),
                                          node->getStringValue("kind"),
                                          node->getDoubleValue("volts"),
                                          node->getDoubleValue("amps") );
            suppliers.push_back( s );
        } else if ( name == "bus" ) {
            FGElectricalBus *b =
                new FGElectricalBus( node->getStringValue("name") );
            buses.push_back( b );
        } else if ( name == "output" ) {
            FGElectricalOutput *o =
                new FGElectricalOutput( node->getStringValue("name") );
            outputs.push_back( o );
        } else if ( name == "connector" ) {
            FGElectricalConnector *c =
                new FGElectricalConnector();
            connectors.push_back( c );
            SGPropertyNode *child;
            int ccount = node->nChildren();
            for ( j = 0; j < ccount; ++j ) {
                child = node->getChild(j);
                string cname = child->getName();
                string cval = child->getStringValue();
                // cout << "  " << cname << " = " << cval << endl;
                if ( cname == "input" ) {
                    FGElectricalComponent *s = find( child->getStringValue() );
                    if ( s != NULL ) {
                        c->add_input( s );
                        if ( s->get_kind() == FG_SUPPLIER ) {
                            ((FGElectricalSupplier *)s)->add_output( c );
                        } else if ( s->get_kind() == FG_BUS ) {
                            ((FGElectricalBus *)s)->add_output( c );
                        } else {
                            SG_LOG( SG_ALL, SG_ALERT,
                                    "Attempt to connect to something that can't provide an output: " 
                                    << child->getStringValue() );
                            return false;
                        }
                    } else {
                        SG_LOG( SG_ALL, SG_ALERT,
                                "Can't find named source: " 
                                << child->getStringValue() );
                        return false;
                    }
                } else if ( cname == "output" ) {
                    FGElectricalComponent *s = find( child->getStringValue() );
                    if ( s != NULL ) {
                        c->add_output( s );
                        if ( s->get_kind() == FG_BUS ) {
                            ((FGElectricalBus *)s)->add_input( c );
                        } else if ( s->get_kind() == FG_OUTPUT ) {
                            ((FGElectricalOutput *)s)->add_input( c );
                        } else {
                            SG_LOG( SG_ALL, SG_ALERT,
                                    "Attempt to connect to something that can't provide an input: " 
                                    << child->getStringValue() );
                            return false;
                        }
                    } else {
                        SG_LOG( SG_ALL, SG_ALERT,
                                "Can't find named source: " 
                                << child->getStringValue() );
                        return false;
                    }
                } else if ( cname == "switch" ) {
                    c->add_switch( child->getStringValue() );
                }
            }
        } else {
            SG_LOG( SG_ALL, SG_ALERT, "Unknown component type specified: " 
                    << name );
            return false;
        }
    }

    return true;
}


// search for the named component and return a pointer to it, NULL otherwise
FGElectricalComponent *FGElectricalSystem::find ( const string &name ) {
    unsigned int i;
    string s;

    // search suppliers
    for ( i = 0; i < suppliers.size(); ++i ) {
        s = ((FGElectricalSupplier *)suppliers[i])->get_name();
        // cout <<  "    " << s << endl;
        if ( s == name ) {
            return suppliers[i];
        }
    }

    // then search buses
    for ( i = 0; i < buses.size(); ++i ) {
        s = ((FGElectricalBus *)buses[i])->get_name();
        // cout <<  "    " << s << endl;
        if ( s == name ) {
            return buses[i];
        }
    }

    // then search outputs
    for ( i = 0; i < outputs.size(); ++i ) {
        s = ((FGElectricalOutput *)outputs[i])->get_name();
        // cout <<  "    " << s << endl;
        if ( s == name ) {
            return outputs[i];
        }
    }

    // nothing found
    return NULL;
}
