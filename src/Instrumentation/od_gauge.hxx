// Owner Drawn Gauge helper class
//
// Written by Harald JOHNSEN, started May 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
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

#include <osg/Texture2D>

#include <simgear/structure/subsystem_mgr.hxx>

/**
 * Owner Drawn Gauge helper class.
 */
class FGODGauge : public SGSubsystem {

public:
    FGODGauge ( SGPropertyNode *node );
    FGODGauge();
    ~FGODGauge();
    virtual void init ();
    virtual void update (double dt);

    /**
     * Start the rendering of the RTT context.
     * @param viewSize size of the destination texture 
     */
    void beginCapture(int viewSize);
    /**
     * Start the rendering of the RTT context.
     */
    void beginCapture(void);
    /**
     * Clear the background.
     */
    void Clear(void);
    /**
     * Finish rendering and save the buffer to a texture.
     * @param texID name of a gl texture
     */
    void endCapture(osg::Texture2D*);
    /**
     * Set the size of the destination texture.
     * @param viewSize size of the destination texture 
     */
    void setSize(int viewSize);
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
     */
    void set_texture(const char * name, osg::Texture2D* new_texture);

private:
    int textureWH;
    bool rtAvailable;

    void allocRT(void);
    void set2D(void);
    void set3D(void);
};

#endif // _OD_GAUGE_HXX
