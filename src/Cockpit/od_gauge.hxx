// Owner Drawn Gauge helper class
//
// Moved to SimGear by Thomas Geymayer - October 2012
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
//

#ifndef _OD_GAUGE_HXX
#define _OD_GAUGE_HXX

#include <simgear/canvas/ODGauge.hxx>
#include <simgear/canvas/CanvasPlacement.hxx>

class SGPropertyNode;

/**
 * Owner Drawn Gauge helper class
 */
class FGODGauge:
  public simgear::canvas::ODGauge
{
  public:
    FGODGauge();
    virtual ~FGODGauge();

    /**
     * Replace an opengl texture name inside a given branch of the scene graph.
     * This is to replace a static texture by a dynamic one
     *
     * @param branch        Scene graph branch to use for search
     * @param name          texture filename
     * @param new_texture   dynamic texture to replace the old one
     * @return A list of groups which override the given texture
     */
    static
    simgear::canvas::Placements set_texture( osg::Node* branch,
                                             const char * name,
                                             osg::Texture2D* new_texture );

    /**
     * Replace an opengl texture name inside the aircraft scene graph.
     * This is to replace a static texture by a dynamic one
     *
     * @param branch        Scene graph branch to search for matching
     * @param name          texture filename
     * @param new_texture   dynamic texture to replace the old one
     * @return A list of groups which override the given texture
     */
    static
    simgear::canvas::Placements
    set_aircraft_texture( const char * name,
                          osg::Texture2D* new_texture );

    /**
     * Replace an opengl texture name inside a given branch of the scene graph.
     * This is to replace a static texture by a dynamic one. The replacement
     * is base on certain filtering criteria which have to be stored in string
     * value childs of the placement node. Recognized nodes are:
     *   - texture  Match the name of the texture
     *   - node     Match the name of the object
     *   - parent   Match any of the object parents names (all the tree upwards)
     *
     * @param placement the node containing the replacement criteria
     * @param new_texture dynamic texture to replace the old one
     * @param an optional cull callback which will be installed on any matching
     *        object
     * @return A list of groups which override the given texture
     */
    static
    simgear::canvas::Placements
    set_texture( osg::Node* branch,
                 SGPropertyNode* placement,
                 osg::Texture2D* new_texture,
                 osg::NodeCallback* cull_callback = 0,
                 const simgear::canvas::CanvasWeakPtr& canvas =
                   simgear::canvas::CanvasWeakPtr() );

    /**
     * Replace an opengl texture name inside the aircraft scene graph.
     *
     * @param placement the node containing the replacement criteria
     * @param new_texture dynamic texture to replace the old one
     * @param an optional cull callback which will be installed on any matching
     *        object
     * @return A list of groups which override the given texture
     */
    static
    simgear::canvas::Placements
    set_aircraft_texture( SGPropertyNode* placement,
                          osg::Texture2D* new_texture,
                          osg::NodeCallback* cull_callback = 0,
                          const simgear::canvas::CanvasWeakPtr& canvas =
                            simgear::canvas::CanvasWeakPtr() );

};

#endif // _OD_GAUGE_HXX
