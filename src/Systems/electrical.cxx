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


#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "electrical.hxx"


FGElectricalComponent::FGElectricalComponent() :
    kind(-1),
    name(""),
    value(0.0)
{
}


FGElectricalSupplier::FGElectricalSupplier ( SGPropertyNode *node ) {
    kind = FG_SUPPLIER;

    // cout << "Creating a supplier" << endl;
    name = node->getStringValue("name");
    string _model = node->getStringValue("kind");
    // cout << "_model = " << _model << endl;
    if ( _model == "battery" ) {
        model = FG_BATTERY;
    } else if ( _model == "alternator" ) {
        model = FG_ALTERNATOR;
    } else if ( _model == "external" ) {
        model = FG_EXTERNAL;
    } else {
        model = FG_UNKNOWN;
    }
    volts = node->getDoubleValue("volts");
    amps = node->getDoubleValue("amps");
    rpm_src = node->getStringValue("rpm-source");

    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        // cout << " scanning: " << child->getName() << endl;
        if ( child->getName() == "prop" ) {
            string prop = child->getStringValue();
            // cout << "  Adding prop = " << prop << endl;
            add_prop( prop );
            fgSetDouble( prop.c_str(), amps );
        }
    }

    _rpm_node = fgGetNode( rpm_src.c_str(), true);
}  


double FGElectricalSupplier::get_output() {
    if ( model == FG_BATTERY ) {
        // cout << "battery amps = " << amps << endl;
        return amps;
    } else if ( model == FG_ALTERNATOR ) {
        // scale alternator output for rpms < 600.  For rpms >= 600
        // give full output.  This is just a WAG, and probably not how
        // it really works but I'm keeping things "simple" to start.
        double rpm = _rpm_node->getDoubleValue();
        double factor = rpm / 600.0;
        if ( factor > 1.0 ) {
            factor = 1.0;
        }
        // cout << "alternator amps = " << amps * factor << endl;
        return amps * factor;
    } else if ( model == FG_EXTERNAL ) {
        // cout << "external amps = " << 0.0 << endl;
        return 0.0;
    } else {
        SG_LOG( SG_ALL, SG_ALERT, "unknown supplier type" );
    }

    return 0.0;
}


FGElectricalBus::FGElectricalBus ( SGPropertyNode *node ) {
    kind = FG_BUS;

    name = node->getStringValue("name");
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        if ( child->getName() == "prop" ) {
            string prop = child->getStringValue();
            add_prop( prop );
        }
    }
}  


FGElectricalOutput::FGElectricalOutput ( SGPropertyNode *node ) {
    kind = FG_OUTPUT;

    name = node->getStringValue("name");
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        if ( child->getName() == "prop" ) {
            string prop = child->getStringValue();
            add_prop( prop );
        }
    }
}  


FGElectricalConnector::FGElectricalConnector ( SGPropertyNode *node,
                                               FGElectricalSystem *es ) {
    kind = FG_CONNECTOR;
    name = "connector";
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        // cout << "  " << cname << " = " << cval << endl;
        if ( cname == "input" ) {
            FGElectricalComponent *s = es->find( child->getStringValue() );
            if ( s != NULL ) {
                add_input( s );
                if ( s->get_kind() == FG_SUPPLIER ) {
                    s->add_output( this );
                } else if ( s->get_kind() == FG_BUS ) {
                    s->add_output( this );
                } else {
                    SG_LOG( SG_ALL, SG_ALERT,
                            "Attempt to connect to something that can't provide an output: " 
                            << child->getStringValue() );
                }
            } else {
                SG_LOG( SG_ALL, SG_ALERT, "Can't find named source: " 
                        << child->getStringValue() );
            }
        } else if ( cname == "output" ) {
            FGElectricalComponent *s = es->find( child->getStringValue() );
            if ( s != NULL ) {
                add_output( s );
                if ( s->get_kind() == FG_BUS ) {
                    s->add_input( this );
                } else if ( s->get_kind() == FG_OUTPUT ) {
                    s->add_input( this );
                } else {
                    SG_LOG( SG_ALL, SG_ALERT,
                            "Attempt to connect to something that can't provide an input: " 
                            << child->getStringValue() );
                }
            } else {
                SG_LOG( SG_ALL, SG_ALERT, "Can't find named source: " 
                        << child->getStringValue() );
            }
        } else if ( cname == "switch" ) {
            // set default value of switch to true
            // cout << "Switch = " << child->getStringValue() << endl;
            fgSetBool( child->getStringValue(), true );
            add_switch( fgGetNode( child->getStringValue(), true ) );
        }
    }

    // do a 2nd pass to pick up starting switch value if specified
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        // cout << "  " << cname << " = " << cval << endl;
        if ( cname == "initial-state" ) {
            if ( cval == "off" ) {
                set_switches( false );
            } else {
                set_switches( true );
            }
        }
    }
}  


// set all switches to the specified state
void FGElectricalConnector::set_switches( bool state ) {
    // cout << "setting switch state to " << state << endl;
    for ( unsigned int i = 0; i < switches.size(); ++i ) {
        switches[i]->setBoolValue( state );
    }
}


// return true if all switches are true, false otherwise.  A connector
// could have multiple switches, but they all need to be true(closed)
// for current to get through.
bool FGElectricalConnector::get_state() {
    unsigned int i;
    for ( i = 0; i < switches.size(); ++i ) {
        if ( ! switches[i]->getBoolValue() ) {
            return false;
        }
    }

    return true;
}


FGElectricalSystem::FGElectricalSystem () :
    enabled(false)
{
}


FGElectricalSystem::~FGElectricalSystem () {
}


void FGElectricalSystem::init () {
    config_props = new SGPropertyNode;

    SGPropertyNode *path_n = fgGetNode("/sim/systems/electrical/path");
    _volts_out = fgGetNode( "/systems/electrical/volts", true );
    _amps_out = fgGetNode( "/systems/electrical/amps", true );

    if (path_n) {
        SGPath config( globals->get_fg_root() );
        config.append( path_n->getStringValue() );

        SG_LOG( SG_ALL, SG_ALERT, "Reading electrical system model from "
                << config.str() );
        try {
            readProperties( config.str(), config_props );

            if ( build() ) {
                enabled = true;
            } else {
                SG_LOG( SG_ALL, SG_ALERT,
                        "Detected an internal inconsistancy in the electrical");
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

    } else
        SG_LOG( SG_ALL, SG_ALERT,
                "No electrical model specified for this model!");

    delete config_props;
}


void FGElectricalSystem::bind () {
}


void FGElectricalSystem::unbind () {
}


void FGElectricalSystem::update (double dt) {
    if ( !enabled ) {
        _amps_out->setDoubleValue(0);
        return;
    }

    // cout << "Updating electrical system" << endl;

    unsigned int i;

    // zero everything out before we start
    for ( i = 0; i < suppliers.size(); ++i ) {
        suppliers[i]->set_value( 0.0 );
    }
    for ( i = 0; i < buses.size(); ++i ) {
        buses[i]->set_value( 0.0 );
    }
    for ( i = 0; i < outputs.size(); ++i ) {
        outputs[i]->set_value( 0.0 );
    }
    for ( i = 0; i < connectors.size(); ++i ) {
        connectors[i]->set_value( 0.0 );
    }

    // for each supplier, propagate the electrical current
    for ( i = 0; i < suppliers.size(); ++i ) {
        // cout << " Updating: " << suppliers[i]->get_name() << endl;
        propagate( suppliers[i], 0.0, " " );
    }

    double alt_norm
        = fgGetDouble("/systems/electrical/suppliers/alternator") / 60.0;

    // impliment an extremely simplistic voltage model (assumes
    // certain naming conventions in electrical system config)
    double volts = 0.0;
    if ( fgGetBool("/controls/switches/master-bat") ) {
        volts = 24.0;
    }
    if ( fgGetBool("/controls/switches/master-alt") &&
         fgGetDouble("/engines/engine[0]/rpm") > 800 )
    {
        double alt_contrib = 28.0;
        if ( alt_contrib > volts ) {
            volts = alt_contrib;
        }
    }
    _volts_out->setDoubleValue( volts );

    // impliment an extremely simplistic amps model (assumes certain
    // naming conventions in the electrical system config) ... FIXME:
    // make this more generic
    double amps = 0.0;
    if ( fgGetBool("/controls/switches/master-bat") ) {
        if ( fgGetBool("/controls/switches/master-alt") &&
             fgGetDouble("/engines/engine[0]/rpm") > 800 )
        {
            amps += 40.0 * alt_norm;
        }
        amps -= 15.0;            // normal load
        if ( fgGetBool("/controls/switches/flashing-beacon") ) {
            amps -= 7.5;
        }
        if ( fgGetBool("/controls/switches/nav-lights") ) {
            amps -= 7.5;
        }
        if ( amps > 7.0 ) {
            amps = 7.0;
        }
    }
    _amps_out->setDoubleValue( amps );
}


bool FGElectricalSystem::build () {
    SGPropertyNode *node;
    int i;

    int count = config_props->nChildren();
    for ( i = 0; i < count; ++i ) {
        node = config_props->getChild(i);
        string name = node->getName();
        // cout << name << endl;
        if ( name == "supplier" ) {
            FGElectricalSupplier *s =
                new FGElectricalSupplier( node );
            suppliers.push_back( s );
        } else if ( name == "bus" ) {
            FGElectricalBus *b =
                new FGElectricalBus( node );
            buses.push_back( b );
        } else if ( name == "output" ) {
            FGElectricalOutput *o =
                new FGElectricalOutput( node );
            outputs.push_back( o );
        } else if ( name == "connector" ) {
            FGElectricalConnector *c =
                new FGElectricalConnector( node, this );
            connectors.push_back( c );
        } else {
            SG_LOG( SG_ALL, SG_ALERT, "Unknown component type specified: " 
                    << name );
            return false;
        }
    }

    return true;
}


// propagate the electrical current through the network
void FGElectricalSystem::propagate( FGElectricalComponent *node, double val,
                                    string s ) {
    s += " ";

    // determine the current to carry forward
    double current = 0.0;
    if ( !fgGetBool("/systems/electrical/serviceable") ) {
        current = 0;
    } else if ( node->get_kind() == FG_SUPPLIER ) {
        // cout << s << " is a supplier" << endl;
        current = ((FGElectricalSupplier *)node)->get_output();
    } else if ( node->get_kind() == FG_BUS ) {
        // cout << s << " is a bus" << endl;
        current = val;
    } else if ( node->get_kind() == FG_OUTPUT ) {
        // cout << s << " is an output" << endl;
        current = val;
    } else if ( node->get_kind() == FG_CONNECTOR ) {
        // cout << s << " is a connector" << endl;
        if ( ((FGElectricalConnector *)node)->get_state() ) {
            current = val;
        } else {
            current = 0.0;
        }
        // cout << s << "  val = " << current << endl;
    } else {
        SG_LOG( SG_ALL, SG_ALERT, "unkown node type" );
    }

    if ( current > node->get_value() ) {
        node->set_value( current );
    }

    int i;

    // publish values to specified properties
    for ( i = 0; i < node->get_num_props(); ++i ) {
        fgSetDouble( node->get_prop(i).c_str(), node->get_value() );
    }
    // cout << s << node->get_name() << " -> " << node->get_value() << endl;

    // propagate to all children
    for ( i = 0; i < node->get_num_outputs(); ++i ) {
        propagate( node->get_output(i), current, s );
    }
}


// search for the named component and return a pointer to it, NULL otherwise
FGElectricalComponent *FGElectricalSystem::find ( const string &name ) {
    unsigned int i;
    string s;

    // search suppliers
    for ( i = 0; i < suppliers.size(); ++i ) {
        s = suppliers[i]->get_name();
        // cout <<  "    " << s << endl;
        if ( s == name ) {
            return suppliers[i];
        }
    }

    // then search buses
    for ( i = 0; i < buses.size(); ++i ) {
        s = buses[i]->get_name();
        // cout <<  "    " << s << endl;
        if ( s == name ) {
            return buses[i];
        }
    }

    // then search outputs
    for ( i = 0; i < outputs.size(); ++i ) {
        s = outputs[i]->get_name();
        // cout <<  "    " << s << endl;
        if ( s == name ) {
            return outputs[i];
        }
    }

    // nothing found
    return NULL;
}
