// Interface for 2D canvas element
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

#ifndef CANVAS_ELEMENT_HXX_
#define CANVAS_ELEMENT_HXX_

#include <simgear/props/props.hxx>
#include <osg/MatrixTransform>
#include <simgear/misc/stdint.hxx> // for uint32_t

namespace osg
{
  class Drawable;
}

namespace canvas
{

  class Element:
    public SGPropertyChangeListener
  {
    public:
      virtual ~Element() = 0;

      /**
       * Called every frame to update internal state
       *
       * @param dt  Frame time in seconds
       */
      virtual void update(double dt);

      osg::ref_ptr<osg::MatrixTransform> getMatrixTransform();

      virtual void childAdded( SGPropertyNode * parent,
                               SGPropertyNode * child );
      virtual void childRemoved( SGPropertyNode * parent,
                                 SGPropertyNode * child );
      virtual void valueChanged(SGPropertyNode * child);

    protected:

      enum Attributes
      {
        COLOR           = 0x0001,
        COLOR_FILL      = 0x0002,
        BOUNDING_BOX    = 0x0004,
        LAST_ATTRIBUTE  = BOUNDING_BOX
      };

      enum TransformType
      {
        TT_NONE,
        TT_MATRIX,
        TT_TRANSLATE,
        TT_ROTATE,
        TT_SCALE
      };

      uint32_t _attributes_used;
      uint32_t _attributes_dirty;

      bool _transform_dirty;
      osg::ref_ptr<osg::MatrixTransform>    _transform;
      std::vector<TransformType>            _transform_types;

      SGPropertyNode_ptr                _node;
      std::vector<SGPropertyNode_ptr>   _color,
                                        _color_fill;
      std::vector<SGPropertyNode_ptr>   _bounding_box;

      Element(SGPropertyNode_ptr node, uint32_t attributes_used = 0);

      virtual void childAdded(SGPropertyNode * child)  {}
      virtual void childRemoved(SGPropertyNode * child){}
      virtual void childChanged(SGPropertyNode * child){}

      virtual void colorChanged(const osg::Vec4& color)  {}
      virtual void colorFillChanged(const osg::Vec4& color){}

      void setDrawable( osg::Drawable* drawable );

    private:

      osg::Drawable  *_drawable;

      Element(const Element&);// = delete
  };

}  // namespace canvas

#endif /* CANVAS_ELEMENT_HXX_ */
