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

#include "CameraGroup.hxx"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include "renderer.hxx"
#include "FGEventHandler.hxx"
#include "WindowBuilder.hxx"
#include "WindowSystemAdapter.hxx"

#include <simgear/math/SGRect.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx> // for copyProperties
#include <simgear/structure/exception.hxx>
#include <simgear/structure/OSGUtils.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/viewer/Compositor.hxx>

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

#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Renderer>

using namespace osg;

namespace {

osg::Matrix
invert(const osg::Matrix& matrix)
{
    return osg::Matrix::inverse(matrix);
}

/// Returns the zoom factor of the master camera.
/// The reference fov is the historic 55 deg
double
zoomFactor()
{
    double fov = fgGetDouble("/sim/current-view/field-of-view", 55);
    if (fov < 1)
        fov = 1;
    return tan(55*0.5*SG_DEGREES_TO_RADIANS)/tan(fov*0.5*SG_DEGREES_TO_RADIANS);
}

osg::Vec2d
preMult(const osg::Vec2d& v, const osg::Matrix& m)
{
    osg::Vec3d tmp = m.preMult(osg::Vec3(v, 0));
    return osg::Vec2d(tmp[0], tmp[1]);
}

osg::Matrix
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

} // anonymous namespace

namespace flightgear
{
using namespace simgear;
using namespace compositor;

class CameraGroupListener : public SGPropertyChangeListener {
public:
    CameraGroupListener(CameraGroup* cg, SGPropertyNode* gnode) :
        _groupNode(gnode),
        _cameraGroup(cg) {
        listenToNode("znear", 0.1f);
        listenToNode("zfar", 120000.0f);
        listenToNode("near-field", 100.0f);
        listenToNode("lod", 1.5f);
    }

    virtual ~CameraGroupListener() {
        unlisten("znear");
        unlisten("zfar");
        unlisten("near-field");
        unlisten("lod");
    }

    virtual void valueChanged(SGPropertyNode* prop) {
        if (!strcmp(prop->getName(), "znear")) {
            _cameraGroup->_zNear = prop->getFloatValue();
        } else if (!strcmp(prop->getName(), "zfar")) {
            _cameraGroup->_zFar = prop->getFloatValue();
        } else if (!strcmp(prop->getName(), "near-field")) {
            _cameraGroup->_nearField = prop->getFloatValue();
        } else if (!strcmp(prop->getName(), "lod")) {
            const float new_lod = prop->getFloatValue();
            _cameraGroup->_viewer->getCamera()->setLODScale(new_lod);
        }
    }
private:
    void listenToNode(const std::string& name, double val) {
        SGPropertyNode* n = _groupNode->getChild(name);
        if (!n) {
            n = _groupNode->getChild(name, 0 /* index */, true);
            n->setDoubleValue(val);
        }
        n->addChangeListener(this);
        valueChanged(n); // propogate initial state through
    }

    void unlisten(const std::string& name) {
        _groupNode->getChild(name)->removeChangeListener(this);
    }

    SGPropertyNode_ptr _groupNode;
    CameraGroup* _cameraGroup; // non-owning reference
};

struct GUIUpdateCallback : public Pass::PassUpdateCallback {
    virtual void updatePass(Pass &pass,
                            const osg::Matrix &view_matrix,
                            const osg::Matrix &proj_matrix) {
        // Just set both the view matrix and the projection matrix
        pass.camera->setViewMatrix(view_matrix);
        pass.camera->setProjectionMatrix(proj_matrix);
    }
};

typedef std::vector<SGPropertyNode_ptr> SGPropertyNodeVec;

osg::ref_ptr<CameraGroup> CameraGroup::_defaultGroup;

CameraGroup::CameraGroup(osgViewer::Viewer* viewer) :
    _viewer(viewer)
{
}

CameraGroup::~CameraGroup()
{

}

void CameraGroup::update(const osg::Vec3d& position,
                         const osg::Quat& orientation)
{
    const osg::Matrix masterView(osg::Matrix::translate(-position)
                                 * osg::Matrix::rotate(orientation.inverse()));
    _viewer->getCamera()->setViewMatrix(masterView);
    const osg::Matrix& masterProj = _viewer->getCamera()->getProjectionMatrix();
    double masterZoomFactor = zoomFactor();

    for (const auto &info : _cameras) {
        osg::Matrix view_matrix;
        if (info->flags & CameraInfo::GUI)
            view_matrix = osg::Matrix::identity();
        else if ((info->flags & CameraInfo::VIEW_ABSOLUTE) != 0)
            view_matrix = info->viewOffset;
        else
            view_matrix = masterView * info->viewOffset;

        osg::Matrix proj_matrix;
        if (info->flags & CameraInfo::GUI) {
            const osg::GraphicsContext::Traits *traits =
                info->compositor->getGraphicsContext()->getTraits();
            proj_matrix = osg::Matrix::ortho2D(0, traits->width, 0, traits->height);
        } else if ((info->flags & CameraInfo::PROJECTION_ABSOLUTE) != 0) {
            if (info->flags & CameraInfo::ENABLE_MASTER_ZOOM) {
                if (info->relativeCameraParent) {
                    // template projection and view matrices of the current camera
                    osg::Matrix P0 = info->projOffset;
                    osg::Matrix R = view_matrix;

                    // The already known projection and view matrix of the parent camera
                    osg::Matrix pP = info->relativeCameraParent->viewMatrix;
                    osg::Matrix pR = info->relativeCameraParent->projMatrix;

                    // And the projection matrix derived from P0 so that the
                    // reference points match
                    proj_matrix = relativeProjection(P0, R,  info->thisReference,
                                                     pP, pR, info->parentReference);
                } else {
                    // We want to zoom, so take the original matrix and apply the
                    // zoom to it
                    proj_matrix = info->projOffset;
                    proj_matrix.postMultScale(osg::Vec3d(masterZoomFactor,
                                                         masterZoomFactor,
                                                         1));
                }
            } else {
                proj_matrix = info->projOffset;
            }
        } else {
            proj_matrix = masterProj * info->projOffset;
        }

        info->viewMatrix = view_matrix;
        info->projMatrix = proj_matrix;
        info->compositor->update(view_matrix, proj_matrix);
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

    // The master camera is the first one added
    const CameraInfo *info = _cameras.front();
    if (!info)
        return 0.0;
    const osg::GraphicsContext::Traits *traits =
        info->compositor->getGraphicsContext()->getTraits();

    return static_cast<double>(traits->height) / traits->width;
}

// FIXME: Port this to the Compositor
#if 0
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

#endif

void CameraGroup::buildDistortionCamera(const SGPropertyNode* psNode,
                                        Camera* camera)
{
    // FIXME: Port this to the Compositor
#if 0
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
#endif
}

void CameraGroup::buildCamera(SGPropertyNode* cameraNode)
{
    WindowBuilder *wBuild = WindowBuilder::getWindowBuilder();
    const SGPropertyNode* windowNode = cameraNode->getNode("window");
    GraphicsWindow* window = 0;
    int cameraFlags = CameraInfo::DO_INTERSECTION_TEST;
    if (windowNode) {
        // New style window declaration / definition
        window = wBuild->buildWindow(windowNode);
    } else {
        // Old style: suck window params out of camera block
        window = wBuild->buildWindow(cameraNode);
    }
    if (!window) {
        return;
    }

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
            cameraFlags |= CameraInfo::VIEW_ABSOLUTE;
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
        cameraFlags |= CameraInfo::PROJECTION_ABSOLUTE;
        if (projectionNode->getBoolValue("fixed-near-far", true))
            cameraFlags |= CameraInfo::FIXED_NEAR_FAR;
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
            cameraFlags |= CameraInfo::PROJECTION_ABSOLUTE;
        } else {
            pOff.makeOrtho(left, right, bottom, top, zNear, zFar);
            cameraFlags |= (CameraInfo::PROJECTION_ABSOLUTE | CameraInfo::ORTHO);
        }
        if (projectionNode->getBoolValue("fixed-near-far", true))
            cameraFlags |= CameraInfo::FIXED_NEAR_FAR;
    } else if ((projectionNode = cameraNode->getNode("master-perspective")) != 0) {
        double zNear = projectionNode->getDoubleValue("eye-distance", 0.4*physicalWidth);
        double xoff = projectionNode->getDoubleValue("x-offset", 0);
        double yoff = projectionNode->getDoubleValue("y-offset", 0);
        double left = -0.5*physicalWidth - xoff;
        double right = 0.5*physicalWidth - xoff;
        double bottom = -0.5*physicalHeight - yoff;
        double top = 0.5*physicalHeight - yoff;
        pOff.makeFrustum(left, right, bottom, top, zNear, zNear*1000);
        cameraFlags |= CameraInfo::PROJECTION_ABSOLUTE | CameraInfo::ENABLE_MASTER_ZOOM;
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
            return;
        }
        const CameraInfo* parentInfo = _cameras[parentCameraIndex];
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
        cameraFlags |= CameraInfo::PROJECTION_ABSOLUTE | CameraInfo::ENABLE_MASTER_ZOOM;
    } else {
        // old style shear parameters
        double shearx = cameraNode->getDoubleValue("shear-x", 0);
        double sheary = cameraNode->getDoubleValue("shear-y", 0);
        pOff.makeTranslate(-shearx, -sheary, 0);
    }
    const SGPropertyNode* psNode = cameraNode->getNode("panoramic-spherical");
    //bool useMasterSceneGraph = !psNode;

    CameraInfo *info = new CameraInfo(cameraFlags);
    _cameras.push_back(info);
    info->name = cameraNode->getStringValue("name");
    info->physicalWidth = physicalWidth;
    info->physicalHeight = physicalHeight;
    info->bezelHeightTop = bezelHeightTop;
    info->bezelHeightBottom = bezelHeightBottom;
    info->bezelWidthLeft = bezelWidthLeft;
    info->bezelWidthRight = bezelWidthRight;
    if (parentCameraIndex < _cameras.size())
        info->relativeCameraParent = _cameras[parentCameraIndex];
    info->parentReference[0] = parentReference[0];
    info->parentReference[1] = parentReference[1];
    info->thisReference[0] = thisReference[0];
    info->thisReference[1] = thisReference[1];
    info->viewOffset = vOff;
    info->projOffset = pOff;

    osg::Viewport *viewport = new osg::Viewport(
        viewportNode->getDoubleValue("x"),
        viewportNode->getDoubleValue("y"),
        // If no width or height has been specified, fill the entire window
        viewportNode->getDoubleValue("width", window->gc->getTraits()->width),
        viewportNode->getDoubleValue("height",window->gc->getTraits()->height));
    std::string default_compositor =
        fgGetString("/sim/rendering/default-compositor", "Compositor/default");
    std::string compositor_path =
        cameraNode->getStringValue("compositor", default_compositor.c_str());
    osg::ref_ptr<SGReaderWriterOptions> options =
        SGReaderWriterOptions::fromPath(globals->get_fg_root());
    options->setPropertyNode(globals->get_props());
    Compositor *compositor = Compositor::create(_viewer,
                                                window->gc,
                                                viewport,
                                                compositor_path,
                                                options);
    if (compositor) {
        info->compositor = compositor;
    } else {
        throw sg_exception(std::string("Failed to create Compositor in path '") +
                           compositor_path + "'");
    }

    // Distortion camera needs the viewport which is created by addCamera().
    if (psNode) {
        info->flags = info->flags | CameraInfo::VIEW_ABSOLUTE;
        //buildDistortionCamera(psNode, camera);
    }
}

void CameraGroup::buildGUICamera(SGPropertyNode* cameraNode,
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
        return;
    }

    Camera* camera = new Camera;
    camera->setName( "GUICamera" );
    camera->setAllowEventFocus(false);
    camera->setGraphicsContext(window->gc.get());
    // If a viewport isn't set on the camera, then it's hard to dig it
    // out of the SceneView objects in the viewer, and the coordinates
    // of mouse events are somewhat bizzare.
    osg::Viewport *viewport = new osg::Viewport(
        0, 0, window->gc->getTraits()->width, window->gc->getTraits()->height);
    camera->setViewport(viewport);
    camera->setClearMask(0);
    camera->setInheritanceMask(CullSettings::ALL_VARIABLES
                               & ~(CullSettings::COMPUTE_NEAR_FAR_MODE
                                   | CullSettings::CULLING_MODE
                                   | CullSettings::CLEAR_MASK
                                   ));
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->setCullingMode(osg::CullSettings::NO_CULLING);
    camera->setProjectionResizePolicy(osg::Camera::FIXED);

    // The camera group will always update the camera
    camera->setReferenceFrame(Transform::ABSOLUTE_RF);

    // Draw all nodes in the order they are added to the GUI camera
    camera->getOrCreateStateSet()
        ->setRenderBinDetails( 0,
                               "PreOrderBin",
                               osg::StateSet::OVERRIDE_RENDERBIN_DETAILS );

    // XXX Camera needs to be drawn last; eventually the render order
    // should be assigned by a camera manager.
    camera->setRenderOrder(osg::Camera::POST_RENDER, 10000);

    Pass *pass = new Pass;
    pass->camera = camera;
    pass->useMastersSceneData = false;
    pass->update_callback = new GUIUpdateCallback;

    // For now we just build a simple Compositor directly from C++ space that
    // encapsulates a single osg::Camera. This could be improved by letting
    // users change the Compositor config in XML space, for example to be able
    // to add post-processing to a HUD.
    // However, since many other parts of FG require direct access to the GUI
    // osg::Camera object, this is fine for now.
    Compositor *compositor = new Compositor(_viewer, window->gc, viewport);
    compositor->addPass(pass);

    const int cameraFlags = CameraInfo::GUI | CameraInfo::DO_INTERSECTION_TEST;
    CameraInfo* info = new CameraInfo(cameraFlags);
    info->name = "GUI camera";
    info->viewOffset = osg::Matrix::identity();
    info->projOffset = osg::Matrix::identity();
    info->compositor = compositor;
    _cameras.push_back(info);

    // Disable statistics for the GUI camera.
    camera->setStats(0);
}

CameraGroup* CameraGroup::buildCameraGroup(osgViewer::Viewer* viewer,
                                           SGPropertyNode* gnode)
{
    CameraGroup* cgroup = new CameraGroup(viewer);
    cgroup->_listener.reset(new CameraGroupListener(cgroup, gnode));

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

    return cgroup;
}

void CameraGroup::resized()
{
    for (const auto &info : _cameras)
        info->compositor->resized();
}

CameraInfo* CameraGroup::getGUICamera() const
{
    auto result = std::find_if(_cameras.begin(), _cameras.end(),
                               [](const osg::ref_ptr<CameraInfo> &i) {
                                   return (i->flags & CameraInfo::GUI) != 0;
                               });
    if (result == _cameras.end())
        return 0;
    return (*result);
}

osg::Camera* getGUICamera(CameraGroup* cgroup)
{
    return cgroup->getGUICamera()->compositor->getPass(0)->camera;
}

static bool
computeCameraIntersection(const CameraGroup *cgroup,
                          const CameraInfo *cinfo,
                          const osg::Vec2d &windowPos,
                          osgUtil::LineSegmentIntersector::Intersections &intersections)
{
    if (!(cinfo->flags & CameraInfo::DO_INTERSECTION_TEST))
        return false;

    const osg::Viewport *viewport = cinfo->compositor->getViewport();
    SGRect<double> viewportRect(viewport->x(), viewport->y(),
                                viewport->x() + viewport->width() - 1.0,
                                viewport->y() + viewport->height()- 1.0);
    double epsilon = 0.5;
    if (!viewportRect.contains(windowPos.x(), windowPos.y(), epsilon))
        return false;

    osg::Vec4d start(windowPos.x(), windowPos.y(), 0.0, 1.0);
    osg::Vec4d end(windowPos.x(), windowPos.y(), 1.0, 1.0);
    osg::Matrix windowMat = viewport->computeWindowMatrix();
    osg::Matrix invViewMat = osg::Matrix::inverse(cinfo->viewMatrix);
    osg::Matrix invProjMat = osg::Matrix::inverse(cinfo->projMatrix * windowMat);
    start = start * invProjMat;
    end = end * invProjMat;
    start /= start.w();
    end /= end.w();
    start = start * invViewMat;
    end = end * invViewMat;

    osg::ref_ptr<osgUtil::LineSegmentIntersector> picker =
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL,
                                            osg::Vec3d(start.x(), start.y(), start.z()),
                                            osg::Vec3d(end.x(), end.y(), end.z()));
    osgUtil::IntersectionVisitor iv(picker);
    iv.setTraversalMask(simgear::PICK_BIT);

    const_cast<CameraGroup *>(cgroup)->getViewer()->getCamera()->accept(iv);
    if (picker->containsIntersections()) {
        intersections = picker->getIntersections();
        return true;
    }

    return false;
}

bool computeIntersections(const CameraGroup* cgroup,
                          const osg::Vec2d& windowPos,
                          osgUtil::LineSegmentIntersector::Intersections& intersections)
{
    // test the GUI first
    CameraInfo* guiCamera = cgroup->getGUICamera();
    if (guiCamera && computeCameraIntersection(cgroup, guiCamera, windowPos, intersections))
        return true;

    // Find camera that contains event
    for (const auto &cinfo : cgroup->_cameras) {
        if (cinfo == guiCamera)
            continue;

        if (computeCameraIntersection(cgroup, cinfo, windowPos, intersections))
            return true;
    }

    intersections.clear();
    return false;
}

void warpGUIPointer(CameraGroup* cgroup, int x, int y)
{
    using osgViewer::GraphicsWindow;
    osg::Camera* guiCamera = getGUICamera(cgroup);
    if (!guiCamera)
        return;
    osg::Viewport* vport = guiCamera->getViewport();
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
    const osg::GraphicsContext::Traits* traits = gw->getTraits();
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

void CameraGroup::buildDefaultGroup(osgViewer::Viewer* viewer)
{
    // Look for windows, camera groups, and the old syntax of
    // top-level cameras
    SGPropertyNode* renderingNode = fgGetNode("/sim/rendering");
    SGPropertyNode* cgroupNode = renderingNode->getNode("camera-group", true);
    bool oldSyntax = !cgroupNode->hasChild("camera");
    if (oldSyntax) {
        for (int i = 0; i < renderingNode->nChildren(); ++i) {
            SGPropertyNode* propNode = renderingNode->getChild(i);
            const char* propName = propNode->getName();
            if (!strcmp(propName, "window") || !strcmp(propName, "camera")) {
                SGPropertyNode* copiedNode
                    = cgroupNode->getNode(propName, propNode->getIndex(), true);
                copyProperties(propNode, copiedNode);
            }
        }

        SGPropertyNodeVec cameras(cgroupNode->getChildren("camera"));
        SGPropertyNode* masterCamera = 0;
        SGPropertyNodeVec::const_iterator it;
        for (it = cameras.begin(); it != cameras.end(); ++it) {
            if ((*it)->getDoubleValue("shear-x", 0.0) == 0.0
                && (*it)->getDoubleValue("shear-y", 0.0) == 0.0) {
                masterCamera = it->ptr();
                break;
            }
        }
        if (!masterCamera) {
            WindowBuilder *windowBuilder = WindowBuilder::getWindowBuilder();
            masterCamera = cgroupNode->getChild("camera", cameras.size(), true);
            setValue(masterCamera->getNode("window/name", true),
                     windowBuilder->getDefaultWindowName());
        }
        SGPropertyNode* nameNode = masterCamera->getNode("window/name");
        if (nameNode)
            setValue(cgroupNode->getNode("gui/window/name", true),
                     nameNode->getStringValue());
    }

    CameraGroup* cgroup = buildCameraGroup(viewer, cgroupNode);
    setDefault(cgroup);
}

} // of namespace flightgear
