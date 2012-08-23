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
#include <simgear/misc/stdint.hxx> // for uint32_t
#include <osg/MatrixTransform>

#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace osg
{
  class Drawable;
}

namespace canvas
{

  class MouseEvent;
  class Element:
    public SGPropertyChangeListener
  {
    public:
      typedef std::map<std::string, const SGPropertyNode*> Style;
      typedef boost::function<void(const SGPropertyNode*)> StyleSetter;
      typedef std::map<std::string, StyleSetter> StyleSetters;

      void removeListener();
      virtual ~Element() = 0;

      /**
       * Called every frame to update internal state
       *
       * @param dt  Frame time in seconds
       */
      virtual void update(double dt);

      virtual bool handleMouseEvent(const canvas::MouseEvent& event);

      osg::ref_ptr<osg::MatrixTransform> getMatrixTransform();

      virtual void childAdded( SGPropertyNode * parent,
                               SGPropertyNode * child );
      virtual void childRemoved( SGPropertyNode * parent,
                                 SGPropertyNode * child );
      virtual void valueChanged(SGPropertyNode * child);

    protected:

      enum Attributes
      {
        BOUNDING_BOX    = 0x0001,
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
      Style                             _style;
      StyleSetters                      _style_setters;
      std::vector<SGPropertyNode_ptr>   _bounding_box;

      Element( SGPropertyNode_ptr node,
               const Style& parent_style,
               uint32_t attributes_used = 0 );

      template<typename T, class C1, class C2>
      Element::StyleSetter
      addStyle(const std::string& name, void (C1::*setter)(T), C2 instance)
      {
        return _style_setters[ name ] =
                 bindStyleSetter<T>(name, setter, instance);
      }

      template<typename T1, typename T2, class C1, class C2>
      Element::StyleSetter
      addStyle(const std::string& name, void (C1::*setter)(T2), C2 instance)
      {
        return _style_setters[ name ] =
                 bindStyleSetter<T1>(name, setter, instance);
      }

      template<class C1, class C2>
      Element::StyleSetter
      addStyle( const std::string& name,
                void (C1::*setter)(const std::string&),
                C2 instance )
      {
        return _style_setters[ name ] =
                 bindStyleSetter<const char*>(name, setter, instance);
      }

      template<typename T1, typename T2, class C1, class C2>
      Element::StyleSetter
      bindStyleSetter( const std::string& name,
                       void (C1::*setter)(T2),
                       C2 instance )
      {
        return boost::bind(setter, instance, boost::bind(&getValue<T1>, _1));
      }

      virtual bool handleLocalMouseEvent(const canvas::MouseEvent& event);

      virtual void childAdded(SGPropertyNode * child)  {}
      virtual void childRemoved(SGPropertyNode * child){}
      virtual void childChanged(SGPropertyNode * child){}

      void setDrawable( osg::Drawable* drawable );
      void setupStyle();

      bool setStyle(const SGPropertyNode* child);

    private:

      osg::ref_ptr<osg::Drawable> _drawable;

      Element(const Element&);// = delete
  };

}  // namespace canvas

#endif /* CANVAS_ELEMENT_HXX_ */
