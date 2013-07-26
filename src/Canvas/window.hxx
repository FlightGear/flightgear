// Window for placing a Canvas onto it (for dialogs, menus, etc.)
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

#ifndef CANVAS_WINDOW_HXX_
#define CANVAS_WINDOW_HXX_

#include <simgear/canvas/elements/CanvasImage.hxx>
#include <simgear/canvas/MouseEvent.hxx>
#include <simgear/props/PropertyBasedElement.hxx>
#include <simgear/props/propertyObject.hxx>
#include <simgear/misc/CSSBorder.hxx>

#include <osg/Geode>
#include <osg/Geometry>

namespace canvas
{
  class Window:
    public simgear::canvas::Image
  {
    public:
      static const std::string TYPE_NAME;

      enum Resize
      {
        NONE    = 0,
        LEFT    = 1,
        RIGHT   = LEFT << 1,
        TOP     = RIGHT << 1,
        BOTTOM  = TOP << 1,
        INIT    = BOTTOM << 1
      };

      typedef simgear::canvas::Style Style;

      Window( const simgear::canvas::CanvasWeakPtr& canvas,
              const SGPropertyNode_ptr& node,
              const Style& parent_style = Style(),
              Element* parent = 0 );
      virtual ~Window();

      virtual void update(double delta_time_sec);
      virtual void valueChanged(SGPropertyNode* node);

      osg::Group* getGroup();
      const SGVec2<float> getPosition() const;
      const SGRect<float>  getScreenRegion() const;

      void setCanvasContent(simgear::canvas::CanvasPtr canvas);
      simgear::canvas::CanvasWeakPtr getCanvasContent() const;

      simgear::canvas::CanvasPtr getCanvasDecoration();

      bool isResizable() const;
      bool isCapturingEvents() const;

      /**
       * Moves window on top of all other windows with the same z-index.
       *
       * @note If no z-index is set it defaults to 0.
       */
      void raise();

      void handleResize( uint8_t mode,
                         const osg::Vec2f& offset = osg::Vec2f() );

    protected:

      enum Attributes
      {
        DECORATION = 1
      };

      uint32_t  _attributes_dirty;

      simgear::canvas::CanvasPtr        _canvas_decoration;
      simgear::canvas::CanvasWeakPtr    _canvas_content;

      simgear::canvas::ImagePtr _image_content,
                                _image_shadow;

      bool _resizable,
           _capture_events;

      simgear::PropertyObject<int> _resize_top,
                                   _resize_right,
                                   _resize_bottom,
                                   _resize_left,
                                   _resize_status;

      simgear::CSSBorder    _decoration_border;

      void parseDecorationBorder(const std::string& str);
      void updateDecoration();
  };
} // namespace canvas

#endif /* CANVAS_WINDOW_HXX_ */
