// A text on the canvas
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

#ifndef CANVAS_TEXT_HXX_
#define CANVAS_TEXT_HXX_

#include "element.hxx"
#include <osgText/Text>
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

namespace canvas
{

  class Text:
    public Element
  {
    public:
      Text(SGPropertyNode_ptr node);
      virtual ~Text();

      virtual void update(double dt);

      void setFont(const char* name);

      void setAlignment(const char* align);
      const char* getAlignment() const;

    protected:

      enum TextAttributes
      {
        FONT_SIZE       = LAST_ATTRIBUTE << 1, // Font size and aspect ration
      };

      osg::ref_ptr<osgText::Text>   _text;

      SGPropertyNode_ptr  _font_size,
                          _font_aspect;

      virtual void childChanged(SGPropertyNode * child);
      virtual void colorChanged(const osg::Vec4& color);
      virtual void colorFillChanged(const osg::Vec4& color);

      typedef osg::ref_ptr<osgText::Font> font_ptr;
      static font_ptr getFont(const std::string& name);
  };

}  // namespace canvas

#endif /* CANVAS_TEXT_HXX_ */
