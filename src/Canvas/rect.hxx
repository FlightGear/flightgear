// Class representing a rectangular region
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

#ifndef CANVAS_RECT_HXX_
#define CANVAS_RECT_HXX_

#include <osg/Vec2>

namespace canvas
{
  template<typename T>
  class Rect
  {
    public:
      Rect() {}

      Rect(T x, T y, T w, T h):
        _x1(x),
        _x2(x + w),
        _y1(y),
        _y2(y + h)
      {}

      void set(T x, T y, T w, T h)
      {
        _x1 = x;
        _x2 = x + w;
        _y1 = y;
        _y2 = y + h;
      }

      T x() const { return _x1; }
      T y() const { return _y1; }
      T width() const { return _x2 - _x1; }
      T height() const { return _y2 - _y1; }

      T l() const { return _x1; }
      T r() const { return _x2; }
      T t() const { return _y1; }
      T b() const { return _y2; }

      bool contains(T x, T y) const
      {
        return _x1 <= x && x <= _x2
            && _y1 <= y && y <= _y2;
      }

    private:
      T _x1, _x2, _y1, _y2;
  };
} // namespace canvas

#endif /* CANVAS_RECT_HXX_ */
