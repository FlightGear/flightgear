// electrical.cxx - a flexible, generic electrical system model.
//
// Written by Curtis Olson, started September 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <cstdlib>
#include <cstring>
#include <algorithm>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "electrical.hxx"


FGElectricalComponent::FGElectricalComponent() :
    kind(-1),
    name(""),
    volts(0.0),
    load_amps(0.0)
{
}

void FGElectricalComponent::add_prop(const std::string &s)
{
    auto nd = fgGetNode(s, true);
    props.push_back(nd);
}

void FGElectricalComponent::publishVoltageToProps() const
{
    const auto v = get_volts();
    for (const auto& nd : props) {
        nd->setFloatValue(v);
    }
}

FGElectricalSupplier::FGElectricalSupplier ( SGPropertyNode *node ) {
    kind = FG_SUPPLIER;

    // cout << "Creating a supplier" << endl;
    name = node->getStringValue("name");
    string _model = node->getStringValue("kind");
    // cout << "_model = " << _model << endl;
    if ( _model == "battery" ) {
        model = FG_BATTERY;
        amp_hours = node->getFloatValue("amp-hours", 40.0);
        percent_remaining = node->getFloatValue("percent-remaining", 1.0);
        charge_amps = node->getFloatValue("charge-amps", 7.0);
    } else if ( _model == "alternator" ) {
        model = FG_ALTERNATOR;
        rpm_src = node->getStringValue("rpm-source");
        rpm_threshold = node->getFloatValue("rpm-threshold", 600.0);
        ideal_amps = node->getFloatValue("amps", 60.0);
    } else if ( _model == "external" ) {
        model = FG_EXTERNAL;
        ideal_amps = node->getFloatValue("amps", 60.0);
    } else {
        model = FG_UNKNOWN;
    }
    ideal_volts = node->getFloatValue("volts");

    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        // cout << " scanning: " << child->getName() << endl;
        if ( !strcmp(child->getName(), "prop") ) {
            string prop = child->getStringValue();
            // cout << "  Adding prop = " << prop << endl;
            add_prop( prop );
            fgSetFloat( prop.c_str(), ideal_amps );
        }
    }

    _rpm_node = fgGetNode( rpm_src.c_str(), true);
}


float FGElectricalSupplier::apply_load( float amps, float dt ) {
    if ( model == FG_BATTERY ) {
        // calculate amp hours used
        float amphrs_used = amps * dt / 3600.0;

        // calculate percent of total available capacity
        float percent_used = amphrs_used / amp_hours;
        percent_remaining -= percent_used;
        if ( percent_remaining < 0.0 ) {
            percent_remaining = 0.0;
        } else if ( percent_remaining > 1.0 ) {
            percent_remaining = 1.0;
        }
        // cout << "battery percent = " << percent_remaining << endl;
        return amp_hours * percent_remaining;
    } else if ( model == FG_ALTERNATOR ) {
        // scale alternator output for rpms < 600.  For rpms >= 600
        // give full output.  This is just a WAG, and probably not how
        // it really works but I'm keeping things "simple" to start.
        float rpm = _rpm_node->getFloatValue();
        float factor = rpm / rpm_threshold;
        if ( factor > 1.0 ) {
            factor = 1.0;
        }
        // cout << "alternator amps = " << amps * factor << endl;
        float available_amps = ideal_amps * factor;
        return available_amps - amps;
    } else if ( model == FG_EXTERNAL ) {
        // cout << "external amps = " << 0.0 << endl;
        float available_amps = ideal_amps;
        return available_amps - amps;
    } else {
        SG_LOG( SG_SYSTEMS, SG_ALERT, "unknown supplier type" );
    }

    return 0.0;
}


float FGElectricalSupplier::get_output_volts() {
    if ( model == FG_BATTERY ) {
        // cout << "battery amps = " << amps << endl;
        float x = 1.0 - percent_remaining;
        float tmp = -(3.0 * x - 1.0);
        float factor = (tmp*tmp*tmp*tmp*tmp + 32) / 32;
        // cout << "battery % = " << percent_remaining <<
        //         " factor = " << factor << endl;
        // percent_remaining -= 0.001;
        return ideal_volts * factor;
    } else if ( model == FG_ALTERNATOR ) {
        // scale alternator output for rpms < 600.  For rpms >= 600
        // give full output.  This is just a WAG, and probably not how
        // it really works but I'm keeping things "simple" to start.
        float rpm = _rpm_node->getFloatValue();
        float factor = rpm / rpm_threshold;
        if ( factor > 1.0 ) {
            factor = 1.0;
        }
        // cout << "alternator amps = " << amps * factor << endl;
        return ideal_volts * factor;
    } else if ( model == FG_EXTERNAL ) {
        // cout << "external amps = " << 0.0 << endl;
        return ideal_volts;
    } else {
        SG_LOG( SG_SYSTEMS, SG_ALERT, "unknown supplier type" );
    }

    return 0.0;
}


float FGElectricalSupplier::get_output_amps() {
    if ( model == FG_BATTERY ) {
        // cout << "battery amp_hours = " << amp_hours << endl;

        // This is a WAG, but produce enough amps to burn the entire
        // battery in one minute.
        return amp_hours * 60.0;
    } else if ( model == FG_ALTERNATOR ) {
        // scale alternator output for rpms < 600.  For rpms >= 600
        // give full output.  This is just a WAG, and probably not how
        // it really works but I'm keeping things "simple" to start.
        float rpm = _rpm_node->getFloatValue();
        float factor = rpm / rpm_threshold;
        if ( factor > 1.0 ) {
            factor = 1.0;
        }
        // cout << "alternator amps = " << ideal_amps * factor << endl;
        return ideal_amps * factor;
    } else if ( model == FG_EXTERNAL ) {
        // cout << "external amps = " << 0.0 << endl;
        return ideal_amps;
    } else {
        SG_LOG( SG_SYSTEMS, SG_ALERT, "unknown supplier type" );
    }

    return 0.0;
}


FGElectricalBus::FGElectricalBus ( SGPropertyNode *node ) {
    kind = FG_BUS;

    name = node->getStringValue("name");
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        if ( !strcmp(child->getName(), "prop") ) {
            string prop = child->getStringValue();
            add_prop( prop );
        }
    }
}


FGElectricalOutput::FGElectricalOutput ( SGPropertyNode *node ) {
    kind = FG_OUTPUT;
    load_amps = 0.1;            // arbitrary default value

    name = node->getStringValue("name");
    SGPropertyNode *draw = node->getNode("rated-draw");
    if ( draw != NULL ) {
        load_amps = draw->getFloatValue();
    }
    // cout << "rated draw = " << output_amps << endl;

    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        if ( !strcmp(child->getName(), "prop") ) {
            string prop = child->getStringValue();
            add_prop( prop );
        }
    }
}


FGElectricalSwitch::FGElectricalSwitch( SGPropertyNode *node ) :
    switch_node( NULL ),
    rating_amps( 0.0f ),
    circuit_breaker( false )
{
    bool initial_state = true;
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "prop" ) {
            switch_node = fgGetNode( cval.c_str(), true );
            // cout << "switch node = " << cval << endl;
        } else if ( cname == "initial-state" ) {
            if ( cval == "off" || cval == "false" ) {
                initial_state = false;
            }
            // cout << "initial state = " << initial_state << endl;
        } else if ( cname == "rating-amps" ) {
            rating_amps = atof( cval.c_str() );
            circuit_breaker = true;
            // cout << "initial state = " << initial_state << endl;
        }
    }

    switch_node->setBoolValue( initial_state );
    // cout << "  value = " << switch_node->getBoolValue() << endl;
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
                    SG_LOG( SG_SYSTEMS, SG_ALERT,
                            "Attempt to connect to something that can't provide an output: "
                            << child->getStringValue() );
                }
            } else {
                SG_LOG( SG_SYSTEMS, SG_ALERT, "Can't find named source: "
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
                } else if ( s->get_kind() == FG_SUPPLIER &&
                            ((FGElectricalSupplier *)s)->get_model()
                            == FGElectricalSupplier::FG_BATTERY ) {
                    s->add_output( this );
                } else {
                    SG_LOG( SG_SYSTEMS, SG_ALERT,
                            "Attempt to connect to something that can't provide an input: "
                            << child->getStringValue() );
                }
            } else {
                SG_LOG( SG_SYSTEMS, SG_ALERT, "Can't find named source: "
                        << child->getStringValue() );
            }
        } else if ( cname == "switch" ) {
             // cout << "Switch = " << child->getStringValue() << endl;
            FGElectricalSwitch s( child );
            add_switch( s );
        }
    }
}


// set all switches to the specified state
void FGElectricalConnector::set_switches( bool state ) {
    // cout << "setting switch state to " << state << endl;
    for ( unsigned int i = 0; i < switches.size(); ++i ) {
        switches[i].set_state( state );
    }
}


// return true if all switches are true, false otherwise.  A connector
// could have multiple switches, but they all need to be true(closed)
// for current to get through.
bool FGElectricalConnector::get_state() {
    unsigned int i;
    for ( i = 0; i < switches.size(); ++i ) {
        if ( ! switches[i].get_state() ) {
            return false;
        }
    }

    return true;
}


FGElectricalSystem::FGElectricalSystem ( SGPropertyNode *node ) :
    name(node->getStringValue("name", "electrical")),
    num(node->getIntValue("number", 0)),
    path(node->getStringValue("path")),
    enabled(false)
{
}


FGElectricalSystem::~FGElectricalSystem()
{
    SG_LOG(SG_SYSTEMS, SG_INFO, "Destroying elec system");
}


void FGElectricalSystem::init () {
    SGPropertyNode_ptr config_props = new SGPropertyNode;

    _volts_out = fgGetNode( "/systems/electrical/volts", true );
    _amps_out = fgGetNode( "/systems/electrical/amps", true );

    // allow the electrical system to be specified via the
    // aircraft-set.xml file (for backwards compatibility) or through
    // the aircraft-systems.xml file.  If a -set.xml entry is
    // specified, that overrides the system entry.
    SGPropertyNode *path_n = fgGetNode("/sim/systems/electrical/path");
    if ( path_n ) {
        if ( path.length() ) {
            SG_LOG( SG_SYSTEMS, SG_INFO,
                    "NOTICE: System manager configuration specifies an " <<
                    "electrical system: " << path << " but it is " <<
                    "being overridden by the one specified in the -set.xml " <<
                    "file: " << path_n->getStringValue() );
        }

        path = path_n->getStringValue();
    }

    if ( path.length() ) {
        SGPath config = globals->resolve_aircraft_path(path);
        if (!config.exists()) {
            SG_LOG( SG_SYSTEMS, SG_ALERT,  "Failed to find electrical system model: " << config );
            return;
        }

        // load an obsolete xml configuration
        SG_LOG( SG_SYSTEMS, SG_DEV_WARN,
                "Reading deprecated xml electrical system model from\n    "
                << config.str() );
        try {
            readProperties( config, config_props );

            if ( build(config_props) ) {
                enabled = true;
            } else {
                throw sg_exception("Logic error in electrical system file.");
            }
        } catch (const sg_exception&) {
            SG_LOG( SG_SYSTEMS, SG_ALERT,
                    "Failed to load electrical system model: "
                    << config );
        }
    } else {
        SG_LOG( SG_SYSTEMS, SG_INFO,
                "No xml-based electrical model specified for this model!");
    }

    if ( !enabled ) {
        _amps_out->setDoubleValue(0);
    }

}


void FGElectricalSystem::bind ()
{
    _serviceable_node = fgGetNode("/systems/electrical/serviceable", true);
}


void FGElectricalSystem::unbind ()
{
    _serviceable_node.reset();
    _volts_out.reset();
    _amps_out.reset();
}

void FGElectricalSystem::deleteComponents(comp_list& comps)
{
    std::for_each(comps.begin(), comps.end(),
                  [](FGElectricalComponent* comp) {
                      delete comp;
                  });

    comps.clear();
}

void FGElectricalSystem::shutdown()
{
    deleteComponents(suppliers);
    deleteComponents(buses);
    deleteComponents(outputs);
    deleteComponents(connectors);
}

void FGElectricalSystem::update (double dt)
{
    if ( !enabled ) {
        return;
    }

    // cout << "Updating electrical system, dt = " << dt << endl;
    _serviceable = _serviceable_node->getBoolValue();
    
    unsigned int i;

    // zero out the voltage before we start, but don't clear the
    // requested load values.
    for ( i = 0; i < suppliers.size(); ++i ) {
        suppliers[i]->set_volts( 0.0 );
    }
    for ( i = 0; i < buses.size(); ++i ) {
        buses[i]->set_volts( 0.0 );
    }
    for ( i = 0; i < outputs.size(); ++i ) {
        outputs[i]->set_volts( 0.0 );
    }
    for ( i = 0; i < connectors.size(); ++i ) {
        connectors[i]->set_volts( 0.0 );
    }

    // for each "external" supplier, propagate the electrical current
    for ( i = 0; i < suppliers.size(); ++i ) {
        FGElectricalSupplier *node = (FGElectricalSupplier *)suppliers[i];
        if ( node->get_model() == FGElectricalSupplier::FG_EXTERNAL ) {
            float load;
            // cout << "Starting propagation: " << suppliers[i]->get_name()
            //      << endl;
            load = propagate( suppliers[i], dt,
                              node->get_output_volts(),
                              node->get_output_amps(),
                              " " );

            if ( node->apply_load( load, dt ) < 0.0 ) {
                SG_LOG(SG_SYSTEMS, SG_ALERT,
                       "Error drawing more current than available!");
            }
        }
    }

    // for each "alternator" supplier, propagate the electrical
    // current
    for ( i = 0; i < suppliers.size(); ++i ) {
        FGElectricalSupplier *node = (FGElectricalSupplier *)suppliers[i];
        if ( node->get_model() == FGElectricalSupplier::FG_ALTERNATOR) {
            float load;
            // cout << "Starting propagation: " << suppliers[i]->get_name()
            //      << endl;
            load = propagate( suppliers[i], dt,
                              node->get_output_volts(),
                              node->get_output_amps(),
                              " " );

            if ( node->apply_load( load, dt ) < 0.0 ) {
                SG_LOG(SG_SYSTEMS, SG_ALERT,
                       "Error drawing more current than available!");
            }
        }
    }

    // for each "battery" supplier, propagate the electrical
    // current
    for ( i = 0; i < suppliers.size(); ++i ) {
        FGElectricalSupplier *node = (FGElectricalSupplier *)suppliers[i];
        if ( node->get_model() == FGElectricalSupplier::FG_BATTERY ) {
            float load;
            // cout << "Starting propagation: " << suppliers[i]->get_name()
            //      << endl;
            load = propagate( suppliers[i], dt,
                              node->get_output_volts(),
                              node->get_output_amps(),
                              " " );
            // cout << "battery load = " << load << endl;

            if ( node->apply_load( load, dt ) < 0.0 ) {
                SG_LOG(SG_SYSTEMS, SG_ALERT,
                       "Error drawing more current than available!");
            }
        }
    }

    float alt_norm
        = fgGetFloat("/systems/electrical/suppliers/alternator") / 60.0;

    // impliment an extremely simplistic voltage model (assumes
    // certain naming conventions in electrical system config)
    // FIXME: we probably want to be able to feed power from all
    // engines if they are running and the master-alt is switched on
    float volts = 0.0;
    if ( fgGetBool("/controls/engines/engine[0]/master-bat") ) {
        volts = 24.0;
    }
    if ( fgGetBool("/controls/engines/engine[0]/master-alt") ) {
        if ( fgGetFloat("/engines/engine[0]/rpm") > 800 ) {
            float alt_contrib = 28.0;
            if ( alt_contrib > volts ) {
                volts = alt_contrib;
            }
        } else if ( fgGetFloat("/engines/engine[0]/rpm") > 200 ) {
            float alt_contrib = 20.0;
            if ( alt_contrib > volts ) {
                volts = alt_contrib;
            }
        }
    }
    _volts_out->setFloatValue( volts );

    // impliment an extremely simplistic amps model (assumes certain
    // naming conventions in the electrical system config) ... FIXME:
    // make this more generic
    float amps = 0.0;
    if ( fgGetBool("/controls/engines/engine[0]/master-bat") ) {
        if ( fgGetBool("/controls/engines/engine[0]/master-alt") &&
             fgGetFloat("/engines/engine[0]/rpm") > 800 )
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
    _amps_out->setFloatValue( amps );
}


bool FGElectricalSystem::build (SGPropertyNode* config_props) {
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
            SG_LOG( SG_SYSTEMS, SG_ALERT, "Unknown component type specified: "
                    << name );
            return false;
        }
    }

    return true;
}


// propagate the electrical current through the network, returns the
// total current drawn by the children of this node.
float FGElectricalSystem::propagate( FGElectricalComponent *node, double dt,
                                     float input_volts, float input_amps,
                                     string s ) {
    s += " ";

    float total_load = 0.0;

    // determine the current to carry forward
    float volts = 0.0;
    if ( !_serviceable) {
        volts = 0;
    } else if ( node->get_kind() == FGElectricalComponent::FG_SUPPLIER ) {
        // cout << s << "is a supplier (" << node->get_name() << ")" << endl;
        FGElectricalSupplier *supplier = (FGElectricalSupplier *)node;
        if ( supplier->get_model() == FGElectricalSupplier::FG_BATTERY ) {
            // cout << s << " (and is a battery)" << endl;
            float battery_volts = supplier->get_output_volts();
            if ( battery_volts < (input_volts - 0.1) ) {
                // special handling of a battery charge condition
                // cout << s << "  (and is being charged) in v = "
                //      << input_volts << " current v = " << battery_volts
                //      << endl;
                supplier->apply_load( -supplier->get_charge_amps(), dt );
                return supplier->get_charge_amps();
            }
        }
        volts = input_volts;
    } else if ( node->get_kind() == FGElectricalComponent::FG_BUS ) {
        // cout << s << "is a bus (" << node->get_name() << ")" << endl;
        volts = input_volts;
    } else if ( node->get_kind() == FGElectricalComponent::FG_OUTPUT ) {
        // cout << s << "is an output (" << node->get_name() << ")" << endl;
        volts = input_volts;
        if ( volts > 1.0 ) {
            // draw current if we have voltage
            total_load = node->get_load_amps();
        }
    } else if ( node->get_kind() == FGElectricalComponent::FG_CONNECTOR ) {
        // cout << s << "is a connector (" << node->get_name() << ")" << endl;
        if ( ((FGElectricalConnector *)node)->get_state() ) {
            volts = input_volts;
        } else {
            volts = 0.0;
        }
        // cout << s << "  input_volts = " << volts << endl;
    } else {
        SG_LOG( SG_SYSTEMS, SG_ALERT, "unknown node type" );
    }

    int i;

    // if this node has found a stronger power source, update the
    // value and propagate to all children
    if ( volts > node->get_volts() ) {
        node->set_volts( volts );
        for ( i = 0; i < node->get_num_outputs(); ++i ) {
            FGElectricalComponent *child = node->get_output(i);
            // send current equal to load
            total_load += propagate( child, dt,
                                     volts, child->get_load_amps(),
                                     s );
        }

        // if not an output node, register the downstream current draw
        // (sum of all children) with this node.  If volts are zero,
        // current draw should be zero.
        if ( node->get_kind() != FGElectricalComponent::FG_OUTPUT ) {
            node->set_load_amps( total_load );
        }

        node->set_available_amps( input_amps - total_load );

        node->publishVoltageToProps();

        /*
        cout << s << node->get_name() << " -> (volts) " << node->get_volts()
             << endl;
        cout << s << node->get_name() << " -> (load amps) " << total_load
             << endl;
        cout << s << node->get_name() << " -> (input amps) " << input_amps
             << endl;
        cout << s << node->get_name() << " -> (extra amps) "
             << node->get_available_amps() << endl;
        */

        return total_load;
    } else {
        // cout << s << "no further propagation" << endl;
        return 0.0;
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


// Register the subsystem.
#if 0
SGSubsystemMgr::Registrant<FGElectricalSystem> registrantFGElectricalSystem;
#endif
