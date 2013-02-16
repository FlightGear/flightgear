// environment_ctrl.cxx -- manager for natural environment information.
//
// Written by David Megginson, started February 2002.
// Partly rewritten by Torsten Dreyer, August 2010.
//
// Copyright (C) 2002  David Megginson - david@megginson.com
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <algorithm>

#include <simgear/math/SGMath.hxx>
#include <Main/fg_props.hxx>
#include "environment_ctrl.hxx"
#include "environment.hxx"

namespace Environment {

/**
 * @brief Describes an element of a LayerTable. A defined environment at a given altitude.
*/
struct LayerTableBucket {
    double altitude_ft;
    FGEnvironment environment;
    inline bool operator< (const LayerTableBucket &b) const {
        return (altitude_ft < b.altitude_ft);
    }
    /** 
    * @brief LessThan predicate for bucket pointers.
    */
    static bool lessThan(LayerTableBucket *a, LayerTableBucket *b) {
        return (a->altitude_ft) < (b->altitude_ft);
    }
};

//////////////////////////////////////////////////////////////////////////////

/**
 * @brief Models a column of our atmosphere by stacking a number of environments above
 *        each other
 */
class LayerTable : public std::vector<LayerTableBucket *>, public SGPropertyChangeListener
{
public:
    LayerTable( SGPropertyNode_ptr rootNode ) :
      _rootNode(rootNode) {}

    ~LayerTable();

    /**
     * @brief Read the environment column from properties relative to the given root node
     * @param environment A template environment to copy values from, not given in the configuration
     */
    void read( FGEnvironment * parent = NULL );

    /**
     *@brief Interpolate and write environment values for a given altitude
     *@param altitude_ft The altitude for the desired environment
     *@environment the destination to write the resulting environment properties to
     */
    void interpolate(double altitude_ft, FGEnvironment * environment);

    /**
     *@brief Bind all environments properties to property nodes and initialize the listeners
     */
    void Bind();

    /**
     *@brief Unbind all environments properties from property nodes and deregister listeners
     */
    void Unbind();
private:
    /**
     * @brief Implementation of SGProertyChangeListener::valueChanged()
     *        Takes care of consitent sea level pressure for the entire column
     */
    void valueChanged( SGPropertyNode * node );
    SGPropertyNode_ptr _rootNode;
};

//////////////////////////////////////////////////////////////////////////////


/**
 *@brief Implementation of the LayerIterpolateController
 */
class LayerInterpolateControllerImplementation : public LayerInterpolateController
{
public:
    LayerInterpolateControllerImplementation( SGPropertyNode_ptr rootNode );
    
    virtual void init ();
    virtual void reinit ();
    virtual void postinit();
    virtual void bind();
    virtual void unbind();
    virtual void update (double delta_time_sec);

private:
    SGPropertyNode_ptr _rootNode;
    bool _enabled;
    double _boundary_transition;
    SGPropertyNode_ptr _altitude_n;
    SGPropertyNode_ptr _altitude_agl_n;

    LayerTable _boundary_table;
    LayerTable _aloft_table;

    FGEnvironment _environment;
    simgear::TiedPropertyList _tiedProperties;
};

//////////////////////////////////////////////////////////////////////////////

LayerTable::~LayerTable() 
{
    for( iterator it = begin(); it != end(); it++ )
        delete (*it);
}

void LayerTable::read(FGEnvironment * parent )
{
    double last_altitude_ft = 0.0;
    double sort_required = false;
    size_t i;

    for (i = 0; i < (size_t)_rootNode->nChildren(); i++) {
        const SGPropertyNode * child = _rootNode->getChild(i);
        if ( child->getNameString() == "entry"
         && child->getStringValue("elevation-ft", "")[0] != '\0'
         && ( child->getDoubleValue("elevation-ft") > 0.1 || i == 0 ) )
    {
            LayerTableBucket * b;
            if( i < size() ) {
                // recycle existing bucket
                b = at(i);
            } else {
                // more nodes than buckets in table, add a new one
                b = new LayerTableBucket;
                push_back(b);
            }
            if (i == 0 && parent != NULL )
                b->environment = *parent;
            if (i > 0)
                b->environment = at(i-1)->environment;
            
            b->environment.read(child);
            b->altitude_ft = b->environment.get_elevation_ft();

            // check, if altitudes are in ascending order
            if( b->altitude_ft < last_altitude_ft )
                sort_required = true;
            last_altitude_ft = b->altitude_ft;
        }
    }
    // remove leftover buckets
    while( size() > i ) {
        LayerTableBucket * b = *(end() - 1);
        delete b;
        pop_back();
    }

    if( sort_required )
        sort(begin(), end(), LayerTableBucket::lessThan);

    // cleanup entries with (almost)same altitude
    for( size_type n = 1; n < size(); n++ ) {
        if( fabs(at(n)->altitude_ft - at(n-1)->altitude_ft ) < 1 ) {
            SG_LOG( SG_ENVIRONMENT, SG_ALERT, "Removing duplicate altitude entry in environment config for altitude " << at(n)->altitude_ft );
            erase( begin() + n );
        }
    }
}

void LayerTable::Bind()
{
    // tie all environments to ~/entry[n]/xxx
    // register this as a changelistener of ~/entry[n]/pressure-sea-level-inhg
    for( unsigned i = 0; i < size(); i++ ) {
        SGPropertyNode_ptr baseNode = _rootNode->getChild("entry", i, true );
        at(i)->environment.Tie( baseNode );
        baseNode->getNode( "pressure-sea-level-inhg", true )->addChangeListener( this );
    }
}

void LayerTable::Unbind()
{
    // untie all environments to ~/entry[n]/xxx
    // deregister this as a changelistener of ~/entry[n]/pressure-sea-level-inhg
    for( unsigned i = 0; i < size(); i++ ) {
        SGPropertyNode_ptr baseNode = _rootNode->getChild("entry", i, true );
        at(i)->environment.Untie();
        baseNode->getNode( "pressure-sea-level-inhg", true )->removeChangeListener( this );
    }
}

void LayerTable::valueChanged( SGPropertyNode * node ) 
{
    // Make sure all environments in our column use the same sea level pressure
    double value = node->getDoubleValue();
    for( iterator it = begin(); it != end(); it++ )
        (*it)->environment.set_pressure_sea_level_inhg( value );
}


void LayerTable::interpolate( double altitude_ft, FGEnvironment * result )
{
    int length = size();
    if (length == 0)
        return;

    // Boundary conditions
    if ((length == 1) || (at(0)->altitude_ft >= altitude_ft)) {
        *result = at(0)->environment; // below bottom of table
        return;
    } else if (at(length-1)->altitude_ft <= altitude_ft) {
        *result = at(length-1)->environment; // above top of table
        return;
    } 

    // Search the interpolation table
    int layer;
    for ( layer = 1; // can't be below bottom layer, handled above
          layer < length && at(layer)->altitude_ft <= altitude_ft;
          layer++);
    FGEnvironment & env1 = (at(layer-1)->environment);
    FGEnvironment & env2 = (at(layer)->environment);
    // two layers of same altitude were sorted out in read_table
    double fraction = ((altitude_ft - at(layer-1)->altitude_ft) /
                      (at(layer)->altitude_ft - at(layer-1)->altitude_ft));
    env1.interpolate(env2, fraction, result);
}

//////////////////////////////////////////////////////////////////////////////

LayerInterpolateControllerImplementation::LayerInterpolateControllerImplementation( SGPropertyNode_ptr rootNode ) :
  _rootNode( rootNode ),
  _enabled(true),
  _boundary_transition(0.0),
  _altitude_n( fgGetNode("/position/altitude-ft", true)),
  _altitude_agl_n( fgGetNode("/position/altitude-agl-ft", true)),
  _boundary_table( rootNode->getNode("boundary", true ) ),
  _aloft_table( rootNode->getNode("aloft", true ) )
{
}

void LayerInterpolateControllerImplementation::init ()
{
    _boundary_table.read();
    // pass in a pointer to the environment of the last bondary layer as
    // a starting point
    _aloft_table.read(&(*(_boundary_table.end()-1))->environment);
}

void LayerInterpolateControllerImplementation::reinit ()
{
    _boundary_table.Unbind();
    _aloft_table.Unbind();
    init();
    postinit();
}

void LayerInterpolateControllerImplementation::postinit()
{
    // we get here after 1. bind() and 2. init() was called by fg_init
    _boundary_table.Bind();
    _aloft_table.Bind();
}

void LayerInterpolateControllerImplementation::bind()
{
    // don't bind the layer tables here, because they have not been read in yet.
    _environment.Tie( _rootNode->getNode( "interpolated", true ) );
    _tiedProperties.Tie( _rootNode->getNode("enabled", true), &_enabled );
    _tiedProperties.Tie( _rootNode->getNode("boundary-transition-ft", true ), &_boundary_transition );
}

void LayerInterpolateControllerImplementation::unbind()
{
    _boundary_table.Unbind();
    _aloft_table.Unbind();
    _tiedProperties.Untie();
    _environment.Untie();
}

void LayerInterpolateControllerImplementation::update (double delta_time_sec)
{
    if( !_enabled || delta_time_sec <= SGLimitsd::min() )
        return;

    double altitude_ft = _altitude_n->getDoubleValue();
    double altitude_agl_ft = _altitude_agl_n->getDoubleValue();

    // avoid div by zero later on and init with a default value if not given
    if( _boundary_transition <= SGLimitsd::min() )
        _boundary_transition = 500;

    int length = _boundary_table.size();

    if (length > 0) {
        // If a boundary table is defined, get the top of the boundary layer
        double boundary_limit = _boundary_table[length-1]->altitude_ft;
        if (boundary_limit >= altitude_agl_ft) {
            // If current altitude is below top of boundary layer, interpolate
            // only in boundary layer
            _boundary_table.interpolate(altitude_agl_ft, &_environment);
            return;
        } else if ((boundary_limit + _boundary_transition) >= altitude_agl_ft) {
            // If current altitude is above top of boundary layer and within the 
            // transition altitude, interpolate boundary and aloft layers
            FGEnvironment env1, env2;
            _boundary_table.interpolate( altitude_agl_ft, &env1);
            _aloft_table.interpolate(altitude_ft, &env2);
            double fraction = (altitude_agl_ft - boundary_limit) / _boundary_transition;
            env1.interpolate(env2, fraction, &_environment);
            return;
        }
    } 
    // If no boundary layer is defined or altitude is above top boundary-layer plus boundary-transition
    // altitude, use only the aloft table
    _aloft_table.interpolate( altitude_ft, &_environment);
}

//////////////////////////////////////////////////////////////////////////////

LayerInterpolateController * LayerInterpolateController::createInstance( SGPropertyNode_ptr rootNode )
{
    return new LayerInterpolateControllerImplementation( rootNode );
}

//////////////////////////////////////////////////////////////////////////////

} // namespace
