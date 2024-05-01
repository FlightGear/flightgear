// QuadTree.hxx - Implimentation of the FlightGear Ground radar
//
// Written by Keith Paterson, started Feb 2023.
//
// Copyright (C) 2023 Keith Paterson.
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

#pragma once
#include <simgear/math/SGGeod.hxx>
#include <simgear/math/SGBox.hxx>
#include <simgear/math/SGRect.hxx>
#include <simgear/debug/logstream.hxx>
#include <algorithm>
#include <array>
#include <type_traits>
#include <memory>
#include <vector>

/**
 * Class .
*/

namespace quadtree {

enum Quadrant {SOUTH_WEST, SOUTH_EAST, NORTH_EAST, NORTH_WEST, UNKNOWN};

template <class T, typename GetBox, typename Equal>
class Node {
  private:
    const std::size_t MAX_DEPTH = 8;
    const std::size_t SPLIT_THRESHOLD = 10;
    const std::size_t depth;
    SGRectd bounds;
    std::array<std::unique_ptr<quadtree::Node<T,GetBox,Equal>>, 4> children;
    std::vector<T*> data;
    const int quadrant;
  public:

    Node(std::size_t depth, int fQuadtrant): depth(depth), data(), quadrant(fQuadtrant)  {
    }

    size_t size() {return data.size();}

    bool isLeaf() {return children[0] == nullptr;};

    void resize( const SGRectd& bounds ) {
        if (data.size()>0) {
            SG_LOG(SG_ATC, SG_ALERT, "Resizing Quadtree with data not supported");
        }
        this->bounds = bounds;
        SG_LOG(SG_ATC, SG_DEBUG , "Resizing Quadtree to " << bounds.x() << "\t" << bounds.y() << "\t" << bounds.width() << "\t" << bounds.height());
    }

    void add(const SGRectd& pos, T* value)
    {
        if (isLeaf())
        {
            if (depth >= MAX_DEPTH || data.size() < SPLIT_THRESHOLD) {
                data.push_back(value);
            }
            else
            {
                auto i = split(pos);
                if (i != UNKNOWN) {
                    children[static_cast<std::size_t>(i)].get()->add(pos, value);
                }
                else {
                    data.push_back(value);
                }
            }
        }
        else
        {
            auto i = getQuadrant(bounds, pos);
            if (i != UNKNOWN) {
              children[static_cast<std::size_t>(i)].get()->add(pos, value);
            }
            else {
              data.push_back(value);
            }
        }
    };

    void removeValue(T* value, const Equal& equalFkt) {
          // Find the value in data
        auto it = std::find_if(std::begin(data), std::end(data),
            [equalFkt, &value](auto* rhs){ return equalFkt(value, rhs); });
        assert(it != std::end(data) && "Trying to remove a value that is not present in the node");
        // Swap with the last element and pop back
        *it = std::move(data.back());
        data.pop_back();
    }

    bool remove(const SGRectd& pos, T* value, const Equal& equal) {
        if (isLeaf()) {
            removeValue(value, equal);
            return true;
        } else {
            // Remove the value in a child if the value is entirely contained in it
            auto i = getQuadrant(bounds, pos);
            if (i != UNKNOWN) {
                if (children[static_cast<std::size_t>(i)].get()->remove(computeBox(pos, i), value, equal))
                    return tryMerge();
            }
            // Otherwise, we remove the value from the current node
            else {
                removeValue(value, equal);
            }
            return false;
        }
    }

    bool tryMerge() {
        auto nbValues = size();
        for (const auto& child : children)
        {
            if (!child.get()->isLeaf())
                return false;
            nbValues += child.get()->size();
        }
        SG_LOG(SG_ATC, SG_DEBUG , "Trying to merge Quadtree " << nbValues);
        return true;
    }


    int split(const SGRectd& pos) {
        // Create children
        SG_LOG(SG_ATC, SG_DEBUG, "Splitting Quadtree " << data.size());

        for (size_t i = 0; i < 4; i++) {
            children[i] = std::make_unique<Node>(depth+1, i);
            children[i].get()->resize(computeBox(bounds, i));
        }
        // Assign values to children
        auto newValues = std::vector<T*>(); // New values for this node
        for (auto value : data)
        {
            auto i = getQuadrant(bounds, pos);
            if (i != UNKNOWN) {
                children[static_cast<std::size_t>(i)].get()->add(pos, value);
            }
            else {
                newValues.push_back(value);
            }
        }
        data = std::move(newValues);
        return getQuadrant(bounds, pos);
    };

    SGRectd computeBox(const SGRectd& box, int i) const
    {
        auto origin = box.getMin();
        auto childSize = box.size();
        childSize /= 2.0;
        switch (i)
        {
            case SOUTH_WEST:
                return SGRectd(SGVec2d(origin.x(), origin.y() + childSize.y()), SGVec2d(origin.x() + childSize.x(), origin.y() + 2*childSize.y()));
            case SOUTH_EAST:
                return SGRectd(SGVec2d(origin.x(), origin.y()), SGVec2d(origin.x() + childSize.x(), origin.y() + childSize.y()));
            case NORTH_WEST:
                return SGRectd(SGVec2d(origin.x() + childSize.x(), origin.y()), SGVec2d(origin.x() + 2*childSize.x(), origin.y() + childSize.y()));
            case NORTH_EAST:
                return SGRectd(SGVec2d(origin.x() + childSize.x(), origin.y() + childSize.y()), SGVec2d(origin.x() + 2*childSize.x(), origin.y() + 2*childSize.y()));
            default:
                assert(false && "Invalid quadrant index");
                return SGRectd();
        }
    };

    SGRectd computeBoxCenter(const SGRectd& box, int i) const
    {
        auto origin = box.getMin();
        auto childSize = box.size();
        childSize /= 4.0;
        switch (i)
        {
            case SOUTH_WEST:
                return SGRectd(SGVec2d(origin.x() + childSize.x(), origin.y() + childSize.y()), SGVec2d(origin.x() + childSize.x(), origin.y() + childSize.y()));
            case SOUTH_EAST:
                return SGRectd(SGVec2d(origin.x() + childSize.x(), origin.y() + 3*childSize.y()), SGVec2d(origin.x() + childSize.x(), origin.y() + 3*childSize.y()));
            case NORTH_WEST:
                return SGRectd(SGVec2d(origin.x() + 3*childSize.x(), origin.y() + childSize.y()), SGVec2d(origin.x() + 3*childSize.x(), origin.y() + childSize.y()));
            case NORTH_EAST:
                return SGRectd(SGVec2d(origin.x() + 3*childSize.x(), origin.y() + 3*childSize.y()), SGVec2d(origin.x() + 3*childSize.x(), origin.y() + 3*childSize.y()));
            default:
                assert(false && "Invalid quadrant index");
                return SGRectd();
        }
    };

    void query(const SGRectd& queryBox, const GetBox& getBoxFunction, std::vector<T*>& values)
    {
        assert(queryBox.contains(bounds.x(), bounds.y()));
        for (auto value : data)
        {
            auto pos = getBoxFunction(value);
            if (queryBox.contains(pos.x(), pos.y())) {
                values.push_back(value);
            }
        }
        if (!isLeaf())
        {
            for (auto i = std::size_t(0); i < children.size(); ++i)
            {
                auto childBox = computeBoxCenter(bounds, static_cast<int>(i));
                if (queryBox.contains(childBox.x(), childBox.y())) {
                    children[i].get()->query(queryBox, getBoxFunction, values);
                }
            }
        }
    };


    Quadrant getQuadrant(const SGRectd& nodeBox, const SGRectd& valueBox) const
    {
        auto center = nodeBox.getMin();
        // West
        if (valueBox.x() < nodeBox.x() + nodeBox.width()/2)
        {
            if (valueBox.y() < nodeBox.y() + nodeBox.height()/2)
                return NORTH_WEST;
            else if (valueBox.y() + valueBox.height() >= nodeBox.y() + nodeBox.height()/2)
                return SOUTH_WEST;
            // Not contained in any quadrant
            else
                return UNKNOWN;
        }
        // East
        else if (valueBox.x() >= nodeBox.x() + nodeBox.width()/2)
        {
            if (valueBox.y() < nodeBox.y() + nodeBox.height()/2)
                return NORTH_EAST;
            else if (valueBox.y() + valueBox.height() >= nodeBox.y() + nodeBox.height()/2)
                return SOUTH_EAST;
            // Not contained in any quadrant
            else
                return UNKNOWN;
        }
        // Not contained in any quadrant
        else
            return UNKNOWN;
    };
};

template <class T, typename GetBox, typename Equal>
class QuadTree {
  private:
    std::unique_ptr<quadtree::Node<T, GetBox, Equal>> rootNode;
    SGRectd rootRect;
    GetBox getBoxFunction;
    Equal equalFunction;
  public:
    QuadTree(const GetBox& getBox,
             const Equal& equal
            ): rootNode(std::make_unique<quadtree::Node<T,GetBox, Equal>>(0, UNKNOWN)), getBoxFunction(getBox), equalFunction(equal) {}

    void resize( const SGRectd& bounds ) {        
        rootNode.get()->resize(bounds);
    }

    void add(T* value)
    {
        rootNode.get()->add(getBoxFunction(value), value);
    }

    void remove(T* value)
    {
        rootNode.get()->remove(getBoxFunction(value), value, equalFunction);
    }

    void query(T* value, std::vector<T*>& values)
    {
        return rootNode.get()->query(getBoxFunction(value), getBoxFunction, values);
    }

    void query(const SGRectd& queryBox, std::vector<T*>& values)
    {
        return rootNode.get()->query(queryBox, getBoxFunction, values);
    }

    size_t size(){return rootNode.get()->size();}
};
}


