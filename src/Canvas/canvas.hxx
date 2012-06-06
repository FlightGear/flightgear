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

#include <Instrumentation/od_gauge.hxx>
#include <simgear/props/props.hxx>
#include <osg/NodeCallback>

#include <memory>
#include <string>

namespace canvas
{
  class Group;
}

class Canvas:
  public SGPropertyChangeListener
{
  public:

    enum StatusFlags
    {
      STATUS_OK,
      MISSING_SIZE_X = 0x0001,
      MISSING_SIZE_Y = 0x0002,
      CREATE_FAILED  = 0x0004
    };

    Canvas();
    virtual ~Canvas();

    void reset(SGPropertyNode* node);
    void update(double delta_time_sec);

    void setSizeX(int sx);
    int getSizeX() const;

    void setSizeY(int sy);
    int getSizeY() const;

    void setViewWidth(int w);
    int getViewWidth() const;

    void setViewHeight(int h);
    int getViewHeight() const;

    int getStatus() const;
    const char* getStatusMsg() const;

    virtual void childAdded( SGPropertyNode * parent,
                             SGPropertyNode * child );
    virtual void childRemoved( SGPropertyNode * parent,
                               SGPropertyNode * child );
    virtual void valueChanged (SGPropertyNode * node);

  private:

    Canvas(const Canvas&); // = delete;
    Canvas& operator=(const Canvas&); // = delete;

    int _size_x,
        _size_y,
        _view_width,
        _view_height;

    int         _status;
    std::string _status_msg;

    bool _sampling_dirty,
         _color_dirty;

    FGODGauge _texture;
    std::auto_ptr<canvas::Group> _root_group;

    SGPropertyNode_ptr              _node;
    std::vector<SGPropertyNode_ptr> _color_background;

    osg::ref_ptr<osg::NodeCallback> _camera_callback;
    osg::ref_ptr<osg::NodeCallback> _cull_callback;
    std::vector<SGPropertyNode*> _dirty_placements;
    std::vector<Placements> _placements;

    void setStatusFlags(unsigned int flags, bool set = true);
    void clearPlacements(int index);
    void clearPlacements();

    void unbind();
};

#endif /* CANVAS_HXX_ */
