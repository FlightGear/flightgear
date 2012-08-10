// A group of 2D canvas elements
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

#ifndef CANVAS_GROUP_HXX_
#define CANVAS_GROUP_HXX_

#include "element.hxx"
#include <boost/shared_ptr.hpp>
#include <list>

namespace canvas
{

  typedef boost::shared_ptr<Element> ElementPtr;

  class Group:
    public Element
  {
    public:
      typedef std::list< std::pair< const SGPropertyNode*,
                                    ElementPtr
                                  >
                       > ChildList;

      Group(SGPropertyNode_ptr node);
      virtual ~Group();

      virtual void update(double dt);

    protected:

      ChildList _children;

      virtual bool handleLocalMouseEvent(const canvas::MouseEvent& event);

      virtual void childAdded(SGPropertyNode * child);
      virtual void childRemoved(SGPropertyNode * child);
  };

}  // namespace canvas

#endif /* CANVAS_GROUP_HXX_ */
