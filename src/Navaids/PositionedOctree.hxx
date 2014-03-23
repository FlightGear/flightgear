/**
 * PositionedOctree - define a spatial octree containing Positioned items
 * arranged by their global cartesian position.
 */
 
// Written by James Turner, started 2012.
//
// Copyright (C) 2012 James Turner
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

#ifndef FG_POSITIONED_OCTREE_HXX
#define FG_POSITIONED_OCTREE_HXX

// std
#include <vector>
#include <set>
#include <queue>
#include <cassert>
#include <map>
#include <functional>

// SimGear
#include <simgear/math/SGGeometry.hxx>

#include <Navaids/positioned.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/PolyLine.hxx>

namespace flightgear
{

namespace Octree
{
  
  const double LEAF_SIZE = SG_NM_TO_METER * 8.0;
  const double LEAF_SIZE_SQR = LEAF_SIZE * LEAF_SIZE;
  
  /**
   * Decorate an object with a double value, and use that value to order
   * items, for the purpoises of the STL algorithms
   */
  template <class T>
  class Ordered
  {
  public:
    Ordered(const T& v, double x) :
    _order(x),
    _inner(v)
    {
      assert(!isnan(x));
    }
    
    Ordered(const Ordered<T>& a) :
    _order(a._order),
    _inner(a._inner)
    {
    }
    
    Ordered<T>& operator=(const Ordered<T>& a)
    {
      _order = a._order;
      _inner = a._inner;
      return *this;
    }
    
    bool operator<(const Ordered<T>& other) const
    {
      return _order < other._order;
    }
    
    bool operator>(const Ordered<T>& other) const
    {
      return _order > other._order;
    }
    
    const T& get() const
    { return _inner; }
    
    double order() const
    { return _order; }
    
  private:
    double _order;
    T _inner;
  };
  
  class Node;
  typedef Ordered<Node*> OrderedNode;
  typedef std::greater<OrderedNode> FNPQCompare;
  
  /**
   * the priority queue is fundamental to our search algorithm. When searching,
   * we know the front of the queue is the nearest unexpanded node (to the search
   * location). The default STL pqueue returns the 'largest' item from top(), so
   * to get the smallest, we need to replace the default Compare functor (less<>)
   * with greater<>.
   */
  typedef std::priority_queue<OrderedNode, std::vector<OrderedNode>, FNPQCompare> FindNearestPQueue;
  
  typedef Ordered<FGPositioned*> OrderedPositioned;
  typedef std::vector<OrderedPositioned> FindNearestResults;
  
  // for extracting lines, we don't care about distance ordering, since
  // we're always grabbing all the lines in an area
  typedef std::deque<Node*> FindLinesDeque;
  
  extern Node* global_spatialOctree;
  
  class Leaf;
  
  /**
   * Octree node base class, tracks its bounding box and provides various
   * queries relating to it
   */
  class Node
  {
  public:
    int64_t guid() const
    { return _ident; }
    
    const SGBoxd& bbox() const
    { return _box; }
    
    bool contains(const SGVec3d& aPos) const
    {
      return intersects(aPos, _box);
    }
    
    double distToNearest(const SGVec3d& aPos) const
    {
      return dist(aPos, _box.getClosestPoint(aPos));
    }
        
    virtual void visit(const SGVec3d& aPos, double aCutoff,
                       FGPositioned::Filter* aFilter,
                       FindNearestResults& aResults, FindNearestPQueue&) = 0;
    
    virtual Leaf* findLeafForPos(const SGVec3d& aPos) const = 0;
      
    virtual void visitForLines(const SGVec3d& aPos, double aCutoff,
                         PolyLineList& aLines,
                         FindLinesDeque& aQ) const = 0;
    virtual ~Node() {}
  protected:
    Node(const SGBoxd &aBox, int64_t aIdent) :
    _ident(aIdent),
    _box(aBox)
    {
    }
    
    const int64_t _ident;
    const SGBoxd _box;
  };
  
  class Leaf : public Node
  {
  public:
    Leaf(const SGBoxd& aBox, int64_t aIdent);
            
    virtual void visit(const SGVec3d& aPos, double aCutoff,
                       FGPositioned::Filter* aFilter,
                       FindNearestResults& aResults, FindNearestPQueue&);
    
    virtual Leaf* findLeafForPos(const SGVec3d&) const
    {
      return const_cast<Leaf*>(this);
    }
    
    void insertChild(FGPositioned::Type ty, PositionedID id);
      
    void addPolyLine(PolyLineRef);
    
    virtual void visitForLines(const SGVec3d& aPos, double aCutoff,
                               PolyLineList& aLines,
                               FindLinesDeque& aQ) const;
  private:
    bool childrenLoaded;
    
    typedef std::multimap<FGPositioned::Type, PositionedID> ChildMap;
    ChildMap children;
      
    PolyLineList lines;
      
    void loadChildren();
  };
  
  class Branch : public Node
  {
  public:
    Branch(const SGBoxd& aBox, int64_t aIdent);
        
    virtual void visit(const SGVec3d& aPos, double aCutoff,
                       FGPositioned::Filter*,
                       FindNearestResults&, FindNearestPQueue& aQ);
    
    virtual Leaf* findLeafForPos(const SGVec3d& aPos) const
    {
      loadChildren();
      return childForPos(aPos)->findLeafForPos(aPos);
    }
    
    int childMask() const;
    
    virtual void visitForLines(const SGVec3d& aPos, double aCutoff,
                               PolyLineList& aLines,
                               FindLinesDeque& aQ) const;
  private:
    Node* childForPos(const SGVec3d& aCart) const;
    Node* childAtIndex(int childIndex) const;
    
    /**
     * Return the box for a child touching the specified corner
     */
    SGBoxd boxForChild(unsigned int aCorner) const
    {
      SGBoxd r(_box.getCenter());
      r.expandBy(_box.getCorner(aCorner));
      return r;
    }
    
    void loadChildren() const;
    
    mutable Node* children[8];
    mutable bool childrenLoaded;
  };

  bool findNearestN(const SGVec3d& aPos, unsigned int aN, double aCutoffM, FGPositioned::Filter* aFilter, FGPositionedList& aResults, int aCutoffMsec);
  bool findAllWithinRange(const SGVec3d& aPos, double aRangeM, FGPositioned::Filter* aFilter, FGPositionedList& aResults, int aCutoffMsec);
} // of namespace Octree

  
} // of namespace flightgear

#endif // of FG_POSITIONED_OCTREE_HXX
