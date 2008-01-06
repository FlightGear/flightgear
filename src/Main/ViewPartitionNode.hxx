// Copyright (C) 2008  Tim Moore
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

#ifndef VIEWPARTITIONNODE_HXX
#define VIEWPARTITIONNODE_HXX 1
// The ViewPartitionNode splits the viewing frustum inherited from a
// camera higher in the scene graph (usually the Viewer master or
// slave camera) into 3 parts: from the parent near plane to a
// intermediate far plane (100m), then out to the current visibility,
// and then beyond where background stuff is rendered. The depth
// buffer and depth testing are disabled for the background.

#include <osg/Camera>
#include <osg/Group>
#include <osg/Matrix>

class ViewPartitionNode : public osg::Group
{
public:
    ViewPartitionNode();
    ViewPartitionNode(const ViewPartitionNode& rhs,
                      const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
    META_Node(flightgear, ViewPartitionNode);

    /** Catch child management functions so the Cameras can be informed
        of added or removed children. */
    virtual bool addChild(osg::Node *child);
    virtual bool insertChild(unsigned int index, osg::Node *child);
    virtual bool removeChildren(unsigned int pos, unsigned int numRemove = 1);
    virtual bool setChild(unsigned int i, osg::Node *node);
    virtual void traverse(osg::NodeVisitor &nv);
    void setVisibility(double vis) { visibility = vis; }
    double getVisibility() const { return visibility; }
    static void makeNewProjMat(osg::Matrixd& oldProj, double znear, double zfar,
                               osg::Matrixd& projection);
protected:

    typedef std::vector< osg::ref_ptr<osg::Camera> > CameraList;
    CameraList cameras;
    enum CameraNum {
        BACKGROUND_CAMERA = 0,
        FAR_CAMERA = 1,
        NEAR_CAMERA = 2
    };
    double visibility;
    static const double nearCameraFar;
};
#endif
