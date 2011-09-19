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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "CameraGroup.hxx"

#include "globals.hxx"
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

void installCullVisitor(Camera* camera)
{
    osgViewer::Renderer* renderer
        = static_cast<osgViewer::Renderer*>(camera->getRenderer());
    for (int i = 0; i < 2; ++i) {
        osgUtil::SceneView* sceneView = renderer->getSceneView(i);
#if SG_OSG_VERSION_LESS_THAN(3,0,0)
        sceneView->setCullVisitor(new simgear::EffectCullVisitor);
#else
        osg::ref_ptr<osgUtil::CullVisitor::Identifier> identifier;
        identifier = sceneView->getCullVisitor()->getIdentifier();
        sceneView->setCullVisitor(new simgear::EffectCullVisitor);
        sceneView->getCullVisitor()->setIdentifier(identifier.get());

        identifier = sceneView->getCullVisitorLeft()->getIdentifier();
        sceneView->setCullVisitorLeft(sceneView->getCullVisitor()->clone());
        sceneView->getCullVisitorLeft()->setIdentifier(identifier.get());

        identifier = sceneView->getCullVisitorRight()->getIdentifier();
        sceneView->setCullVisitorRight(sceneView->getCullVisitor()->clone());
        sceneView->getCullVisitorRight()->setIdentifier(identifier.get());
#endif
    }
}
}

namespace flightgear
{
void updateCameras(const CameraInfo* info)
{
    if (info->camera.valid())
        info->camera->getViewport()->setViewport(info->x, info->y,
                                                 info->width, info->height);
    if (info->farCamera.valid())
        info->farCamera->getViewport()->setViewport(info->x, info->y,
                                                    info->width, info->height);
}

CameraInfo* CameraGroup::addCamera(unsigned flags, Camera* camera,
                                   const Matrix& view,
                                   const Matrix& projection,
                                   bool useMasterSceneData)
{
    CameraInfo* info = new CameraInfo(flags);
    // The camera group will always update the camera
    camera->setReferenceFrame(Transform::ABSOLUTE_RF);

    Camera* farCamera = 0;
    if ((flags & (GUI | ORTHO)) == 0) {
        farCamera = new Camera;
        farCamera->setAllowEventFocus(camera->getAllowEventFocus());
        farCamera->setGraphicsContext(camera->getGraphicsContext());
        farCamera->setCullingMode(camera->getCullingMode());
        farCamera->setInheritanceMask(camera->getInheritanceMask());
        farCamera->setReferenceFrame(Transform::ABSOLUTE_RF);
        // Each camera's viewport is written when the window is
        // resized; if the the viewport isn't copied here, it gets updated
        // twice and ends up with the wrong value.
        farCamera->setViewport(simgear::clone(camera->getViewport()));
        farCamera->setDrawBuffer(camera->getDrawBuffer());
        farCamera->setReadBuffer(camera->getReadBuffer());
        farCamera->setRenderTargetImplementation(
            camera->getRenderTargetImplementation());
        const Camera::BufferAttachmentMap& bufferMap
            = camera->getBufferAttachmentMap();
        if (bufferMap.count(Camera::COLOR_BUFFER) != 0) {
            farCamera->attach(
                Camera::COLOR_BUFFER,
                bufferMap.find(Camera::COLOR_BUFFER)->second._texture.get());
        }
        _viewer->addSlave(farCamera, projection, view, useMasterSceneData);
        installCullVisitor(farCamera);
        info->farCamera = farCamera;
        info->farSlaveIndex = _viewer->getNumSlaves() - 1;
        farCamera->setRenderOrder(Camera::POST_RENDER, info->farSlaveIndex);
        camera->setCullMask(camera->getCullMask() & ~simgear::BACKGROUND_BIT);
        camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    }
    _viewer->addSlave(camera, projection, view, useMasterSceneData);
    installCullVisitor(camera);
    info->camera = camera;
    info->slaveIndex = _viewer->getNumSlaves() - 1;
    camera->setRenderOrder(Camera::POST_RENDER, info->slaveIndex);
    _cameras.push_back(info);
    return info;
}

void CameraGroup::update(const osg::Vec3d& position,
                         const osg::Quat& orientation)
{
    const Matrix masterView(osg::Matrix::translate(-position)
                            * osg::Matrix::rotate(orientation.inverse()));
    _viewer->getCamera()->setViewMatrix(masterView);
    const Matrix& masterProj = _viewer->getCamera()->getProjectionMatrix();
    for (CameraList::iterator i = _cameras.begin(); i != _cameras.end(); ++i) {
        const CameraInfo* info = i->get();
        const View::Slave& slave = _viewer->getSlave(info->slaveIndex);
#if SG_OSG_VERSION_LESS_THAN(3,0,0)
        // refreshes camera viewports (for now)
        updateCameras(info);
#endif
        Camera* camera = info->camera.get();
        Matrix viewMatrix;
        if ((info->flags & VIEW_ABSOLUTE) != 0)
            viewMatrix = slave._viewOffset;
        else
            viewMatrix = masterView * slave._viewOffset;
        camera->setViewMatrix(viewMatrix);
        Matrix projectionMatrix;
        if ((info->flags & PROJECTION_ABSOLUTE) != 0)
            projectionMatrix = slave._projectionOffset;
        else
            projectionMatrix = masterProj * slave._projectionOffset;

        if (!info->farCamera.valid()) {
            camera->setProjectionMatrix(projectionMatrix);
        } else {
            Camera* farCamera = info->farCamera.get();
            farCamera->setViewMatrix(viewMatrix);
            double left, right, bottom, top, parentNear, parentFar;
            projectionMatrix.getFrustum(left, right, bottom, top,
                                        parentNear, parentFar);
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
    }
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
    
    const osg::Viewport* viewport = info->camera->getViewport();
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

static osg::Geometry* createParoramicSphericalDisplayDistortionMesh(
    const Vec3& origin, const Vec3& widthVector, const Vec3& heightVector,
    double sphere_radius, double collar_radius,
    Image* intensityMap = 0, const Matrix& projectorMatrix = Matrix())
{
    osg::Vec3d center(0.0,0.0,0.0);
    osg::Vec3d eye(0.0,0.0,0.0);

    double distance = sqrt(sphere_radius*sphere_radius - collar_radius*collar_radius);
    bool flip = false;
    bool texcoord_flip = false;

    osg::Vec3d projector = eye - osg::Vec3d(0.0,0.0, distance);

#if 0
    OSG_INFO<<"createParoramicSphericalDisplayDistortionMesh : Projector position = "<<projector<<std::endl;
    OSG_INFO<<"createParoramicSphericalDisplayDistortionMesh : distance = "<<distance<<std::endl;
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

    osg::Vec3 bottom = origin;
    osg::Vec3 dx = xAxis*(width/((float)(noSteps-2)));
    osg::Vec3 dy = yAxis*(height/((float)(noSteps-1)));

    osg::Vec3 top = origin + yAxis*height;

    osg::Vec3 screenCenter = origin + widthVector*0.5f + heightVector*0.5f;
    float screenRadius = heightVector.length() * 0.5f;

    geometry->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);

    for(int i=0;i<noSteps;++i)
    {
        osg::Vec3 cursor = bottom+dy*(float)i;
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
    geode->addDrawable(createParoramicSphericalDisplayDistortionMesh(
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
#if defined(HAVE_CULLSETTINGS_CLEAR_MASK)
                                   | CullSettings::CLEAR_MASK
#endif
                                   ));

    osg::Matrix pOff;
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
    const SGPropertyNode* projectionNode = 0;
    if ((projectionNode = cameraNode->getNode("perspective")) != 0) {
        double fovy = projectionNode->getDoubleValue("fovy-deg", 55.0);
        double aspectRatio = projectionNode->getDoubleValue("aspect-ratio",
                                                            1.0);
        double zNear = projectionNode->getDoubleValue("near", 0.0);
        double zFar = projectionNode->getDoubleValue("far", 0.0);
        double offsetX = projectionNode->getDoubleValue("offset-x", 0.0);
        double offsetY = projectionNode->getDoubleValue("offset-y", 0.0);
        double tan_fovy = tan(DegreesToRadians(fovy*0.5));
        double right = tan_fovy * aspectRatio * zNear + offsetX;
        double left = -tan_fovy * aspectRatio * zNear + offsetX;
        double top = tan_fovy * zNear + offsetY;
        double bottom = -tan_fovy * zNear + offsetY;
        pOff.makeFrustum(left, right, bottom, top, zNear, zFar);
        cameraFlags |= PROJECTION_ABSOLUTE;
    } else if ((projectionNode = cameraNode->getNode("frustum")) != 0
               || (projectionNode = cameraNode->getNode("ortho")) != 0) {
        double top = projectionNode->getDoubleValue("top", 0.0);
        double bottom = projectionNode->getDoubleValue("bottom", 0.0);
        double left = projectionNode->getDoubleValue("left", 0.0);
        double right = projectionNode->getDoubleValue("right", 0.0);
        double zNear = projectionNode->getDoubleValue("near", 0.0);
        double zFar = projectionNode->getDoubleValue("far", 0.0);
        if (cameraNode->getNode("frustum")) {
            pOff.makeFrustum(left, right, bottom, top, zNear, zFar);
            cameraFlags |= PROJECTION_ABSOLUTE;
        } else {
            pOff.makeOrtho(left, right, bottom, top, zNear, zFar);
            cameraFlags |= (PROJECTION_ABSOLUTE | ORTHO);
        }
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
    CameraInfo* info = addCamera(cameraFlags, camera, vOff, pOff,
                                 useMasterSceneGraph);
    // If a viewport isn't set on the camera, then it's hard to dig it
    // out of the SceneView objects in the viewer, and the coordinates
    // of mouse events are somewhat bizzare.
    SGPropertyNode* viewportNode = cameraNode->getNode("viewport", true);
    buildViewport(info, viewportNode, window->gc->getTraits());
    updateCameras(info);
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
      SG_LOG(SG_GENERAL, SG_WARN, "CameraGroup::buildGUICamera: failed to build a window");
      return NULL;
    }

    Camera* camera = new Camera;
    camera->setAllowEventFocus(false);
    camera->setGraphicsContext(window->gc.get());
    camera->setViewport(new Viewport);
    // XXX Camera needs to be drawn last; eventually the render order
    // should be assigned by a camera manager.
    camera->setRenderOrder(osg::Camera::POST_RENDER, 100);
        camera->setClearMask(0);
    camera->setInheritanceMask(CullSettings::ALL_VARIABLES
                               & ~(CullSettings::COMPUTE_NEAR_FAR_MODE
                                   | CullSettings::CULLING_MODE
#if defined(HAVE_CULLSETTINGS_CLEAR_MASK)
                                   | CullSettings::CLEAR_MASK
#endif
                                   ));
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->setCullingMode(osg::CullSettings::NO_CULLING);
    camera->setProjectionResizePolicy(Camera::FIXED);
    camera->setReferenceFrame(Transform::ABSOLUTE_RF);
    const int cameraFlags = GUI;
    CameraInfo* result = addCamera(cameraFlags, camera, Matrixd::identity(),
                                   Matrixd::identity(), false);
    SGPropertyNode* viewportNode = cameraNode->getNode("viewport", true);
    buildViewport(result, viewportNode, window->gc->getTraits());

    // Disable statistics for the GUI camera.
    result->camera->setStats(0);
    updateCameras(result);
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
        if (info->farCamera.valid() && info->farCamera->getNodeMask() != 0) {
            info->camera->setCullMask(nm & ~simgear::BACKGROUND_BIT);
            info->camera->setCullMaskLeft(nm & ~simgear::BACKGROUND_BIT);
            info->camera->setCullMaskRight(nm & ~simgear::BACKGROUND_BIT);
            info->farCamera->setCullMask(nm);
            info->farCamera->setCullMaskLeft(nm);
            info->farCamera->setCullMaskRight(nm);
        } else {
            info->camera->setCullMask(nm);
            info->camera->setCullMaskLeft(nm);
            info->camera->setCullMaskRight(nm);
        }
    }
}

void CameraGroup::resized()
{
    for (CameraIterator i = camerasBegin(), e = camerasEnd(); i != e; ++i) {
        CameraInfo *info = i->get();
        const Viewport* viewport = info->camera->getViewport();
        info->x = viewport->x();
        info->y = viewport->y();
        info->width = viewport->width();
        info->height = viewport->height();
    }
}

Camera* getGUICamera(CameraGroup* cgroup)
{
    CameraGroup::CameraIterator end = cgroup->camerasEnd();
    CameraGroup::CameraIterator result
        = std::find_if(cgroup->camerasBegin(), end,
                       FlagTester<CameraInfo>(CameraGroup::GUI));
    if (result != end)
        return (*result)->camera.get();
    else
        return 0;
}

bool computeIntersections(const CameraGroup* cgroup,
                          const osgGA::GUIEventAdapter* ea,
                          osgUtil::LineSegmentIntersector::Intersections& intersections)
{
    using osgUtil::Intersector;
    using osgUtil::LineSegmentIntersector;
    double x, y;
    eventToWindowCoords(ea, x, y);
    // Find camera that contains event
    for (CameraGroup::ConstCameraIterator iter = cgroup->camerasBegin(),
             e = cgroup->camerasEnd();
         iter != e;
         ++iter) {
        const CameraInfo* cinfo = iter->get();
        if ((cinfo->flags & CameraGroup::DO_INTERSECTION_TEST) == 0)
            continue;
        const Camera* camera = cinfo->camera.get();
        if (camera->getGraphicsContext() != ea->getGraphicsContext())
            continue;
        const Viewport* viewport = camera->getViewport();
        double epsilon = 0.5;
        if (!(x >= viewport->x() - epsilon
              && x < viewport->x() + viewport->width() -1.0 + epsilon
              && y >= viewport->y() - epsilon
              && y < viewport->y() + viewport->height() -1.0 + epsilon))
            continue;
        Vec4d start(x, y, 0.0, 1.0);
        Vec4d end(x, y, 1.0, 1.0);
        Matrix windowMat = viewport->computeWindowMatrix();
        Matrix startPtMat = Matrix::inverse(camera->getProjectionMatrix()
                                            * windowMat);
        Matrix endPtMat;
        if (!cinfo->farCamera.valid() || cinfo->farCamera->getNodeMask() == 0)
            endPtMat = startPtMat;
        else
            endPtMat = Matrix::inverse(cinfo->farCamera->getProjectionMatrix()
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
        const_cast<Camera*>(camera)->accept(iv);
        if (picker->containsIntersections()) {
            intersections = picker->getIntersections();
            return true;
        } else {
            break;
        }
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
