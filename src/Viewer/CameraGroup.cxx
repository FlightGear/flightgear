// Copyright (C) 2008  Tim Moore
// Copyright (C) 2011  Mathias Froehlich
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "CameraGroup.hxx"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include "renderer.hxx"
#include "FGEventHandler.hxx"
#include "WindowBuilder.hxx"
#include "WindowSystemAdapter.hxx"
#include <simgear/props/props.hxx>
#include <simgear/structure/OSGUtils.hxx>
#include <simgear/structure/OSGVersion.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/util/RenderConstants.hxx>

#include <algorithm>
#include <cstring>
#include <string>

#include <osg/Camera>
#include <osg/Geometry>
#include <osg/GraphicsContext>
#include <osg/io_utils>
#include <osg/Math>
#include <osg/Matrix>
#include <osg/Notify>
#include <osg/Program>
#include <osg/Quat>
#include <osg/TexMat>
#include <osg/Vec3d>
#include <osg/Viewport>

#include <osgUtil/IntersectionVisitor>

#include <osgViewer/GraphicsWindow>
#include <osgViewer/Renderer>

namespace flightgear {
const char* MAIN_CAMERA = "main";
const char* FAR_CAMERA = "far";
const char* GEOMETRY_CAMERA = "geometry";
const char* SHADOW_CAMERA = "shadow";
const char* LIGHTING_CAMERA = "lighting";
const char* DISPLAY_CAMERA = "display";
}

static osg::Matrix
invert(const osg::Matrix& matrix)
{
    return osg::Matrix::inverse(matrix);
}

/// Returns the zoom factor of the master camera.
/// The reference fov is the historic 55 deg
static double
zoomFactor()
{
    double fov = fgGetDouble("/sim/current-view/field-of-view", 55);
    if (fov < 1)
        fov = 1;
    return tan(55*0.5*SG_DEGREES_TO_RADIANS)/tan(fov*0.5*SG_DEGREES_TO_RADIANS);
}

static osg::Vec2d
preMult(const osg::Vec2d& v, const osg::Matrix& m)
{
  osg::Vec3d tmp = m.preMult(osg::Vec3(v, 0));
  return osg::Vec2d(tmp[0], tmp[1]);
}

static osg::Matrix
relativeProjection(const osg::Matrix& P0, const osg::Matrix& R, const osg::Vec2d ref[2],
                   const osg::Matrix& pP, const osg::Matrix& pR, const osg::Vec2d pRef[2])
{
  // Track the way from one projection space to the other:
  // We want
  //  P = T*S*P0
  // where P0 is the projection template sensible for the given window size,
  // T is a translation matrix and S a scale matrix.
  // We need to determine T and S so that the reference points in the parents
  // projection space match the two reference points in this cameras projection space.

  // Starting from the parents camera projection space, we get into this cameras
  // projection space by the transform matrix:
  //  P*R*inv(pP*pR) = T*S*P0*R*inv(pP*pR)
  // So, at first compute that matrix without T*S and determine S and T from that

  // Ok, now osg uses the inverse matrix multiplication order, thus:
  osg::Matrix PtoPwithoutTS = invert(pR*pP)*R*P0;
  // Compute the parents reference points in the current projection space
  // without the yet unknown T and S
  osg::Vec2d pRefInThis[2] = {
    preMult(pRef[0], PtoPwithoutTS),
    preMult(pRef[1], PtoPwithoutTS)
  };

  // To get the same zoom, rescale to match the parents size
  double s = (ref[0] - ref[1]).length()/(pRefInThis[0] - pRefInThis[1]).length();
  osg::Matrix S = osg::Matrix::scale(s, s, 1);

  // For the translation offset, incorporate the now known scale
  // and recompute the position ot the first reference point in the
  // currents projection space without the yet unknown T.
  pRefInThis[0] = preMult(pRef[0], PtoPwithoutTS*S);
  // The translation is then the difference of the reference points
  osg::Matrix T = osg::Matrix::translate(osg::Vec3d(ref[0] - pRefInThis[0], 0));

  // Compose and return the desired final projection matrix
  return P0*S*T;
}

namespace flightgear
{
using namespace osg;

using std::strcmp;
using std::string;

ref_ptr<CameraGroup> CameraGroup::_defaultGroup;

CameraGroup::CameraGroup(osgViewer::Viewer* viewer) :
    _viewer(viewer)
{
}

}

namespace
{
using namespace osg;

// Given a projection matrix, return a new one with the same frustum
// sides and new near / far values.

void makeNewProjMat(Matrixd& oldProj, double znear,
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
}

namespace flightgear
{
void CameraInfo::updateCameras()
{
    bufferSize->set( osg::Vec2f( width, height ) );

    for (CameraMap::iterator ii = cameras.begin(); ii != cameras.end(); ++ii ) {
        float f = ii->second.scaleFactor;
        if ( f == 0.0f ) continue;

        if (ii->second.camera->getRenderTargetImplementation() == osg::Camera::FRAME_BUFFER_OBJECT)
            ii->second.camera->getViewport()->setViewport(0, 0, width*f, height*f);
        else
            ii->second.camera->getViewport()->setViewport(x*f, y*f, width*f, height*f);
    }

    for (RenderBufferMap::iterator ii = buffers.begin(); ii != buffers.end(); ++ii ) {
        float f = ii->second.scaleFactor;
        if ( f == 0.0f ) continue;
        osg::Texture2D* texture = ii->second.texture.get();
        if ( texture->getTextureHeight() != height*f || texture->getTextureWidth() != width*f ) {
            texture->setTextureSize( width*f, height*f );
            texture->dirtyTextureObject();
        }
    }
}

void CameraInfo::resized(double w, double h)
{
    if (w == 1.0 && h == 1.0)
        return;

    bufferSize->set( osg::Vec2f( w, h ) );

    for (RenderBufferMap::iterator ii = buffers.begin(); ii != buffers.end(); ++ii) {
        float s = ii->second.scaleFactor;
        if ( s == 0.0f ) continue;
        ii->second.texture->setTextureSize( w * s, h * s );
        ii->second.texture->dirtyTextureObject();
    }

    for (CameraMap::iterator ii = cameras.begin(); ii != cameras.end(); ++ii) {
        RenderStageInfo& rsi = ii->second;
        if (!rsi.resizable ||
                rsi.camera->getRenderTargetImplementation() != osg::Camera::FRAME_BUFFER_OBJECT ||
                rsi.scaleFactor == 0.0f )
            continue;

        Viewport* vp = rsi.camera->getViewport();
        vp->width() = w * rsi.scaleFactor;
        vp->height() = h * rsi.scaleFactor;

        osgViewer::Renderer* renderer
            = static_cast<osgViewer::Renderer*>(rsi.camera->getRenderer());
        for (int i = 0; i < 2; ++i) {
            osgUtil::SceneView* sceneView = renderer->getSceneView(i);
            sceneView->getRenderStage()->setFrameBufferObject(0);
            sceneView->getRenderStage()->setCameraRequiresSetUp(true);
            if (sceneView->getRenderStageLeft()) {
                sceneView->getRenderStageLeft()->setFrameBufferObject(0);
                sceneView->getRenderStageLeft()->setCameraRequiresSetUp(true);
            }
            if (sceneView->getRenderStageRight()) {
                sceneView->getRenderStageRight()->setFrameBufferObject(0);
                sceneView->getRenderStageRight()->setCameraRequiresSetUp(true);
            }
        }
    }
}

osg::Camera* CameraInfo::getCamera(const std::string& k) const
{
    CameraMap::const_iterator ii = cameras.find( k );
    if (ii == cameras.end())
        return 0;
    return ii->second.camera.get();
}

osg::Texture2D* CameraInfo::getBuffer(const std::string& k) const
{
    RenderBufferMap::const_iterator ii = buffers.find(k);
    if (ii == buffers.end())
        return 0;
    return ii->second.texture.get();
}

int CameraInfo::getMainSlaveIndex() const
{
    return cameras.find( MAIN_CAMERA )->second.slaveIndex;
}

void CameraInfo::setMatrices(osg::Camera* c)
{
    view->set( c->getViewMatrix() );
    osg::Matrixd vi = c->getInverseViewMatrix();
    viewInverse->set( vi );
    projInverse->set( osg::Matrix::inverse( c->getProjectionMatrix() ) );
    osg::Vec4d pos = osg::Vec4d(0., 0., 0., 1.) * vi;
    worldPosCart->set( osg::Vec3f( pos.x(), pos.y(), pos.z() ) );
    SGGeod pos2 = SGGeod::fromCart( SGVec3d( pos.x(), pos.y(), pos.z() ) );
    worldPosGeod->set( osg::Vec3f( pos2.getLongitudeRad(), pos2.getLatitudeRad(), pos2.getElevationM() ) );
}

void CameraGroup::update(const osg::Vec3d& position,
                         const osg::Quat& orientation)
{
    const Matrix masterView(osg::Matrix::translate(-position)
                            * osg::Matrix::rotate(orientation.inverse()));
    _viewer->getCamera()->setViewMatrix(masterView);
    const Matrix& masterProj = _viewer->getCamera()->getProjectionMatrix();
    double masterZoomFactor = zoomFactor();
    for (CameraList::iterator i = _cameras.begin(); i != _cameras.end(); ++i) {
        const CameraInfo* info = i->get();

        Camera* camera = info->getCamera(MAIN_CAMERA);
        if ( camera ) {
            const View::Slave& slave = _viewer->getSlave(info->getMainSlaveIndex());
#if SG_OSG_VERSION_LESS_THAN(3,0,0)
            // refreshes camera viewports (for now)
            info->updateCameras();
#endif
            Matrix viewMatrix;
            if (info->flags & GUI) {
                viewMatrix = osg::Matrix(); // identifty transform on the GUI camera
            } else if ((info->flags & VIEW_ABSOLUTE) != 0)
                viewMatrix = slave._viewOffset;
            else
                viewMatrix = masterView * slave._viewOffset;
            camera->setViewMatrix(viewMatrix);
            Matrix projectionMatrix;
            if (info->flags & GUI) {
                projectionMatrix = osg::Matrix::ortho2D(0, info->width, 0, info->height);
            } else if ((info->flags & PROJECTION_ABSOLUTE) != 0) {
                if (info->flags & ENABLE_MASTER_ZOOM) {
                    if (info->relativeCameraParent < _cameras.size()) {
                        // template projection matrix and view matrix of the current camera
                        osg::Matrix P0 = slave._projectionOffset;
                        osg::Matrix R = viewMatrix;

                        // The already known projection and view matrix of the parent camera
                        const CameraInfo* parentInfo = _cameras[info->relativeCameraParent].get();
                        RenderStageInfo prsi = parentInfo->cameras.find(MAIN_CAMERA)->second;
                        osg::Matrix pP = prsi.camera->getProjectionMatrix();
                        osg::Matrix pR = prsi.camera->getViewMatrix();
                    
                        // And the projection matrix derived from P0 so that the reference points match
                        projectionMatrix = relativeProjection(P0, R, info->thisReference,
                                                              pP, pR, info->parentReference);
                    
                    } else {
                        // We want to zoom, so take the original matrix and apply the zoom to it.
                        projectionMatrix = slave._projectionOffset;
                        projectionMatrix.postMultScale(osg::Vec3d(masterZoomFactor, masterZoomFactor, 1));
                    }
                } else {
                    projectionMatrix = slave._projectionOffset;
                }
            } else {
                projectionMatrix = masterProj * slave._projectionOffset;
            }

            CameraMap::const_iterator ii = info->cameras.find(FAR_CAMERA);
            if (ii == info->cameras.end() || !ii->second.camera.valid()) {
                camera->setProjectionMatrix(projectionMatrix);
            } else {
                Camera* farCamera = ii->second.camera;
                farCamera->setViewMatrix(viewMatrix);
                double left, right, bottom, top, parentNear, parentFar;
                projectionMatrix.getFrustum(left, right, bottom, top,
                                            parentNear, parentFar);
                if ((info->flags & FIXED_NEAR_FAR) == 0) {
                    parentNear = _zNear;
                    parentFar = _zFar;
                }
                if (parentFar < _nearField || _nearField == 0.0f) {
                    camera->setProjectionMatrix(projectionMatrix);
                    camera->setCullMask(camera->getCullMask()
                                        | simgear::BACKGROUND_BIT);
                    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                    farCamera->setNodeMask(0);
                } else {
                    Matrix nearProj, farProj;
                    makeNewProjMat(projectionMatrix, parentNear, _nearField,
                                   nearProj);
                    makeNewProjMat(projectionMatrix, _nearField, parentFar,
                                   farProj);
                    camera->setProjectionMatrix(nearProj);
                    camera->setCullMask(camera->getCullMask()
                                        & ~simgear::BACKGROUND_BIT);
                    camera->setClearMask(GL_DEPTH_BUFFER_BIT);
                    farCamera->setProjectionMatrix(farProj);
                    farCamera->setNodeMask(camera->getNodeMask());
                }
            }
        } else {
            bool viewDone = false;
            Matrix viewMatrix;
            bool projectionDone = false;
            Matrix projectionMatrix;
            for ( CameraMap::const_iterator ii = info->cameras.begin(); ii != info->cameras.end(); ++ii ) {
                if ( ii->first == SHADOW_CAMERA ) {
                    globals->get_renderer()->updateShadowCamera(info, position);
                    continue;
                }
                if ( ii->second.fullscreen )
                    continue;

                Camera* camera = ii->second.camera.get();
                int slaveIndex = ii->second.slaveIndex;
                const View::Slave& slave = _viewer->getSlave(slaveIndex);

                if ( !viewDone ) {
                    if ((info->flags & VIEW_ABSOLUTE) != 0)
                        viewMatrix = slave._viewOffset;
                    else
                        viewMatrix = masterView * slave._viewOffset;
                    viewDone = true;
                }

                camera->setViewMatrix( viewMatrix );

                if ( !projectionDone ) {
                    if ((info->flags & PROJECTION_ABSOLUTE) != 0) {
                        if (info->flags & ENABLE_MASTER_ZOOM) {
                            if (info->relativeCameraParent < _cameras.size()) {
                                // template projection matrix and view matrix of the current camera
                                osg::Matrix P0 = slave._projectionOffset;
                                osg::Matrix R = viewMatrix;

                                // The already known projection and view matrix of the parent camera
                                const CameraInfo* parentInfo = _cameras[info->relativeCameraParent].get();
                                RenderStageInfo prsi = parentInfo->cameras.find(MAIN_CAMERA)->second;
                                osg::Matrix pP = prsi.camera->getProjectionMatrix();
                                osg::Matrix pR = prsi.camera->getViewMatrix();
                    
                                // And the projection matrix derived from P0 so that the reference points match
                                projectionMatrix = relativeProjection(P0, R, info->thisReference,
                                                                      pP, pR, info->parentReference);
                    
                            } else {
                                // We want to zoom, so take the original matrix and apply the zoom to it.
                                projectionMatrix = slave._projectionOffset;
                                projectionMatrix.postMultScale(osg::Vec3d(masterZoomFactor, masterZoomFactor, 1));
                            }
                        } else {
                            projectionMatrix = slave._projectionOffset;
                        }
                    } else {
                        projectionMatrix = masterProj * slave._projectionOffset;
                    }
                    projectionDone = true;
                }

                camera->setProjectionMatrix(projectionMatrix);
            }
        }
    }

    globals->get_renderer()->setPlanes( _zNear, _zFar );
}

void CameraGroup::setCameraParameters(float vfov, float aspectRatio)
{
    if (vfov != 0.0f && aspectRatio != 0.0f)
        _viewer->getCamera()
            ->setProjectionMatrixAsPerspective(vfov,
                                               1.0f / aspectRatio,
                                               _zNear, _zFar);
}
    
double CameraGroup::getMasterAspectRatio() const
{
    if (_cameras.empty())
        return 0.0;
    
    const CameraInfo* info = _cameras.front();
    
    osg::Camera* camera = info->getCamera(MAIN_CAMERA);
    if ( !camera )
        camera = info->getCamera( GEOMETRY_CAMERA );
    const osg::Viewport* viewport = camera->getViewport();
    if (!viewport) {
        return 0.0;
    }
    
    return static_cast<double>(viewport->height()) / viewport->width();
}
    
}

namespace
{
// A raw value for property nodes that references a class member via
// an osg::ref_ptr.
template<class C, class T>
class RefMember : public SGRawValue<T>
{
public:
    RefMember (C *obj, T C::*ptr)
        : _obj(obj), _ptr(ptr) {}
    virtual ~RefMember () {}
    virtual T getValue () const
    {
        return _obj.get()->*_ptr;
    }
    virtual bool setValue (T value)
    {
        _obj.get()->*_ptr = value;
        return true;
    }
    virtual SGRawValue<T> * clone () const
    {
        return new RefMember(_obj.get(), _ptr);
    }
private:
    ref_ptr<C> _obj;
    T C::* const _ptr;
};

template<typename C, typename T>
RefMember<C, T> makeRefMember(C *obj, T C::*ptr)
{
    return RefMember<C, T>(obj, ptr);
}

template<typename C, typename T>
void bindMemberToNode(SGPropertyNode* parent, const char* childName,
                      C* obj, T C::*ptr, T value)
{
    SGPropertyNode* valNode = parent->getNode(childName);
    RefMember<C, T> refMember = makeRefMember(obj, ptr);
    if (!valNode) {
        valNode = parent->getNode(childName, true);
        valNode->tie(refMember, false);
        setValue(valNode, value);
    } else {
        valNode->tie(refMember, true);
    }
}

void buildViewport(flightgear::CameraInfo* info, SGPropertyNode* viewportNode,
                   const osg::GraphicsContext::Traits *traits)
{
    using namespace flightgear;
    bindMemberToNode(viewportNode, "x", info, &CameraInfo::x, 0.0);
    bindMemberToNode(viewportNode, "y", info, &CameraInfo::y, 0.0);
    bindMemberToNode(viewportNode, "width", info, &CameraInfo::width,
                     static_cast<double>(traits->width));
    bindMemberToNode(viewportNode, "height", info, &CameraInfo::height,
                     static_cast<double>(traits->height));
}
}

namespace flightgear
{

// Mostly copied from osg's osgViewer/View.cpp

static osg::Geometry* createPanoramicSphericalDisplayDistortionMesh(
    const Vec3& origin, const Vec3& widthVector, const Vec3& heightVector,
    double sphere_radius, double collar_radius,
    Image* intensityMap = 0, const Matrix& projectorMatrix = Matrix())
{
    osg::Vec3d center(0.0,0.0,0.0);
    osg::Vec3d eye(0.0,0.0,0.0);

    double distance = sqrt(sphere_radius*sphere_radius - collar_radius*collar_radius);
    bool flip = false;
    bool texcoord_flip = false;

#if 0
    osg::Vec3d projector = eye - osg::Vec3d(0.0,0.0, distance);

    OSG_INFO<<"createPanoramicSphericalDisplayDistortionMesh : Projector position = "<<projector<<std::endl;
    OSG_INFO<<"createPanoramicSphericalDisplayDistortionMesh : distance = "<<distance<<std::endl;
#endif
    // create the quad to visualize.
    osg::Geometry* geometry = new osg::Geometry();

    geometry->setSupportsDisplayList(false);

    osg::Vec3 xAxis(widthVector);
    float width = widthVector.length();
    xAxis /= width;

    osg::Vec3 yAxis(heightVector);
    float height = heightVector.length();
    yAxis /= height;

    int noSteps = 160;

    osg::Vec3Array* vertices = new osg::Vec3Array;
    osg::Vec2Array* texcoords0 = new osg::Vec2Array;
    osg::Vec2Array* texcoords1 = intensityMap==0 ? new osg::Vec2Array : 0;
    osg::Vec4Array* colors = new osg::Vec4Array;

#if 0
    osg::Vec3 bottom = origin;
    osg::Vec3 dx = xAxis*(width/((float)(noSteps-2)));
    osg::Vec3 dy = yAxis*(height/((float)(noSteps-1)));
#endif
    osg::Vec3 top = origin + yAxis*height;

    osg::Vec3 screenCenter = origin + widthVector*0.5f + heightVector*0.5f;
    float screenRadius = heightVector.length() * 0.5f;

    geometry->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);

    for(int i=0;i<noSteps;++i)
    {
        //osg::Vec3 cursor = bottom+dy*(float)i;
        for(int j=0;j<noSteps;++j)
        {
            osg::Vec2 texcoord(double(i)/double(noSteps-1), double(j)/double(noSteps-1));
            double theta = texcoord.x() * 2.0 * osg::PI;
            double phi = (1.0-texcoord.y()) * osg::PI;

            if (texcoord_flip) texcoord.y() = 1.0f - texcoord.y();

            osg::Vec3 pos(sin(phi)*sin(theta), sin(phi)*cos(theta), cos(phi));
            pos = pos*projectorMatrix;

            double alpha = atan2(pos.x(), pos.y());
            if (alpha<0.0) alpha += 2.0*osg::PI;

            double beta = atan2(sqrt(pos.x()*pos.x() + pos.y()*pos.y()), pos.z());
            if (beta<0.0) beta += 2.0*osg::PI;

            double gamma = atan2(sqrt(double(pos.x()*pos.x() + pos.y()*pos.y())), double(pos.z()+distance));
            if (gamma<0.0) gamma += 2.0*osg::PI;


            osg::Vec3 v = screenCenter + osg::Vec3(sin(alpha)*gamma*2.0/osg::PI, -cos(alpha)*gamma*2.0/osg::PI, 0.0f)*screenRadius;

            if (flip)
                vertices->push_back(osg::Vec3(v.x(), top.y()-(v.y()-origin.y()),v.z()));
            else
                vertices->push_back(v);

            texcoords0->push_back( texcoord );

            osg::Vec2 texcoord1(alpha/(2.0*osg::PI), 1.0f - beta/osg::PI);
            if (intensityMap)
            {
                colors->push_back(intensityMap->getColor(texcoord1));
            }
            else
            {
                colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
                if (texcoords1) texcoords1->push_back( texcoord1 );
            }


        }
    }


    // pass the created vertex array to the points geometry object.
    geometry->setVertexArray(vertices);

    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    geometry->setTexCoordArray(0,texcoords0);
    if (texcoords1) geometry->setTexCoordArray(1,texcoords1);

    osg::DrawElementsUShort* elements = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES);
    geometry->addPrimitiveSet(elements);

    for(int i=0;i<noSteps-1;++i)
    {
        for(int j=0;j<noSteps-1;++j)
        {
            int i1 = j+(i+1)*noSteps;
            int i2 = j+(i)*noSteps;
            int i3 = j+1+(i)*noSteps;
            int i4 = j+1+(i+1)*noSteps;

            osg::Vec3& v1 = (*vertices)[i1];
            osg::Vec3& v2 = (*vertices)[i2];
            osg::Vec3& v3 = (*vertices)[i3];
            osg::Vec3& v4 = (*vertices)[i4];

            if ((v1-screenCenter).length()>screenRadius) continue;
            if ((v2-screenCenter).length()>screenRadius) continue;
            if ((v3-screenCenter).length()>screenRadius) continue;
            if ((v4-screenCenter).length()>screenRadius) continue;

            elements->push_back(i1);
            elements->push_back(i2);
            elements->push_back(i3);

            elements->push_back(i1);
            elements->push_back(i3);
            elements->push_back(i4);
        }
    }

    return geometry;
}

void CameraGroup::buildDistortionCamera(const SGPropertyNode* psNode,
                                        Camera* camera)
{
    const SGPropertyNode* texNode = psNode->getNode("texture");
    if (!texNode) {
        // error
        return;
    }
    string texName = texNode->getStringValue();
    TextureMap::iterator itr = _textureTargets.find(texName);
    if (itr == _textureTargets.end()) {
        // error
        return;
    }
    Viewport* viewport = camera->getViewport();
    float width = viewport->width();
    float height = viewport->height();
    TextureRectangle* texRect = itr->second.get();
    double radius = psNode->getDoubleValue("radius", 1.0);
    double collar = psNode->getDoubleValue("collar", 0.45);
    Geode* geode = new Geode();
    geode->addDrawable(createPanoramicSphericalDisplayDistortionMesh(
                           Vec3(0.0f,0.0f,0.0f), Vec3(width,0.0f,0.0f),
                           Vec3(0.0f,height,0.0f), radius, collar));

    // new we need to add the texture to the mesh, we do so by creating a
    // StateSet to contain the Texture StateAttribute.
    StateSet* stateset = geode->getOrCreateStateSet();
    stateset->setTextureAttributeAndModes(0, texRect, StateAttribute::ON);
    stateset->setMode(GL_LIGHTING, StateAttribute::OFF);

    TexMat* texmat = new TexMat;
    texmat->setScaleByTextureRectangleSize(true);
    stateset->setTextureAttributeAndModes(0, texmat, osg::StateAttribute::ON);
#if 0
    if (!applyIntensityMapAsColours && intensityMap)
    {
        stateset->setTextureAttributeAndModes(1, new osg::Texture2D(intensityMap), osg::StateAttribute::ON);
    }
#endif
    // add subgraph to render
    camera->addChild(geode);
    camera->setClearMask(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->setCullingMode(osg::CullSettings::NO_CULLING);
    camera->setName("DistortionCorrectionCamera");
}

CameraInfo* CameraGroup::buildCamera(SGPropertyNode* cameraNode)
{
    WindowBuilder *wBuild = WindowBuilder::getWindowBuilder();
    const SGPropertyNode* windowNode = cameraNode->getNode("window");
    GraphicsWindow* window = 0;
    int cameraFlags = DO_INTERSECTION_TEST;
    if (windowNode) {
        // New style window declaration / definition
        window = wBuild->buildWindow(windowNode);
    } else {
        // Old style: suck window params out of camera block
        window = wBuild->buildWindow(cameraNode);
    }
    if (!window) {
        return 0;
    }
    Camera* camera = new Camera;
    camera->setAllowEventFocus(false);
    camera->setGraphicsContext(window->gc.get());
    camera->setViewport(new Viewport);
    camera->setCullingMode(CullSettings::SMALL_FEATURE_CULLING
                           | CullSettings::VIEW_FRUSTUM_CULLING);
    camera->setInheritanceMask(CullSettings::ALL_VARIABLES
                               & ~(CullSettings::CULL_MASK
                                   | CullSettings::CULLING_MODE
                                   | CullSettings::CLEAR_MASK
                                   ));

    osg::Matrix vOff;
    const SGPropertyNode* viewNode = cameraNode->getNode("view");
    if (viewNode) {
        double heading = viewNode->getDoubleValue("heading-deg", 0.0);
        double pitch = viewNode->getDoubleValue("pitch-deg", 0.0);
        double roll = viewNode->getDoubleValue("roll-deg", 0.0);
        double x = viewNode->getDoubleValue("x", 0.0);
        double y = viewNode->getDoubleValue("y", 0.0);
        double z = viewNode->getDoubleValue("z", 0.0);
        // Build a view matrix, which is the inverse of a model
        // orientation matrix.
        vOff = (Matrix::translate(-x, -y, -z)
                * Matrix::rotate(-DegreesToRadians(heading),
                                 Vec3d(0.0, 1.0, 0.0),
                                 -DegreesToRadians(pitch),
                                 Vec3d(1.0, 0.0, 0.0),
                                 -DegreesToRadians(roll),
                                 Vec3d(0.0, 0.0, 1.0)));
        if (viewNode->getBoolValue("absolute", false))
            cameraFlags |= VIEW_ABSOLUTE;
    } else {
        // Old heading parameter, works in the opposite direction
        double heading = cameraNode->getDoubleValue("heading-deg", 0.0);
        vOff.makeRotate(DegreesToRadians(heading), osg::Vec3(0, 1, 0));
    }
    // Configuring the physical dimensions of a monitor
    SGPropertyNode* viewportNode = cameraNode->getNode("viewport", true);
    double physicalWidth = viewportNode->getDoubleValue("width", 1024);
    double physicalHeight = viewportNode->getDoubleValue("height", 768);
    double bezelHeightTop = 0;
    double bezelHeightBottom = 0;
    double bezelWidthLeft = 0;
    double bezelWidthRight = 0;
    const SGPropertyNode* physicalDimensionsNode = 0;
    if ((physicalDimensionsNode = cameraNode->getNode("physical-dimensions")) != 0) {
        physicalWidth = physicalDimensionsNode->getDoubleValue("width", physicalWidth);
        physicalHeight = physicalDimensionsNode->getDoubleValue("height", physicalHeight);
        const SGPropertyNode* bezelNode = 0;
        if ((bezelNode = physicalDimensionsNode->getNode("bezel")) != 0) {
            bezelHeightTop = bezelNode->getDoubleValue("top", bezelHeightTop);
            bezelHeightBottom = bezelNode->getDoubleValue("bottom", bezelHeightBottom);
            bezelWidthLeft = bezelNode->getDoubleValue("left", bezelWidthLeft);
            bezelWidthRight = bezelNode->getDoubleValue("right", bezelWidthRight);
        }
    }
    osg::Matrix pOff;
    unsigned parentCameraIndex = ~0u;
    osg::Vec2d parentReference[2];
    osg::Vec2d thisReference[2];
    SGPropertyNode* projectionNode = 0;
    if ((projectionNode = cameraNode->getNode("perspective")) != 0) {
        double fovy = projectionNode->getDoubleValue("fovy-deg", 55.0);
        double aspectRatio = projectionNode->getDoubleValue("aspect-ratio",
                                                            1.0);
        double zNear = projectionNode->getDoubleValue("near", 0.0);
        double zFar = projectionNode->getDoubleValue("far", zNear + 20000);
        double offsetX = projectionNode->getDoubleValue("offset-x", 0.0);
        double offsetY = projectionNode->getDoubleValue("offset-y", 0.0);
        double tan_fovy = tan(DegreesToRadians(fovy*0.5));
        double right = tan_fovy * aspectRatio * zNear + offsetX;
        double left = -tan_fovy * aspectRatio * zNear + offsetX;
        double top = tan_fovy * zNear + offsetY;
        double bottom = -tan_fovy * zNear + offsetY;
        pOff.makeFrustum(left, right, bottom, top, zNear, zFar);
        cameraFlags |= PROJECTION_ABSOLUTE;
        if (projectionNode->getBoolValue("fixed-near-far", true))
            cameraFlags |= FIXED_NEAR_FAR;
    } else if ((projectionNode = cameraNode->getNode("frustum")) != 0
               || (projectionNode = cameraNode->getNode("ortho")) != 0) {
        double top = projectionNode->getDoubleValue("top", 0.0);
        double bottom = projectionNode->getDoubleValue("bottom", 0.0);
        double left = projectionNode->getDoubleValue("left", 0.0);
        double right = projectionNode->getDoubleValue("right", 0.0);
        double zNear = projectionNode->getDoubleValue("near", 0.0);
        double zFar = projectionNode->getDoubleValue("far", zNear + 20000);
        if (cameraNode->getNode("frustum")) {
            pOff.makeFrustum(left, right, bottom, top, zNear, zFar);
            cameraFlags |= PROJECTION_ABSOLUTE;
        } else {
            pOff.makeOrtho(left, right, bottom, top, zNear, zFar);
            cameraFlags |= (PROJECTION_ABSOLUTE | ORTHO);
        }
        if (projectionNode->getBoolValue("fixed-near-far", true))
            cameraFlags |= FIXED_NEAR_FAR;
    } else if ((projectionNode = cameraNode->getNode("master-perspective")) != 0) {
        double zNear = projectionNode->getDoubleValue("eye-distance", 0.4*physicalWidth);
        double xoff = projectionNode->getDoubleValue("x-offset", 0);
        double yoff = projectionNode->getDoubleValue("y-offset", 0);
        double left = -0.5*physicalWidth - xoff;
        double right = 0.5*physicalWidth - xoff;
        double bottom = -0.5*physicalHeight - yoff;
        double top = 0.5*physicalHeight - yoff;
        pOff.makeFrustum(left, right, bottom, top, zNear, zNear*1000);
        cameraFlags |= PROJECTION_ABSOLUTE | ENABLE_MASTER_ZOOM;
    } else if ((projectionNode = cameraNode->getNode("right-of-perspective"))
               || (projectionNode = cameraNode->getNode("left-of-perspective"))
               || (projectionNode = cameraNode->getNode("above-perspective"))
               || (projectionNode = cameraNode->getNode("below-perspective"))
               || (projectionNode = cameraNode->getNode("reference-points-perspective"))) {
        std::string name = projectionNode->getStringValue("parent-camera");
        for (unsigned i = 0; i < _cameras.size(); ++i) {
            if (_cameras[i]->name != name)
                continue;
            parentCameraIndex = i;
        }
        if (_cameras.size() <= parentCameraIndex) {
            SG_LOG(SG_VIEW, SG_ALERT, "CameraGroup::buildCamera: "
                   "failed to find parent camera for relative camera!");
            return 0;
        }
        const CameraInfo* parentInfo = _cameras[parentCameraIndex].get();
        if (projectionNode->getNameString() == "right-of-perspective") {
            double tmp = (parentInfo->physicalWidth + 2*parentInfo->bezelWidthRight)/parentInfo->physicalWidth;
            parentReference[0] = osg::Vec2d(tmp, -1);
            parentReference[1] = osg::Vec2d(tmp, 1);
            tmp = (physicalWidth + 2*bezelWidthLeft)/physicalWidth;
            thisReference[0] = osg::Vec2d(-tmp, -1);
            thisReference[1] = osg::Vec2d(-tmp, 1);
        } else if (projectionNode->getNameString() == "left-of-perspective") {
            double tmp = (parentInfo->physicalWidth + 2*parentInfo->bezelWidthLeft)/parentInfo->physicalWidth;
            parentReference[0] = osg::Vec2d(-tmp, -1);
            parentReference[1] = osg::Vec2d(-tmp, 1);
            tmp = (physicalWidth + 2*bezelWidthRight)/physicalWidth;
            thisReference[0] = osg::Vec2d(tmp, -1);
            thisReference[1] = osg::Vec2d(tmp, 1);
        } else if (projectionNode->getNameString() == "above-perspective") {
            double tmp = (parentInfo->physicalHeight + 2*parentInfo->bezelHeightTop)/parentInfo->physicalHeight;
            parentReference[0] = osg::Vec2d(-1, tmp);
            parentReference[1] = osg::Vec2d(1, tmp);
            tmp = (physicalHeight + 2*bezelHeightBottom)/physicalHeight;
            thisReference[0] = osg::Vec2d(-1, -tmp);
            thisReference[1] = osg::Vec2d(1, -tmp);
        } else if (projectionNode->getNameString() == "below-perspective") {
            double tmp = (parentInfo->physicalHeight + 2*parentInfo->bezelHeightBottom)/parentInfo->physicalHeight;
            parentReference[0] = osg::Vec2d(-1, -tmp);
            parentReference[1] = osg::Vec2d(1, -tmp);
            tmp = (physicalHeight + 2*bezelHeightTop)/physicalHeight;
            thisReference[0] = osg::Vec2d(-1, tmp);
            thisReference[1] = osg::Vec2d(1, tmp);
        } else if (projectionNode->getNameString() == "reference-points-perspective") {
            SGPropertyNode* parentNode = projectionNode->getNode("parent", true);
            SGPropertyNode* thisNode = projectionNode->getNode("this", true);
            SGPropertyNode* pointNode;

            pointNode = parentNode->getNode("point", 0, true);
            parentReference[0][0] = pointNode->getDoubleValue("x", 0)*2/parentInfo->physicalWidth;
            parentReference[0][1] = pointNode->getDoubleValue("y", 0)*2/parentInfo->physicalHeight;
            pointNode = parentNode->getNode("point", 1, true);
            parentReference[1][0] = pointNode->getDoubleValue("x", 0)*2/parentInfo->physicalWidth;
            parentReference[1][1] = pointNode->getDoubleValue("y", 0)*2/parentInfo->physicalHeight;

            pointNode = thisNode->getNode("point", 0, true);
            thisReference[0][0] = pointNode->getDoubleValue("x", 0)*2/physicalWidth;
            thisReference[0][1] = pointNode->getDoubleValue("y", 0)*2/physicalHeight;
            pointNode = thisNode->getNode("point", 1, true);
            thisReference[1][0] = pointNode->getDoubleValue("x", 0)*2/physicalWidth;
            thisReference[1][1] = pointNode->getDoubleValue("y", 0)*2/physicalHeight;
        }

        pOff = osg::Matrix::perspective(45, physicalWidth/physicalHeight, 1, 20000);
        cameraFlags |= PROJECTION_ABSOLUTE | ENABLE_MASTER_ZOOM;
    } else {
        // old style shear parameters
        double shearx = cameraNode->getDoubleValue("shear-x", 0);
        double sheary = cameraNode->getDoubleValue("shear-y", 0);
        pOff.makeTranslate(-shearx, -sheary, 0);
    }
    const SGPropertyNode* textureNode = cameraNode->getNode("texture");
    if (textureNode) {
        string texName = textureNode->getStringValue("name");
        int tex_width = textureNode->getIntValue("width");
        int tex_height = textureNode->getIntValue("height");
        TextureRectangle* texture = new TextureRectangle;

        texture->setTextureSize(tex_width, tex_height);
        texture->setInternalFormat(GL_RGB);
        texture->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
        texture->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        texture->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
        texture->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
        camera->setDrawBuffer(GL_FRONT);
        camera->setReadBuffer(GL_FRONT);
        camera->setRenderTargetImplementation(Camera::FRAME_BUFFER_OBJECT);
        camera->attach(Camera::COLOR_BUFFER, texture);
        _textureTargets[texName] = texture;
    } else {
        camera->setDrawBuffer(GL_BACK);
        camera->setReadBuffer(GL_BACK);
    }
    const SGPropertyNode* psNode = cameraNode->getNode("panoramic-spherical");
    bool useMasterSceneGraph = !psNode;
    CameraInfo* info = globals->get_renderer()->buildRenderingPipeline(this, cameraFlags, camera, vOff, pOff,
                                                                        window->gc.get(), useMasterSceneGraph);
    info->name = cameraNode->getStringValue("name");
    info->physicalWidth = physicalWidth;
    info->physicalHeight = physicalHeight;
    info->bezelHeightTop = bezelHeightTop;
    info->bezelHeightBottom = bezelHeightBottom;
    info->bezelWidthLeft = bezelWidthLeft;
    info->bezelWidthRight = bezelWidthRight;
    info->relativeCameraParent = parentCameraIndex;
    info->parentReference[0] = parentReference[0];
    info->parentReference[1] = parentReference[1];
    info->thisReference[0] = thisReference[0];
    info->thisReference[1] = thisReference[1];
    // If a viewport isn't set on the camera, then it's hard to dig it
    // out of the SceneView objects in the viewer, and the coordinates
    // of mouse events are somewhat bizzare.
    buildViewport(info, viewportNode, window->gc->getTraits());
    info->updateCameras();
    // Distortion camera needs the viewport which is created by addCamera().
    if (psNode) {
        info->flags = info->flags | VIEW_ABSOLUTE;
        buildDistortionCamera(psNode, camera);
    }
    return info;
}

CameraInfo* CameraGroup::buildGUICamera(SGPropertyNode* cameraNode,
                                        GraphicsWindow* window)
{
    WindowBuilder *wBuild = WindowBuilder::getWindowBuilder();
    const SGPropertyNode* windowNode = (cameraNode
                                        ? cameraNode->getNode("window")
                                        : 0);
    if (!window && windowNode) {
      // New style window declaration / definition
      window = wBuild->buildWindow(windowNode);
    }

    if (!window) { // buildWindow can fail
      SG_LOG(SG_VIEW, SG_WARN, "CameraGroup::buildGUICamera: failed to build a window");
      return NULL;
    }

    Camera* camera = new Camera;
    camera->setName( "GUICamera" );
    camera->setAllowEventFocus(false);
    camera->setGraphicsContext(window->gc.get());
    camera->setViewport(new Viewport);
    camera->setClearMask(0);
    camera->setInheritanceMask(CullSettings::ALL_VARIABLES
                               & ~(CullSettings::COMPUTE_NEAR_FAR_MODE
                                   | CullSettings::CULLING_MODE
                                   | CullSettings::CLEAR_MASK
                                   ));
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->setCullingMode(osg::CullSettings::NO_CULLING);
    camera->setProjectionResizePolicy(Camera::FIXED);
    camera->setReferenceFrame(Transform::ABSOLUTE_RF);
    const int cameraFlags = GUI | DO_INTERSECTION_TEST;

    CameraInfo* result = new CameraInfo(cameraFlags);
    // The camera group will always update the camera
    camera->setReferenceFrame(Transform::ABSOLUTE_RF);

    getViewer()->addSlave(camera, Matrixd::identity(), Matrixd::identity(), false);
    //installCullVisitor(camera);
    int slaveIndex = getViewer()->getNumSlaves() - 1;
    result->addCamera( MAIN_CAMERA, camera, slaveIndex );
    camera->setRenderOrder(Camera::POST_RENDER, slaveIndex);
    addCamera(result);

    // XXX Camera needs to be drawn last; eventually the render order
    // should be assigned by a camera manager.
    camera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
    SGPropertyNode* viewportNode = cameraNode->getNode("viewport", true);
    buildViewport(result, viewportNode, window->gc->getTraits());

    // Disable statistics for the GUI camera.
    camera->setStats(0);
    result->updateCameras();
    return result;
}

CameraGroup* CameraGroup::buildCameraGroup(osgViewer::Viewer* viewer,
                                           SGPropertyNode* gnode)
{
    CameraGroup* cgroup = new CameraGroup(viewer);
    for (int i = 0; i < gnode->nChildren(); ++i) {
        SGPropertyNode* pNode = gnode->getChild(i);
        const char* name = pNode->getName();
        if (!strcmp(name, "camera")) {
            cgroup->buildCamera(pNode);
        } else if (!strcmp(name, "window")) {
            WindowBuilder::getWindowBuilder()->buildWindow(pNode);
        } else if (!strcmp(name, "gui")) {
            cgroup->buildGUICamera(pNode);
        }
    }
    bindMemberToNode(gnode, "znear", cgroup, &CameraGroup::_zNear, .1f);
    bindMemberToNode(gnode, "zfar", cgroup, &CameraGroup::_zFar, 120000.0f);
    bindMemberToNode(gnode, "near-field", cgroup, &CameraGroup::_nearField,
                     100.0f);
    return cgroup;
}

void CameraGroup::setCameraCullMasks(Node::NodeMask nm)
{
    for (CameraIterator i = camerasBegin(), e = camerasEnd(); i != e; ++i) {
        CameraInfo* info = i->get();
        if (info->flags & GUI)
            continue;
        osg::ref_ptr<osg::Camera> farCamera = info->getCamera(FAR_CAMERA);
        osg::Camera* camera = info->getCamera( MAIN_CAMERA );
        if (camera) {
            if (farCamera.valid() && farCamera->getNodeMask() != 0) {
                camera->setCullMask(nm & ~simgear::BACKGROUND_BIT);
                camera->setCullMaskLeft(nm & ~simgear::BACKGROUND_BIT);
                camera->setCullMaskRight(nm & ~simgear::BACKGROUND_BIT);
                farCamera->setCullMask(nm);
                farCamera->setCullMaskLeft(nm);
                farCamera->setCullMaskRight(nm);
            } else {
                camera->setCullMask(nm);
                camera->setCullMaskLeft(nm);
                camera->setCullMaskRight(nm);
            }
        } else {
            camera = info->getCamera( GEOMETRY_CAMERA );
            if (camera == 0) continue;
            camera->setCullMask( nm & ~simgear::MODELLIGHT_BIT );

            camera = info->getCamera( LIGHTING_CAMERA );
            if (camera == 0) continue;
            osg::Switch* sw = camera->getChild(0)->asSwitch();
            for (unsigned int i = 0; i < sw->getNumChildren(); ++i) {
                osg::Camera* lc = dynamic_cast<osg::Camera*>(sw->getChild(i));
                if (lc == 0) continue;
                string name = lc->getName();
                if (name == "LightCamera") {
                    lc->setCullMask( (nm & simgear::LIGHTS_BITS) | (lc->getCullMask() & ~simgear::LIGHTS_BITS) );
                }
            }
        }
    }
}

void CameraGroup::resized()
{
    for (CameraIterator i = camerasBegin(), e = camerasEnd(); i != e; ++i) {
        CameraInfo *info = i->get();
        Camera* camera = info->getCamera( MAIN_CAMERA );
        if ( camera == 0 )
            camera = info->getCamera( DISPLAY_CAMERA );
        const Viewport* viewport = camera->getViewport();
        info->x = viewport->x();
        info->y = viewport->y();
        info->width = viewport->width();
        info->height = viewport->height();

        info->resized( info->width, info->height );
    }
}

const CameraInfo* CameraGroup::getGUICamera() const
{
    ConstCameraIterator result
        = std::find_if(camerasBegin(), camerasEnd(),
                   FlagTester<CameraInfo>(GUI));
    if (result == camerasEnd()) {
        return NULL;
    }

    return *result;
}
  
Camera* getGUICamera(CameraGroup* cgroup)
{
    const CameraInfo* info = cgroup->getGUICamera();
    if (!info) {
        return NULL;
    }
    
    return info->getCamera(MAIN_CAMERA);
}

static bool computeCameraIntersection(const CameraInfo* cinfo,
                                      const osgGA::GUIEventAdapter* ea,
                                      osgUtil::LineSegmentIntersector::Intersections& intersections)
{
  using osgUtil::Intersector;
  using osgUtil::LineSegmentIntersector;
  double x, y;
  eventToWindowCoords(ea, x, y);
  
  if (!(cinfo->flags & CameraGroup::DO_INTERSECTION_TEST))
    return false;
  
  const Camera* camera = cinfo->getCamera(MAIN_CAMERA);
  if ( !camera )
    camera = cinfo->getCamera( GEOMETRY_CAMERA );
  if (camera->getGraphicsContext() != ea->getGraphicsContext())
    return false;
  
  const Viewport* viewport = camera->getViewport();
  double epsilon = 0.5;
  if (!(x >= viewport->x() - epsilon
        && x < viewport->x() + viewport->width() -1.0 + epsilon
        && y >= viewport->y() - epsilon
        && y < viewport->y() + viewport->height() -1.0 + epsilon))
    return false;
  
  Vec4d start(x, y, 0.0, 1.0);
  Vec4d end(x, y, 1.0, 1.0);
  Matrix windowMat = viewport->computeWindowMatrix();
  Matrix startPtMat = Matrix::inverse(camera->getProjectionMatrix()
                                      * windowMat);
  Matrix endPtMat;
  const Camera* farCamera = cinfo->getCamera( FAR_CAMERA );
  if (!farCamera || farCamera->getNodeMask() == 0)
    endPtMat = startPtMat;
  else
    endPtMat = Matrix::inverse(farCamera->getProjectionMatrix()
                               * windowMat);
  start = start * startPtMat;
  start /= start.w();
  end = end * endPtMat;
  end /= end.w();
  ref_ptr<LineSegmentIntersector> picker
  = new LineSegmentIntersector(Intersector::VIEW,
                               Vec3d(start.x(), start.y(), start.z()),
                               Vec3d(end.x(), end.y(), end.z()));
  osgUtil::IntersectionVisitor iv(picker.get());
  iv.setTraversalMask( ~simgear::MODELLIGHT_BIT );
  const_cast<Camera*>(camera)->accept(iv);
  if (picker->containsIntersections()) {
    intersections = picker->getIntersections();
    return true;
  }
  
  return false;
}
  
bool computeIntersections(const CameraGroup* cgroup,
                          const osgGA::GUIEventAdapter* ea,
                          osgUtil::LineSegmentIntersector::Intersections& intersections)
{
    // test the GUI first
    const CameraInfo* guiCamera = cgroup->getGUICamera();
    if (guiCamera && computeCameraIntersection(guiCamera, ea, intersections))
        return true;
    
    // Find camera that contains event
    for (CameraGroup::ConstCameraIterator iter = cgroup->camerasBegin(),
             e = cgroup->camerasEnd();
         iter != e;
         ++iter) {
        const CameraInfo* cinfo = iter->get();
        if (cinfo == guiCamera)
            continue;
        
        if (computeCameraIntersection(cinfo, ea, intersections))
            return true;
    }
  
    intersections.clear();
    return false;
}

void warpGUIPointer(CameraGroup* cgroup, int x, int y)
{
    using osgViewer::GraphicsWindow;
    Camera* guiCamera = getGUICamera(cgroup);
    if (!guiCamera)
        return;
    Viewport* vport = guiCamera->getViewport();
    GraphicsWindow* gw
        = dynamic_cast<GraphicsWindow*>(guiCamera->getGraphicsContext());
    if (!gw)
        return;
    globals->get_renderer()->getEventHandler()->setMouseWarped();
    // Translate the warp request into the viewport of the GUI camera,
    // send the request to the window, then transform the coordinates
    // for the Viewer's event queue.
    double wx = x + vport->x();
    double wyUp = vport->height() + vport->y() - y;
    double wy;
    const GraphicsContext::Traits* traits = gw->getTraits();
    if (gw->getEventQueue()->getCurrentEventState()->getMouseYOrientation()
        == osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS) {
        wy = traits->height - wyUp;
    } else {
        wy = wyUp;
    }
    gw->getEventQueue()->mouseWarped(wx, wy);
    gw->requestWarpPointer(wx, wy);
    osgGA::GUIEventAdapter* eventState
        = cgroup->getViewer()->getEventQueue()->getCurrentEventState();
    double viewerX
        = (eventState->getXmin()
           + ((wx / double(traits->width))
              * (eventState->getXmax() - eventState->getXmin())));
    double viewerY
        = (eventState->getYmin()
           + ((wyUp / double(traits->height))
              * (eventState->getYmax() - eventState->getYmin())));
    cgroup->getViewer()->getEventQueue()->mouseWarped(viewerX, viewerY);
}
}
