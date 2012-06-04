// An OpenVG path on the canvas
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

#ifndef CANVAS_PATH_HXX_
#define CANVAS_PATH_HXX_

#include "element.hxx"

namespace canvas
{
  class PathDrawable;
  class Path:
    public Element
  {
    public:
      Path(SGPropertyNode_ptr node);
      virtual ~Path();

      virtual void update(double dt);

    protected:

      enum PathAttributes
      {
        CMDS       = LAST_ATTRIBUTE << 1,
        COORDS     = CMDS << 1,
        STROKE     = COORDS << 1
      };

      osg::ref_ptr<PathDrawable> _path;

      virtual void childChanged(SGPropertyNode * child);
      virtual void colorChanged(const osg::Vec4& color);
      virtual void colorFillChanged(const osg::Vec4& color);
  };

}  // namespace canvas

#endif /* CANVAS_PATH_HXX_ */
