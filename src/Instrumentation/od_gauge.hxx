// Owner Drawn Gauge helper class
//
// Written by Harald JOHNSEN, started May 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
//
// Ported to OSG by Tim Moore - Jun 2007
//
// Heavily modified to be usable for the 2d Canvas by Thomas Geymayer - April 2012
// Supports now multisampling/mipmapping, usage of the stencil buffer and placing
// the texture in the scene by certain filter criteria
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

#include <osg/NodeCallback>
#include <osg/Group>

namespace osg {
  class Camera;
  class Texture2D;
}

class SGPropertyNode;
typedef std::vector<osg::ref_ptr<osg::Group> > Placements;

/**
 * Owner Drawn Gauge helper class.
 */
class FGODGauge
{
public:
    FGODGauge();
    virtual ~FGODGauge();

    /**
     * Set the size of the render target.
     *
     * @param size_x    X size
     * @param size_y    Y size. Defaults to size_x if not specified
     */
    void setSize(int size_x, int size_y = -1);

    /**
     * Set the size of the viewport
     *
     * @param width
     * @param height    Defaults to width if not specified
     */
    void setViewSize(int width, int height = -1);

    /**
     * DEPRECATED
     *
     * Get size of squared texture
     */
    int size() const { return _size_x; }
    
    /**
     * Set whether to use image coordinates or not.
     *
     * Default: origin == center of texture
     * Image Coords: origin == top left corner
     */
    void useImageCoords(bool use = true);

    /**
     * Enable/Disable using a stencil buffer
     */
    void useStencil(bool use = true);

    /**
     * Set sampling parameters for mipmapping and coverage sampling
     * antialiasing.
     *
     * @note color_samples is not allowed to be higher than coverage_samples
     *
     */
    void setSampling( bool mipmapping,
                      int coverage_samples = 0,
                      int color_samples = 0 );

    /**
     * Say if we can render to a texture.
     * @return true if rtt is available
     */
    bool serviceable(void);

    /**
     * Replace an opengl texture name inside the aircraft scene graph.
     * This is to replace a static texture by a dynamic one
     * @param name texture filename
     * @param new_texture dynamic texture to replace the old one
     * @return A list of groups which override the given texture
     */
    Placements set_texture(const char * name, osg::Texture2D* new_texture);

    /**
     * Replace an opengl texture name inside the aircraft scene graph.
     * This is to replace a static texture by a dynamic one. The replacement
     * is base on certain filtering criteria which have to be stored in string
     * value childs of the placement node. Recognized nodes are:
     *   - texture  Match the name of the texture
     *   - node     Match the name of the object
     *   - parent   Match any of the object parents names (all the tree upwards)
     * @param placement the node containing the replacement criteria
     * @param new_texture dynamic texture to replace the old one
     * @param an optional cull callback which will be installed on any matching
     *        object
     * @return A list of groups which override the given texture
     */
    Placements set_texture( const SGPropertyNode* placement,
                            osg::Texture2D* new_texture,
                            osg::NodeCallback* cull_callback = 0 );

    /**
     * Get the OSG camera for drawing this gauge.
     */
    osg::Camera* getCamera() { return camera.get(); }

    osg::Texture2D* getTexture() { return texture.get(); }
    //void setTexture(osg::Texture2D* t) { texture = t; }

    // Real initialization function. Bad name.
    void allocRT(osg::NodeCallback* camera_cull_callback = 0);

private:
    int _size_x,
        _size_y,
        _view_width,
        _view_height;
    bool _use_image_coords,
         _use_stencil,
         _use_mipmapping;

    // Multisampling parameters
    int  _coverage_samples,
         _color_samples;

    bool rtAvailable;
    osg::ref_ptr<osg::Camera> camera;
    osg::ref_ptr<osg::Texture2D> texture;

    void updateCoordinateFrame();
    void updateStencil();
    void updateSampling();

};

#endif // _OD_GAUGE_HXX
