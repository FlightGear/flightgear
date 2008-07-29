// Copyright (C) 2008 Tim Moore
//
// Much of this file was copied directly from the osgdepthpartition
// example in the OpenSceneGraph distribution, so I feel it is right
// to preserve the license in that code:
/*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

#include <osg/CullSettings>
#include <osg/Depth>
#include <osg/StateSet>
#include <osgUtil/CullVisitor>
// For DotOsgWrapperProxy
#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/Output>

#include <simgear/scene/util/RenderConstants.hxx>
#include "ViewPartitionNode.hxx"

using namespace osg;

const double ViewPartitionNode::nearCameraFar = 100.0;
ViewPartitionNode::ViewPartitionNode():
    cameras(2), visibility(40000.0)
{
    const GLbitfield inheritanceMask = (CullSettings::ALL_VARIABLES
                                        & ~CullSettings::COMPUTE_NEAR_FAR_MODE
                                        & ~CullSettings::NEAR_FAR_RATIO
                                        & ~CullSettings::CULLING_MODE
                                        & ~CullSettings::CULL_MASK);
    int i = 1;
    for (CameraList::iterator iter = cameras.begin();
         iter != cameras.end();
         ++iter, ++i) {
        Camera* camera = new Camera;
        camera->setReferenceFrame(Transform::RELATIVE_RF);
        camera->setInheritanceMask(inheritanceMask);
        camera->setComputeNearFarMode(CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        camera->setCullingMode(CullSettings::VIEW_FRUSTUM_CULLING);
        camera->setRenderOrder(Camera::POST_RENDER, i);
        *iter = camera;
    }

    cameras[NEAR_CAMERA]->setClearMask(GL_DEPTH_BUFFER_BIT);
    // Background camera will have cleared the buffers and doesn't
    // touch the depth buffer
    cameras[FAR_CAMERA]->setClearMask(GL_DEPTH_BUFFER_BIT);
    // near camera shouldn't render the background.
    cameras[NEAR_CAMERA]->setCullMask(cameras[NEAR_CAMERA]->getCullMask()
                                      & ~simgear::BACKGROUND_BIT);
}

ViewPartitionNode::ViewPartitionNode(const ViewPartitionNode& rhs,
                                     const CopyOp& copyop):
cameras(2), visibility(rhs.visibility)
{
    for (int i = 0; i < 2; i++)
        cameras[i] = static_cast<Camera*>(copyop(rhs.cameras[i].get()));
}

void ViewPartitionNode::traverse(NodeVisitor& nv)
{
    osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(&nv);
    if(!cv) { 
        Group::traverse(nv);
        return; 
    }
    RefMatrix& modelview = *(cv->getModelViewMatrix());
    RefMatrix& projection = *(cv->getProjectionMatrix());
    // Get the frustum of the enclosing camera. The ViewPartitionNode
    // will divide up the frustum between that camera's near and far
    // planes i.e., parentNear and ParentFar.
    double left, right, bottom, top, parentNear, parentFar;
    projection.getFrustum(left, right, bottom, top, parentNear, parentFar);
    double nearPlanes[2], farPlanes[2];
    nearPlanes[0] = nearCameraFar;
    farPlanes[0] = parentFar;
    nearPlanes[1] = parentNear;
    farPlanes[1] = nearCameraFar;
    
    for (int i = 0; i < 2; ++i) {
        if (farPlanes[i] >0.0) {
            ref_ptr<RefMatrix> newProj = new RefMatrix();
            makeNewProjMat(projection, nearPlanes[i], farPlanes[i],
                           *newProj.get());
            cv->pushProjectionMatrix(newProj.get());
            cameras[i]->accept(nv);
            cv->popProjectionMatrix();
        }
    }
}

bool ViewPartitionNode::addChild(osg::Node *child)
{
    return insertChild(_children.size(), child);
}

bool ViewPartitionNode::insertChild(unsigned int index, osg::Node *child)
{
    if(!Group::insertChild(index, child))
        return false;
    // Insert child into each Camera

    for (CameraList::iterator iter = cameras.begin();
         iter != cameras.end();
         ++iter) {
        (*iter)->insertChild(index, child);
    }
    return true;
}

bool ViewPartitionNode::removeChildren(unsigned int pos, unsigned int numRemove)
{
    if (!Group::removeChildren(pos, numRemove))
        return false;

    // Remove child from each Camera

    for (CameraList::iterator iter = cameras.begin();
         iter != cameras.end();
         ++iter) {
        (*iter)->removeChildren(pos, numRemove);
    }
    return true;
}

bool ViewPartitionNode::setChild(unsigned int i, osg::Node *node)
{
    if(!Group::setChild(i, node)) return false; // Set child

    // Set child for each Camera
    for (CameraList::iterator iter = cameras.begin();
         iter != cameras.end();
         ++iter) {
        (*iter)->setChild(i, node);
    }
    return true;
}

// Given a projection matrix, return a new one with the same frustum
// sides and new near / far values.

void ViewPartitionNode::makeNewProjMat(Matrixd& oldProj, double znear,
                                       double zfar, Matrixd& projection)
{
    projection = oldProj;
    // Slightly inflate the near & far planes to avoid objects at the
    // extremes being clipped out.
    znear *= 0.999;
    zfar *= 1.001;

    // Clamp the projection matrix z values to the range (near, far)
    double epsilon = 1.0e-6;
    if (fabs(projection(0,3)) < epsilon &&
        fabs(projection(1,3)) < epsilon &&
        fabs(projection(2,3)) < epsilon) {
        // Projection is Orthographic
        epsilon = -1.0/(zfar - znear); // Used as a temp variable
        projection(2,2) = 2.0*epsilon;
        projection(3,2) = (zfar + znear)*epsilon;
    } else {
        // Projection is Perspective
        double trans_near = (-znear*projection(2,2) + projection(3,2)) /
            (-znear*projection(2,3) + projection(3,3));
        double trans_far = (-zfar*projection(2,2) + projection(3,2)) /
            (-zfar*projection(2,3) + projection(3,3));
        double ratio = fabs(2.0/(trans_near - trans_far));
        double center = -0.5*(trans_near + trans_far);

        projection.postMult(osg::Matrixd(1.0, 0.0, 0.0, 0.0,
                                         0.0, 1.0, 0.0, 0.0,
                                         0.0, 0.0, ratio, 0.0,
                                         0.0, 0.0, center*ratio, 1.0));
    }

}

namespace
{
bool ViewPartitionNode_readLocalData(osg::Object& obj, osgDB::Input& fr)
{
    ViewPartitionNode& node = static_cast<ViewPartitionNode&>(obj);
    if (fr[0].matchWord("visibility")) {
        ++fr;
        double visibility;
        if (fr[0].getFloat(visibility))
            ++fr;
        else
            return false;
        node.setVisibility(visibility);
    }
    return true;
}

bool ViewPartitionNode_writeLocalData(const Object& obj, osgDB::Output& fw)
{
    const ViewPartitionNode& node = static_cast<const ViewPartitionNode&>(obj);
    fw.indent() << "visibility " << node.getVisibility() << std::endl;
    return true;
}

osgDB::RegisterDotOsgWrapperProxy viewPartitionNodeProxy
(
    new ViewPartitionNode,
    "ViewPartitionNode",
    "Object Node ViewPartitionNode Group",
    &ViewPartitionNode_readLocalData,
    &ViewPartitionNode_writeLocalData
    );
}
