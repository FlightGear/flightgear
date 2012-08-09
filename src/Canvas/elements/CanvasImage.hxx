// An image on the canvas
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

#ifndef CANVAS_IMAGE_HXX_
#define CANVAS_IMAGE_HXX_

#include "element.hxx"
#include <Canvas/canvas_fwd.hpp>
#include <Canvas/rect.hxx>

#include <osg/Texture2D>

namespace canvas
{

  class Image:
    public Element
  {
    public:
      /**
       * @param node    Property node containing settings for this image:
       *                  rect/[left/right/top/bottom]  Dimensions of source
       *                                                rect
       *                  size[0-1]                     Dimensions of rectangle
       *                  [x,y]                         Position of rectangle
       */
      Image(SGPropertyNode_ptr node);
      ~Image();

      virtual void update(double dt);

      void setCanvas(CanvasPtr canvas);
      CanvasWeakPtr getCanvas() const;

      void setImage(osg::Image *img);

      const Rect<float>& getRegion() const;

      /**
       * Callback for every changed child node
       */
      virtual void valueChanged(SGPropertyNode *node);

    protected:

      enum ImageAttributes
      {
        SRC_RECT       = LAST_ATTRIBUTE << 1, // Source image rectangle
        DEST_SIZE      = SRC_RECT << 1        // Element size
      };

      /**
       * Callback for changed direct child nodes
       */
      virtual void childChanged(SGPropertyNode * child);
      virtual void colorFillChanged(const osg::Vec4& color);

      void handleHit(float x, float y);

      osg::ref_ptr<osg::Texture2D> _texture;
      // TODO optionally forward events to canvas
      CanvasWeakPtr _canvas;

      osg::Geometry *_geom;
      osg::Vec3Array *_vertices;
      osg::Vec2Array *_texCoords;
      osg::Vec4Array* _colors;

      SGPropertyNode *_node_src_rect;
      Rect<float>     _src_rect,
                      _region;
  };

}  // namespace canvas

#endif /* CANVAS_IMAGE_HXX_ */
