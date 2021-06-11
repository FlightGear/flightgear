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

#include "config.h"

#include "PositionedOctree.hxx"
#include "positioned.hxx"

#include <cassert>
#include <algorithm> // for sort
#include <cstring> // for memset
#include <iostream>

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/timing/timestamp.hxx>

#include "PolyLine.hxx"

namespace flightgear
{

namespace Octree
{

static std::unique_ptr<Node> global_spatialOctree;
static std::unique_ptr<Node> global_transientOctree;

double RADIUS_EARTH_M = 7000 * 1000.0; // 7000km is plenty

Node* globalTransientOctree()
{
    if (!global_transientOctree) {
        SGVec3d earthExtent(RADIUS_EARTH_M, RADIUS_EARTH_M, RADIUS_EARTH_M);
        global_transientOctree.reset(new Octree::Branch(SGBox<double>(-earthExtent, earthExtent), 1, false));
    }

    return global_transientOctree.get();
}

Node* globalPersistentOctree()
{
    if (!global_spatialOctree) {
        SGVec3d earthExtent(RADIUS_EARTH_M, RADIUS_EARTH_M, RADIUS_EARTH_M);
        global_spatialOctree.reset(new Octree::Branch(SGBox<double>(-earthExtent, earthExtent), 1, true));
    }

    return global_spatialOctree.get();
}

Node::Node(const SGBoxd& aBox, int64_t aIdent, bool persistent) : _ident(aIdent),
                                                                  _persistent(persistent),
                                                                  _box(aBox)
{
}

Node::~Node()
{
}

void Node::addPolyLine(const PolyLineRef& aLine)
{
    lines.push_back(aLine);
}

void Node::visitForLines(const SGVec3d& aPos, double aCutoff,
                           PolyLineList& aLines,
                           FindLinesDeque& aQ) const
{
    SG_UNUSED(aPos);
    SG_UNUSED(aCutoff);

    aLines.insert(aLines.end(), lines.begin(), lines.end());
}

Node *Node::findNodeForBox(const SGBoxd&) const
{
    return const_cast<Node*>(this);
}

Leaf::Leaf(const SGBoxd& aBox, int64_t aIdent, bool persistent) : Node(aBox, aIdent, persistent)
{
    if (!persistent) {
        _childrenLoaded = true;
    }
}

void Leaf::visit(const SGVec3d& aPos, double aCutoff,
                   FGPositioned::Filter* aFilter,
                   FindNearestResults& aResults, FindNearestPQueue&)
{
  int previousResultsSize = aResults.size();
  int addedCount = 0;
  NavDataCache* cache = NavDataCache::instance();

  loadChildren();

  ChildMap::const_iterator it = children.lower_bound(aFilter->minType());
  ChildMap::const_iterator end = children.upper_bound(aFilter->maxType());

  for (; it != end; ++it) {
    FGPositioned* p = cache->loadById(it->second);
    double d = dist(aPos, p->cart());
    if (d > aCutoff) {
      continue;
    }

    if (aFilter && !aFilter->pass(p)) {
      continue;
    }

    ++addedCount;
    aResults.push_back(OrderedPositioned(p, d));
  }

  if (addedCount == 0) {
    return;
  }

  // keep aResults sorted
  // sort the new items, usually just one or two items
  std::sort(aResults.begin() + previousResultsSize, aResults.end());

  // merge the two sorted ranges together - in linear time
  std::inplace_merge(aResults.begin(),
                     aResults.begin() + previousResultsSize, aResults.end());
}

void Leaf::insertChild(FGPositioned::Type ty, PositionedID id)
{
  assert(_childrenLoaded);
  children.insert(children.end(), TypedPositioned(ty, id));
}

void Leaf::loadChildren()
{
    if (_childrenLoaded) {
        return;
    }

  NavDataCache* cache = NavDataCache::instance();
  for (const auto& tp : cache->getOctreeLeafChildren(guid())) {
    // REVIEW: Memory Leak - 1,728 bytes in 36 blocks are still reachable
    children.insert(children.end(), tp);
  } // of leaf members iteration

  _childrenLoaded = true;
}

///////////////////////////////////////////////////////////////////////////////

Branch::Branch(const SGBoxd& aBox, int64_t aIdent, bool persistent) : Node(aBox, aIdent, persistent)
{
    _children.fill(nullptr);
    if (!_persistent) {
        _childrenLoaded = true;
    }
}

void Branch::visit(const SGVec3d& aPos, double aCutoff,
                   FGPositioned::Filter*,
                   FindNearestResults&, FindNearestPQueue& aQ)
{
  loadChildren();
  for (unsigned int i=0; i<8; ++i) {
      if (!_children[i]) {
          continue;
      }

      double d = _children[i]->distToNearest(aPos);
      if (d > aCutoff) {
          continue; // exceeded cutoff
    }

    aQ.push(Ordered<Node*>(_children[i], d));
  } // of child iteration
}

void Branch::visitForLines(const SGVec3d& aPos, double aCutoff,
                           PolyLineList& aLines,
                           FindLinesDeque& aQ) const
{
    // add our own lines, easy
    Node::visitForLines(aPos, aCutoff, aLines, aQ);

    for (unsigned int i=0; i<8; ++i) {
        if (!_children[i]) {
            continue;
        }

        double d = _children[i]->distToNearest(aPos);
        if (d > aCutoff) {
            continue; // exceeded cutoff
        }

        aQ.push_back(_children[i]);
    } // of child iteration
}

static bool boxContainsBox(const SGBoxd& a, const SGBoxd& b)
{
    const SGVec3d aMin(a.getMin()),
            aMax(a.getMax()),
            bMin(b.getMin()),
            bMax(b.getMax());
    for (int i=0; i<3; ++i) {
        if ((bMin[i] < aMin[i]) || (bMax[i] > aMax[i])) return false;
    }

    return true;
}

Node *Branch::findNodeForBox(const SGBoxd &box) const
{
    // do this so childAtIndex sees consistent state of
    // children[] and loaded flag.
    loadChildren();

    for (unsigned int i=0; i<8; ++i) {
        const SGBoxd childBox(boxForChild(i));
        if (boxContainsBox(childBox, box)) {
            return childAtIndex(i)->findNodeForBox(box);
        }
    }

    return Node::findNodeForBox(box);
}

Node* Branch::childForPos(const SGVec3d& aCart) const
{
  assert(contains(aCart));
  int childIndex = 0;

  SGVec3d center(_box.getCenter());
// tests must match indices in SGbox::getCorner
  if (aCart.x() < center.x()) {
    childIndex += 1;
  }

  if (aCart.y() < center.y()) {
    childIndex += 2;
  }

  if (aCart.z() < center.z()) {
    childIndex += 4;
  }

  return childAtIndex(childIndex);
}

Node* Branch::childAtIndex(int childIndex) const
{
    Node* child = _children[childIndex];
    if (!child) { // lazy building of children
        SGBoxd cb(boxForChild(childIndex));
        double d2 = dot(cb.getSize(), cb.getSize());

        assert(((_ident << 3) >> 3) == _ident);

        // child index is 0..7, so 3-bits is sufficient, and hence we can
        // pack 20 levels of octree into a int64, which is plenty
        int64_t childIdent = (_ident << 3) | childIndex;

        if (d2 < LEAF_SIZE_SQR) {
            // REVIEW: Memory Leak - 480 bytes in 3 blocks are still reachable
            child = new Leaf(cb, childIdent, _persistent);
        } else {
            // REVIEW: Memory Leak - 9,152 bytes in 52 blocks are still reachable
            child = new Branch(cb, childIdent, _persistent);
        }

        _children[childIndex] = child;

        if (_persistent && _childrenLoaded) {
            // childrenLoad is done, so we're defining a new node - add it to the
            // cache too.
            NavDataCache::instance()->defineOctreeNode(const_cast<Branch*>(this), child);
        }
  }

  return _children[childIndex];
}

void Branch::loadChildren() const
{
    if (_childrenLoaded) {
        return;
    }

  int childrenMask = NavDataCache::instance()->getOctreeBranchChildren(guid());
  for (int i=0; i<8; ++i) {
    if ((1 << i) & childrenMask) {
      childAtIndex(i); // accessing will create!
    }
  } // of child index iteration

// set this after creating the child nodes, so the cache update logic
// in childAtIndex knows any future created children need to be added.
  _childrenLoaded = true;
}

int Branch::childMask() const
{
  int result = 0;
  for (int i=0; i<8; ++i) {
      if (_children[i]) {
          result |= 1 << i;
      }
  }

  return result;
}

bool findNearestN(const SGVec3d& aPos, unsigned int aN, double aCutoffM, FGPositioned::Filter* aFilter, FGPositionedList& aResults, int aCutoffMsec)
{
  aResults.clear();
  FindNearestPQueue pq;
  FindNearestResults results;
  pq.push(Ordered<Node*>(globalPersistentOctree(), 0));
  double cut = aCutoffM;

  SGTimeStamp tm;
  tm.stamp();

  while (!pq.empty() && (tm.elapsedMSec() < aCutoffMsec)) {
    if (!results.empty()) {
      // terminate the search if we have sufficent results, and we are
      // sure no node still on the queue contains a closer match
      double furthestResultOrder = results.back().order();
      if ((results.size() >= aN) && (furthestResultOrder < pq.top().order())) {
    // clear the PQ to mark this has 'full results' instead of partial
        pq = FindNearestPQueue();
        break;
      }
    }

    Node* nd = pq.top().get();
    pq.pop();

    nd->visit(aPos, cut, aFilter, results, pq);
  } // of queue iteration

  // depending on leaf population, we may have (slighty) more results
  // than requested
  unsigned int numResults = std::min((unsigned int) results.size(), aN);
  // copy results out
  aResults.resize(numResults);
  for (unsigned int r=0; r<numResults; ++r) {
    aResults[r] = results[r].get();
  }

  return !pq.empty();
}

bool findAllWithinRange(const SGVec3d& aPos, double aRangeM, FGPositioned::Filter* aFilter, FGPositionedList& aResults, int aCutoffMsec)
{
  aResults.clear();
  FindNearestPQueue pq;
  FindNearestResults results;
  pq.push(Ordered<Node*>(globalPersistentOctree(), 0));
  double rng = aRangeM;

  SGTimeStamp tm;
  tm.stamp();

  while (!pq.empty() && (tm.elapsedMSec() < aCutoffMsec)) {
    Node* nd = pq.top().get();
    pq.pop();

    nd->visit(aPos, rng, aFilter, results, pq);
  } // of queue iteration

  unsigned int numResults = results.size();
  // copy results out
  aResults.resize(numResults);
  for (unsigned int r=0; r<numResults; ++r) {
    aResults[r] = results[r].get();
  }

  return !pq.empty();
}

} // of namespace Octree

} // of namespace flightgear
