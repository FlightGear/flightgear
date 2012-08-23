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
      Text(SGPropertyNode_ptr node, const Style& parent_style);
      ~Text();

      void setText(const char* text);
      void setFont(const char* name);
      void setAlignment(const char* align);

    protected:

      class TextOSG;
      osg::ref_ptr<TextOSG> _text;

      virtual void childChanged(SGPropertyNode * child);

      void handleHit(float x, float y);

      typedef osg::ref_ptr<osgText::Font> font_ptr;
      static font_ptr getFont(const std::string& name);
  };

}  // namespace canvas

#endif /* CANVAS_TEXT_HXX_ */
