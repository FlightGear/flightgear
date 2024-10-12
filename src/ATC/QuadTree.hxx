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
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/debug/logstream.hxx>
#include <algorithm>
#include <array>
#include <type_traits>
#include <memory>
#include <vector>
#include <string>

/**
 * Class .
*/

namespace quadtree {

enum Quadrant {SOUTH_WEST, SOUTH_EAST, NORTH_WEST, NORTH_EAST, UNKNOWN};

template <class T, typename GetBox, typename Equal>
class Node {
  private:
    const std::size_t MAX_DEPTH = 8;
    const std::size_t SPLIT_THRESHOLD = 10;
    const std::size_t depth;
    SGRectd bounds;
    std::array<std::unique_ptr<quadtree::Node<T,GetBox,Equal>>, 4> children;
    std::vector<SGSharedPtr<T>> data;
    const int quadrant;
  public:

    Node(std::size_t depth, int fQuadtrant): depth(depth), data(), quadrant(fQuadtrant)  {
    }

    size_t size() {
        if (children[0] == nullptr) {
            return data.size();
        } else {
            int childrenSize = 0;
            for (auto& n : children) {
                childrenSize += n->size();
            }
            return childrenSize;  
        }
    }

    bool isLeaf() {return children[0] == nullptr;};

    void resize( const SGRectd& bounds ) {
        if (data.size()>0) {
            SG_LOG(SG_ATC, SG_ALERT, "Resizing Quadtree with data not supported");
        }
        this->bounds = bounds;
        SG_LOG(SG_ATC, SG_DEBUG , "Resizing Quadtree " << quadrant << " to " << bounds.x() << "\t" << bounds.y() << "\t Width : " << bounds.width() << "\t Height : " << bounds.height());
    }

    bool add(const SGRectd& pos, SGSharedPtr<T> value, const Equal& equalFkt, const GetBox& getBoxFunction)
    {
        if (equalFkt==nullptr || getBoxFunction==nullptr) {
            SG_LOG(SG_ATC, SG_DEBUG , "equalFkt " << equalFkt << " getBoxFunction " << getBoxFunction);
        }
        if (isLeaf())
        {
            if (!bounds.contains(pos.x(), pos.y())) {
                SG_LOG(SG_ATC, SG_ALERT , "Not in node Quadrant " << quadrant << " to bounds  " << bounds.x() << "\t" << bounds.y() << "\t" << (bounds.x()+bounds.width()) << "\t" << (bounds.y()+bounds.height()) << "\t Pos : \t" << pos.x() << "\t" << pos.y() );
                return false;
            }
            if (depth >= MAX_DEPTH || data.size() < SPLIT_THRESHOLD) {
                if (depth >= MAX_DEPTH) {
                    SG_LOG(SG_ATC, SG_DEBUG , "Max Depth reached");
                }
                // 
                auto it = std::find_if(std::begin(data), std::end(data),
                    [equalFkt, value](auto rhs){ return equalFkt(value, rhs); });
                if (it == std::end(data)) {
                    data.push_back(value);
                    return true;
                } else {
                    SG_LOG(SG_ATC, SG_DEBUG , "Not readded" );
                    return false;
                }
            }
            else
            {
                auto i = split(pos, equalFkt, getBoxFunction);
                if (i != UNKNOWN) {
                    return children[static_cast<std::size_t>(i)].get()->add(pos, value, equalFkt, getBoxFunction);
                }
                else {
                    return false;
                }
            }
        }
        else
        {
            auto i = getQuadrant(bounds, pos);
            if (i != UNKNOWN) {
              return children[static_cast<std::size_t>(i)].get()->add(pos, value, equalFkt, getBoxFunction);
            }
            else {
              return false;
            }
        }
    };

    bool move(const SGRectd& newPos, const SGRectd& pos, SGSharedPtr<T> value, const Equal& equalFkt, const GetBox& getBoxFunction)
    {
        SG_LOG(SG_ATC, SG_DEBUG,
               "Moving  " << pos.x() << ":" << pos.y() << " to " << newPos.x() << ":" << newPos.y() << (isLeaf()?" leaf ":" "));

        // finding 
        if (isLeaf())
        {
            // No need to do anything since in same node
//            removeValue(value, equalFkt);
            return true;
        }
        else
        {
            auto oldQuadrant = getQuadrant(bounds, pos);
            auto newQuadrant = getQuadrant(bounds, newPos);
            if (oldQuadrant != UNKNOWN) {
                if (oldQuadrant != newQuadrant) {
                    SG_LOG(SG_ATC, SG_DEBUG,
                      "Moving from quadrant " << oldQuadrant << " to quadrant " << newQuadrant << " Level " << depth );
                    children[static_cast<std::size_t>(oldQuadrant)].get()->remove(pos, value, equalFkt);    
                    children[static_cast<std::size_t>(newQuadrant)].get()->add(newPos, value, equalFkt, getBoxFunction);    
                } else {
                    children[static_cast<std::size_t>(oldQuadrant)].get()->move(newPos, pos, value, equalFkt, getBoxFunction);
                }
            } else {
                SG_LOG(SG_ATC, SG_WARN,
                    "Failed Moving from quadrant " << oldQuadrant << " to quadrant " << newQuadrant << " Level " << depth );
            }
            return false;
        }
    };

    bool removeValue(SGSharedPtr<T> value, const Equal& equalFkt) {
          // Find the value in data
        auto it = std::find_if(std::begin(data), std::end(data),
            [equalFkt, value](auto rhs){ return equalFkt(value, rhs); });
        if (it == std::end(data)) {
            SG_LOG(SG_ATC, SG_ALERT , "Trying to remove non existant data " << data.size());
            return false;
        }
        // Swap with the last element and pop back
        *it = std::move(data.back());
        data.pop_back();
        return true;
    }

    bool remove(const SGRectd& pos, SGSharedPtr<T> value, const Equal& equalFunction) {
        if (isLeaf()) {
            return removeValue(value, equalFunction);
        } else {
            // Remove the value in a child if the value is entirely contained in it
            auto i = getQuadrant(bounds, pos);
            SG_LOG(SG_ATC, SG_DEBUG , "Remove from quadrant " << i << " Depth " << depth);
            if (i != UNKNOWN) {                
                if (children[static_cast<std::size_t>(i)].get()->remove(computeBox(pos, i), value, equalFunction)) {
                    return tryMerge();
                } else {
                    bool found = children[static_cast<std::size_t>(i)].get()->findFullScan(value, equalFunction, "Error /");
                    SG_LOG(SG_ATC, SG_DEBUG , "Trying to find misplaced data " << found);
                }
            // Otherwise, we remove the value from the current node
            } else {
                SG_LOG(SG_ATC, SG_ALERT , "Trying to remove from UNKNOWN non leaf " );
                return removeValue(value, equalFunction);
            }
            return false;
        }
    }

    /**
     * For debugging. Find path to value
     */

    bool findFullScan(SGSharedPtr<T> value, const Equal& equalFkt, const std::string& path) {
        if (isLeaf()) {
            auto it = std::find_if(std::begin(data), std::end(data),
                [equalFkt, value](auto rhs){ return equalFkt(value, rhs); });
            if (it == std::end(data)) {
//                SG_LOG(SG_ATC, SG_ALERT , "Not found " << path << " " );
                return false;
            } else {
                SG_LOG(SG_ATC, SG_DEBUG , "Found in path node " << path << " " );
                return true;
            }
        } else {
            for (size_t i = 0; i < 4; i++) {
                std::string subpath = path + std::to_string(i) + "/";
                bool found = children[static_cast<std::size_t>(i)].get()->findFullScan(value, equalFkt, subpath);
                if (found) {
                    return true;
                }                
            } 
            // Not found in a subnode
            return false;
        }
    }

    bool printPath(const SGRectd& pos, SGSharedPtr<T> value, const Equal& equalFkt, const std::string& path) {
        if (isLeaf()) {
            SG_LOG(SG_ATC, SG_DEBUG , path );
            auto it = std::find_if(std::begin(data), std::end(data),
                [equalFkt, value](auto rhs){ return equalFkt(value, rhs); });
            if (it == std::end(data)) {
                SG_LOG(SG_ATC, SG_ALERT , "Not found when printing path" );
                return false;
            } else {
                return true;
            }
        } else {
            auto i = getQuadrant(bounds, pos);
            if (i != UNKNOWN) {
                std::string subpath = path + std::to_string(i) + "/";
                return children[static_cast<std::size_t>(i)].get()->printPath(computeBox(pos, i), value, equalFkt, subpath);
            } else {
                SG_LOG(SG_ATC, SG_ALERT , "Unkown quadrant " );
            }
            return false;
        }
    }

    bool tryMerge() {
        auto nbValues = size();
        for (const auto& child : children)
        {
            if (!child.get()->isLeaf())
              return true;
            nbValues += child.get()->size();
        }
        SG_LOG(SG_ATC, SG_DEBUG , "Trying to merge Quadtree " << nbValues);
        return true;
    }


    int split(const SGRectd& pos, const Equal& equalFkt, const GetBox& getBoxFunction) {
        // Create children
        SG_LOG(SG_ATC, SG_DEBUG, "Splitting Quadtree Size : " << data.size() << " Depth : " << depth);

        for (size_t i = 0; i < 4; i++) {
            children[i] = std::make_unique<Node>(depth+1, i);
            children[i].get()->resize(computeBox(bounds, i));
        }
        // Assign values to children
        auto newValues = std::vector<SGSharedPtr<T>>(); // New values for this node
        for (auto value : data)
        {
            auto i = getQuadrant(bounds, getBoxFunction(value));
            if (i != UNKNOWN) {
                children[static_cast<std::size_t>(i)].get()->add(getBoxFunction(value), value, equalFkt, getBoxFunction);
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
                return SGRectd(SGVec2d(origin.x(), origin.y()), SGVec2d(origin.x() + childSize.x(), origin.y() + childSize.y()));
            case NORTH_WEST:
                return SGRectd(SGVec2d(origin.x(), origin.y() + childSize.y()), SGVec2d(origin.x() + childSize.x(), origin.y() + 2*childSize.y()));
            case SOUTH_EAST:
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
            case NORTH_WEST:
                return SGRectd(SGVec2d(origin.x() + childSize.x(), origin.y() + 3*childSize.y()), SGVec2d(origin.x() + childSize.x(), origin.y() + 3*childSize.y()));
            case SOUTH_EAST:
                return SGRectd(SGVec2d(origin.x() + 3*childSize.x(), origin.y() + childSize.y()), SGVec2d(origin.x() + 3*childSize.x(), origin.y() + childSize.y()));
            case NORTH_EAST:
                return SGRectd(SGVec2d(origin.x() + 3*childSize.x(), origin.y() + 3*childSize.y()), SGVec2d(origin.x() + 3*childSize.x(), origin.y() + 3*childSize.y()));
            default:
                assert(false && "Invalid quadrant index");
                return SGRectd();
        }
    };

    void query(const SGRectd& queryBox, const GetBox& getBoxFunction, std::vector<SGSharedPtr<T>>& values)
    {
        SG_LOG(SG_ATC, SG_DEBUG, "Query Quadtree " << queryBox.getMin().x() << "\t" << queryBox.getMin().y() << "\t" 
        << queryBox.getMax().x() << "\t" << queryBox.getMax().y()
        << " depth " << depth << " Leaf : " << (isLeaf()?"true":"false"));
        assert(queryBox.contains(bounds.x(), bounds.y()));
        for (auto value : data)
        {
            auto pos = getBoxFunction(value);
            SG_LOG(SG_ATC, SG_DEBUG, "Query Quadtree " << pos.x() << "\t" << pos.y());
            if (queryBox.contains(pos.x(), pos.y())) {
                values.push_back(value);
            }
        }
        if (!isLeaf())
        {
            for (auto i = std::size_t(0); i < children.size(); i++)
            {
                //FIXME
                auto childBox = computeBox(bounds, static_cast<int>(i));
                SG_LOG(SG_ATC, SG_DEBUG, "Query Quadtree center " << i << "\t" << childBox.x() << "\t" << childBox.y() << "\t" << childBox.width() << "\t" << childBox.height() );
                if (childBox.contains(queryBox.getMin().x(), queryBox.getMin().y()) ||
                    childBox.contains(queryBox.getMin().x(), queryBox.getMax().y()) || 
                    childBox.contains(queryBox.getMax().x(), queryBox.getMax().y()) ||
                    childBox.contains(queryBox.getMax().x(), queryBox.getMin().y())) {
                    children[i].get()->query(queryBox, getBoxFunction, values);
                } else {
                    SG_LOG(SG_ATC, SG_DEBUG, "Query Quadtree center " << i << " not found " );
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
                return SOUTH_WEST;
            else if (valueBox.y() + valueBox.height() >= nodeBox.y() + nodeBox.height()/2)
                return NORTH_WEST;
            // Not contained in any quadrant
            else
                return UNKNOWN;
        }
        // East
        else if (valueBox.x() >= nodeBox.x() + nodeBox.width()/2)
        {
            if (valueBox.y() < nodeBox.y() + nodeBox.height()/2)
                return SOUTH_EAST;
            else if (valueBox.y() + valueBox.height() >= nodeBox.y() + nodeBox.height()/2)
                return NORTH_EAST;
            // Not contained in any quadrant
            else
                return UNKNOWN;
        }
        // Not contained in any quadrant
        else
            return UNKNOWN;
    };

    SGRectd getBounds() {
       return bounds;
    }
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

    bool add(SGSharedPtr<T> value)
    {
        if (rootNode.get()!=nullptr) {
            SGRectd pos = getBoxFunction(value);
            SGRectd bounds = rootNode.get()->getBounds();
            if (!bounds.contains(pos.x(), pos.y())) {
                SG_LOG(SG_ATC, SG_ALERT , "Not in index Bounds : " << bounds.x() << "x" << bounds.y() << "\t" << (bounds.x()+bounds.width()) << "x" << (bounds.y()+bounds.height()) );
                SG_LOG(SG_ATC, SG_ALERT , "Pos : " << pos.x() << "\t" << pos.y() );
                return false;
            }
            return rootNode.get()->add(getBoxFunction(value), value, equalFunction, getBoxFunction);
        }
        return true;
    }

    bool move(const SGRectd& newPos, SGSharedPtr<T> value)
    {
        /*
        bool found = rootNode.get()->printPath(getBoxFunction(value), value, equalFunction, "Start/");
        if (!found) {
            rootNode.get()->findFullScan(value, equalFunction, "Error/");
        }
        */
        rootNode.get()->move(newPos, getBoxFunction(value), value, equalFunction, getBoxFunction);
        /*
        rootNode.get()->printPath(newPos, value, equalFunction, "End/");
        */
        return true;
    }

    bool remove(SGSharedPtr<T> value)
    {
        return rootNode.get()->remove(getBoxFunction(value), value, equalFunction);
    }

    bool printPath(SGSharedPtr<T> value)
    {
        return rootNode.get()->printPath(getBoxFunction(value), value, equalFunction, "/");
    }

    void query(SGSharedPtr<T> value, std::vector<SGSharedPtr<T>>& values)
    {
        return rootNode.get()->query(getBoxFunction(value), getBoxFunction, values);
    }

    void query(const SGRectd& queryBox, std::vector<SGSharedPtr<T>>& values)
    {
        return rootNode.get()->query(queryBox, getBoxFunction, values);
    }

    size_t size(){return rootNode.get()->size();}
};
}


