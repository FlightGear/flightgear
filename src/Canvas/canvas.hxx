// The canvas for rendering with the 2d api
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef CANVAS_HXX_
#define CANVAS_HXX_

#include "placement.hxx"
#include "property_based_element.hxx"

#include <Canvas/canvas_fwd.hpp>
#include <Instrumentation/od_gauge.hxx>

#include <simgear/props/propertyObject.hxx>
#include <osg/NodeCallback>

#include <memory>
#include <string>

class Canvas:
  public PropertyBasedElement
{
  public:

    enum StatusFlags
    {
      STATUS_OK,
      MISSING_SIZE_X = 0x0001,
      MISSING_SIZE_Y = 0x0002,
      CREATE_FAILED  = 0x0004
    };

    /**
     * Callback used to disable/enable rendering to the texture if it is not
     * visible
     */
    class CameraCullCallback:
      public osg::NodeCallback
    {
      public:
        CameraCullCallback();

        /**
         * Enable rendering for the next frame
         */
        void enableRendering();

      private:
        bool _render;
        unsigned int _render_frame;

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
    };
    typedef osg::ref_ptr<CameraCullCallback> CameraCullCallbackPtr;

    /**
     * This callback is installed on every placement of the canvas in the
     * scene to only render the canvas if at least one placement is visible
     */
    class CullCallback:
      public osg::NodeCallback
    {
      public:
        CullCallback(CameraCullCallback* camera_cull);

      private:
        CameraCullCallback *_camera_cull;

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
    };
    typedef osg::ref_ptr<CullCallback> CullCallbackPtr;

    Canvas(SGPropertyNode* node);
    virtual ~Canvas();

    void update(double delta_time_sec);

    int getSizeX() const
    { return _size_x; }
  
    int getSizeY() const
    { return _size_y; }
  
    void setSizeX(int sx);
    void setSizeY(int sy);

    void setViewWidth(int w);
    void setViewHeight(int h);

    bool handleMouseEvent(const canvas::MouseEvent& event);

    virtual void childAdded( SGPropertyNode * parent,
                             SGPropertyNode * child );
    virtual void childRemoved( SGPropertyNode * parent,
                               SGPropertyNode * child );
    virtual void valueChanged (SGPropertyNode * node);

    osg::Texture2D* getTexture() const;
    GLuint getTexId() const;

    CameraCullCallbackPtr getCameraCullCallback() const;
    CullCallbackPtr getCullCallback() const;

    static void addPlacementFactory( const std::string& type,
                                     canvas::PlacementFactory factory );

  private:

    Canvas(const Canvas&); // = delete;
    Canvas& operator=(const Canvas&); // = delete;

    int _size_x,
        _size_y,
        _view_width,
        _view_height;

    simgear::PropertyObject<int>            _status;
    simgear::PropertyObject<std::string>    _status_msg;

    simgear::PropertyObject<int>    _mouse_x, _mouse_y,
                                    _mouse_dx, _mouse_dy,
                                    _mouse_button,
                                    _mouse_state,
                                    _mouse_mod,
                                    _mouse_scroll,
                                    _mouse_event;

    bool _sampling_dirty,
         _color_dirty;

    FGODGauge _texture;
    std::auto_ptr<canvas::Group> _root_group;

    std::vector<SGPropertyNode_ptr> _color_background;

    CameraCullCallbackPtr _camera_callback;
    CullCallbackPtr _cull_callback;
    bool _render_always; //<! Used to disable automatic lazy rendering (culling)

    std::vector<SGPropertyNode*> _dirty_placements;
    std::vector<canvas::Placements> _placements;

    typedef std::map<std::string, canvas::PlacementFactory> PlacementFactoryMap;
    static PlacementFactoryMap _placement_factories;

    void setStatusFlags(unsigned int flags, bool set = true);
};

#endif /* CANVAS_HXX_ */
