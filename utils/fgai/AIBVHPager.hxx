// Copyright (C) 2009 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef _AIBVHPAGER_HXX
#define _AIBVHPAGER_HXX

#include <string>
#include <simgear/bvh/BVHNode.hxx>
#include <simgear/bvh/BVHPager.hxx>

namespace fgai {

class AIBVHPager : public simgear::BVHPager {
public:
    AIBVHPager();
    ~AIBVHPager();

    /// Load the flightgear scenery into the pager
    void setScenery(const std::string& fg_root, const std::string& fg_scenery);

    /// Get a bounding volume subtree contained in sphere.
    /// This is similar to the not so well known ground cache.
    SGSharedPtr<simgear::BVHNode> getBoundingVolumes(const SGSphered& sphere);

private:
    class ReadFileCallback;
    class SubTreeCollector;

    /// The possibly paged root node
    SGSharedPtr<simgear::BVHNode> _node;
};

} // namespace fgai

#endif
