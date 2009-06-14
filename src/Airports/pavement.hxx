// pavement.hxx - class to represent complex taxiway specified in v850 apt.dat 
//
// Copyright (C) 2009 Frederic Bouvier
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
//
// $Id$

#ifndef FG_PAVEMENT_HXX
#define FG_PAVEMENT_HXX

#include <Navaids/positioned.hxx>

class FGPavement : public FGPositioned
{
public:
  /*
   * 111 = Node (simple point).
   * 112 = Node with Bezier control point.
   * 113 = Node (close loop) point (to close a pavement boundary).
   * 114 = Node (close loop) point with Bezier control point (to close a pavement boundary).
   * 115 = Node (end) point to terminate a linear feature (so has no descriptive codes).
   * 116 = Node (end) point with Bezier control point, to terminate a linear feature (so has no descriptive codes).
   */
  struct NodeBase : public SGReferenced
  {
    SGGeod mPos;
    bool mClose;
    virtual ~NodeBase(){} // To enable RTTI
  };
  struct SimpleNode : public NodeBase //111,113
  {
    SimpleNode(const SGGeod &aPos, bool aClose) {
      mPos = aPos;
      mClose = aClose;
    }
  };
  struct BezierNode : public NodeBase //112,114
  {
    BezierNode(const SGGeod &aPos, const SGGeod &aCtrlPt, bool aClose) {
      mPos = aPos;
      mClose = aClose;
      mControl = aCtrlPt;
    }
    SGGeod mControl;
  };
  typedef std::vector<SGSharedPtr<NodeBase> > NodeList;


  FGPavement(const std::string& aIdent, const SGGeod& aPos);

  void addNode(const SGGeod &aPos, bool aClose = false);
  void addBezierNode(const SGGeod &aPos, const SGGeod &aCtrlPt, bool aClose = false);

  const NodeList &getNodeList() const { return mNodes; }


private:
  NodeList mNodes;
};

#endif // of FG_PAVEMENT_HXX
