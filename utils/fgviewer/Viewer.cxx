// Viewer.cxx -- alternative flightgear viewer application
//
// Copyright (C) 2009 - 2012  Mathias Froehlich
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
#include <config.h>
#endif

#include "Viewer.hxx"

#include <osg/Version>
#include <osg/ProxyNode>
#include <osg/PagedLOD>
#include <osgDB/ReadFile>

#ifdef __linux__
#include <X11/Xlib.h>
#include <osgViewer/api/X11/GraphicsWindowX11>
#endif

#include "MEncoderCaptureOperation.hxx"
#include "ArgumentParser.hxx"

#include <cassert>

namespace fgviewer  {

Viewer::Viewer(ArgumentParser& arguments) :
    osgViewer::Viewer(arguments.osg()),
    _sceneDataGroup(new osg::Group),
    _timeIncrement(SGTimeStamp::fromSec(0)),
    _simTime(SGTimeStamp::fromSec(0))
{
    /// Careful: this method really assigns the sceneDataGroup to all cameras!
    /// FIXME the 'useMasterScene' flag at the slave is able to get around that!!!
    osgViewer::Viewer::setSceneData(_sceneDataGroup.get());
    /// The only changed default that is renderer independent ...
    getCamera()->setClearColor(osg::Vec4(0, 0, 0, 0));
}

Viewer::~Viewer()
{
    stopThreading();

#if FG_HAVE_HLA
    if (_viewerFederate.valid())
        _viewerFederate->shutdown();
    _viewerFederate = 0;
#endif
}

bool
Viewer::readCameraConfig(const SGPropertyNode& viewerNode)
{
    // Collect and realize all windows
    for (int i = 0; i < viewerNode.nChildren(); ++i) {
        // FIXME support window, fullscreen, offscreen
        const SGPropertyNode* windowNode = viewerNode.getChild(i);
        if (!windowNode || windowNode->getNameString() != "window")
            continue;

        std::string name = windowNode->getStringValue("name", "");
        if (name.empty()) {
            SG_LOG(SG_VIEW, SG_ALERT, "Ignoring unnamed window!");
            return false;
        }

        Drawable* drawable = getOrCreateDrawable(name);

        osg::GraphicsContext::ScreenIdentifier screenIdentifier;
        screenIdentifier = getScreenIdentifier(windowNode->getStringValue("display", ""));
        drawable->setScreenIdentifier(screenIdentifier.displayName());

        if (windowNode->getBoolValue("fullscreen", false)) {
            osg::GraphicsContext::ScreenSettings screenSettings;
            screenSettings = getScreenSettings(screenIdentifier);
            drawable->setPosition(SGVec2i(0, 0));
            drawable->setSize(SGVec2i(screenSettings.width, screenSettings.height));
            drawable->setFullscreen(true);
            drawable->setOffscreen(false);

        } else if (windowNode->getBoolValue("video", false)) {
            drawable->setPosition(SGVec2i(0, 0));
            SGVec2i size;
            size[0] = windowNode->getIntValue("geometry/width", 1366);
            size[1] = windowNode->getIntValue("geometry/height", 768);
            drawable->setSize(size);
            drawable->setFullscreen(true);
            drawable->setOffscreen(true);

            std::string outputFile = windowNode->getStringValue("output-file", "fgviewer.avi");
            unsigned fps = windowNode->getIntValue("frames-per-second", 30);

            /// This is the case for the video writers, have a fixed time increment
            _timeIncrement = SGTimeStamp::fromSec(1.0/fps);

            MEncoderCaptureOperation* captureOperation;
            captureOperation = new MEncoderCaptureOperation(outputFile, fps);
            osgViewer::ScreenCaptureHandler* captureHandler;
            captureHandler = new osgViewer::ScreenCaptureHandler(captureOperation, -1);
            addEventHandler(captureHandler);
            captureHandler->startCapture();

        } else {

            SGVec2i position;
            position[0] = windowNode->getIntValue("geometry/x", 0);
            position[1] = windowNode->getIntValue("geometry/y", 0);
            drawable->setPosition(position);
            SGVec2i size;
            size[0] = windowNode->getIntValue("geometry/width", 1366);
            size[1] = windowNode->getIntValue("geometry/height", 768);
            drawable->setSize(size);
            drawable->setFullscreen(false);
            drawable->setOffscreen(false);
        }
    }

    for (int i = 0; i < viewerNode.nChildren(); ++i) {
        const SGPropertyNode* cameraNode = viewerNode.getChild(i);
        if (!cameraNode || cameraNode->getNameString() != "camera")
            continue;

        std::string name = cameraNode->getStringValue("name", "");
        if (name.empty()) {
            SG_LOG(SG_VIEW, SG_ALERT, "Camera configuration needs a name!");
            return false;
        }

        SlaveCamera* slaveCamera = getOrCreateSlaveCamera(name);

        std::string drawableName = cameraNode->getStringValue("window", "");
        if (drawableName.empty()) {
            SG_LOG(SG_VIEW, SG_ALERT, "Camera configuration needs an assigned window!");
            return false;
        }
        Drawable* drawable = getDrawable(drawableName);
        if (!drawable) {
            SG_LOG(SG_VIEW, SG_ALERT, "Camera configuration \"" << name << "\" needs a drawable configured!");
            return false;
        }
        slaveCamera->setDrawableName(drawableName);
        drawable->attachSlaveCamera(slaveCamera);

        SGVec2i size = drawable->getSize();
        SGVec4i viewport(0, 0, size[0], size[1]);
        viewport[0] = cameraNode->getIntValue("viewport/x", viewport[0]);
        viewport[1] = cameraNode->getIntValue("viewport/y", viewport[1]);
        viewport[2] = cameraNode->getIntValue("viewport/width", viewport[2]);
        viewport[3] = cameraNode->getIntValue("viewport/height", viewport[3]);
        slaveCamera->setViewport(viewport);

        double headingDeg = cameraNode->getDoubleValue("view-offset/heading-deg", 0);
        double pitchDeg = cameraNode->getDoubleValue("view-offset/pitch-deg", 0);
        double rollDeg = cameraNode->getDoubleValue("view-offset/roll-deg", 0);
        slaveCamera->setViewOffsetDeg(headingDeg, pitchDeg, rollDeg);

        // Care for the reference points
        if (const SGPropertyNode* referencePointsNode = cameraNode->getNode("reference-points")) {
            for (int j = 0; j < referencePointsNode->nChildren(); ++j) {
                const SGPropertyNode* referencePointNode = cameraNode->getNode("reference-point");
                if (!referencePointNode)
                    continue;
                std::string name = referencePointNode->getStringValue("name", "");
                if (name.empty())
                    continue;
                osg::Vec2 point;
                point[0] = referencePointNode->getDoubleValue("x", 0);
                point[1] = referencePointNode->getDoubleValue("y", 0);
                slaveCamera->setProjectionReferencePoint(name, point);
            }
        }
        // Define 4 reference points by monitor dimensions
        else if (const SGPropertyNode* physicalDimensionsNode = cameraNode->getNode("physical-dimensions")) {
            double physicalWidth = physicalDimensionsNode->getDoubleValue("width", viewport[2]);
            double physicalHeight = physicalDimensionsNode->getDoubleValue("height", viewport[3]);
            if (const SGPropertyNode* bezelNode = physicalDimensionsNode->getNode("bezel")) {
                double bezelHeightTop = bezelNode->getDoubleValue("top", 0);
                double bezelHeightBottom = bezelNode->getDoubleValue("bottom", 0);
                double bezelWidthLeft = bezelNode->getDoubleValue("left", 0);
                double bezelWidthRight = bezelNode->getDoubleValue("right", 0);
                slaveCamera->setMonitorProjectionReferences(physicalWidth, physicalHeight,
                                                            bezelHeightTop, bezelHeightBottom,
                                                            bezelWidthLeft, bezelWidthRight);
            }
        }

        // The frustum node takes precedence, as this is the most explicit one.
        if (const SGPropertyNode* frustumNode = cameraNode->getNode("frustum")) {
            Frustum frustum(slaveCamera->getAspectRatio());
            frustum._near = frustumNode->getDoubleValue("near", frustum._near);
            frustum._left = frustumNode->getDoubleValue("left", frustum._left);
            frustum._right = frustumNode->getDoubleValue("right", frustum._right);
            frustum._bottom = frustumNode->getDoubleValue("bottom", frustum._bottom);
            frustum._top = frustumNode->getDoubleValue("top", frustum._top);
            slaveCamera->setFrustum(frustum);

        } else if (const SGPropertyNode* perspectiveNode = cameraNode->getNode("perspective")) {
            double fieldOfViewDeg = perspectiveNode->getDoubleValue("field-of-view-deg", 55);
            slaveCamera->setFustumByFieldOfViewDeg(fieldOfViewDeg);

        } else if (const SGPropertyNode* monitorNode = cameraNode->getNode("monitor")) {

            std::string referenceCameraName;
            std::string names[2];
            std::string referenceNames[2];

            // FIXME??!!
            if (const SGPropertyNode* leftOfNode = monitorNode->getNode("left-of")) {
                referenceCameraName = leftOfNode->getStringValue("");
                names[0] = "lowerRight";
                referenceNames[0] = "lowerLeft";
                names[1] = "upperRight";
                referenceNames[1] = "upperLeft";
            } else if (const SGPropertyNode* rightOfNode = monitorNode->getNode("right-of")) {
                referenceCameraName = rightOfNode->getStringValue("");
                names[0] = "lowerLeft";
                referenceNames[0] = "lowerRight";
                names[1] = "upperLeft";
                referenceNames[1] = "upperRight";
            } else if (const SGPropertyNode* aboveNode = monitorNode->getNode("above")) {
                referenceCameraName = aboveNode->getStringValue("");
                names[0] = "lowerLeft";
                referenceNames[0] = "upperLeft";
                names[1] = "lowerRight";
                referenceNames[1] = "upperRight";
            } else if (const SGPropertyNode* belowNode = monitorNode->getNode("below")) {
                referenceCameraName = belowNode->getStringValue("");
                names[0] = "upperLeft";
                referenceNames[0] = "lowerLeft";
                names[1] = "upperRight";
                referenceNames[1] = "lowerRight";
            } else {
                // names[0] = ;
                // referenceNames[0] = ;
                // names[1] = ;
                // referenceNames[1] = ;
            }

            // If we finally found a set of reference points that should match,
            // then create a relative frustum matching these references
            if (SlaveCamera* referenceSlaveCamera = getSlaveCamera(referenceCameraName)) {
                slaveCamera->setRelativeFrustum(names, *referenceSlaveCamera, referenceNames);
            } else {
                SG_LOG(SG_VIEW, SG_ALERT, "Unable to find reference camera \"" << referenceCameraName
                       << "\" for camera \"" << name << "\"!");
            }
        } else {
            // Set a proper default taking the current aspect ratio into account
            slaveCamera->setFustumByFieldOfViewDeg(55);
        }
    }

    return true;
}

void
Viewer::setupDefaultCameraConfigIfUnset()
{
    if (getNumDrawables() || getNumSlaveCameras())
        return;

    osg::GraphicsContext::ScreenIdentifier screenIdentifier;
    screenIdentifier = getDefaultScreenIdentifier();

    Drawable* drawable = getOrCreateDrawable("fgviewer");
    drawable->setScreenIdentifier(screenIdentifier.displayName());
    drawable->setPosition(SGVec2i(0, 0));
    SGVec2i size(800, 600);
    drawable->setSize(size);
    drawable->setFullscreen(false);
    drawable->setOffscreen(false);

    SlaveCamera* slaveCamera = getOrCreateSlaveCamera(drawable->getName());
    slaveCamera->setDrawableName(drawable->getName());
    drawable->attachSlaveCamera(slaveCamera);
    slaveCamera->setViewport(SGVec4i(0, 0, size[0], size[1]));
    slaveCamera->setViewOffset(osg::Matrix::identity());
    slaveCamera->setFustumByFieldOfViewDeg(55);
}

bool
Viewer::readConfiguration(const std::string&)
{
    return false;
}

void
Viewer::setRenderer(Renderer* renderer)
{
    if (!renderer) {
        SG_LOG(SG_VIEW, SG_ALERT, "Viewer::setRenderer(): Setting the renderer to zero is not supported!");
        return;
    }
    if (_renderer.valid()) {
        SG_LOG(SG_VIEW, SG_ALERT, "Viewer::setRenderer(): Setting the renderer twice is not supported!");
        return;
    }
    _renderer = renderer;
}

Renderer*
Viewer::getRenderer()
{
    return _renderer.get();
}

Drawable*
Viewer::getOrCreateDrawable(const std::string& name)
{
    Drawable* drawable = getDrawable(name);
    if (drawable)
        return drawable;
    if (!_renderer.valid())
        return 0;
    drawable = _renderer->createDrawable(*this, name);
    if (!drawable)
        return 0;
    _drawableVector.push_back(drawable);
    return drawable;
}

Drawable*
Viewer::getDrawable(const std::string& name)
{
    return getDrawable(getDrawableIndex(name));
}

unsigned
Viewer::getDrawableIndex(const std::string& name)
{
    for (DrawableVector::size_type i = 0; i < _drawableVector.size(); ++i) {
        if (_drawableVector[i]->getName() == name)
            return i;
    }
    return ~0u;
}

Drawable*
Viewer::getDrawable(unsigned index)
{
    if (_drawableVector.size() <= index)
        return 0;
    return _drawableVector[index].get();
}

unsigned
Viewer::getNumDrawables() const
{
    return _drawableVector.size();
}

SlaveCamera*
Viewer::getOrCreateSlaveCamera(const std::string& name)
{
    SlaveCamera* slaveCamera = getSlaveCamera(name);
    if (slaveCamera)
        return slaveCamera;
    if (!_renderer.valid())
        return 0;
    slaveCamera = _renderer->createSlaveCamera(*this, name);
    if (!slaveCamera)
        return 0;
    _slaveCameraVector.push_back(slaveCamera);
    return slaveCamera;
}

SlaveCamera*
Viewer::getSlaveCamera(const std::string& name)
{
    return getSlaveCamera(getSlaveCameraIndex(name));
}

unsigned
Viewer::getSlaveCameraIndex(const std::string& name)
{
    for (SlaveCameraVector::size_type i = 0; i < _slaveCameraVector.size(); ++i) {
        if (_slaveCameraVector[i]->getName() == name)
            return i;
    }
    return ~0u;
}

SlaveCamera*
Viewer::getSlaveCamera(unsigned index)
{
    if (_slaveCameraVector.size() <= index)
        return 0;
    return _slaveCameraVector[index].get();
}

unsigned
Viewer::getNumSlaveCameras() const
{
    return _slaveCameraVector.size();
}

void
Viewer::realize()
{
    if (isRealized())
        return;

    if (!_renderer.valid())
        return;

    // Setup a default config if there is none
    setupDefaultCameraConfigIfUnset();

    // Realize
    if (!_renderer->realize(*this)) {
        SG_LOG(SG_VIEW, SG_ALERT, "Renderer::realize() failed!");
        return;
    }

    osgViewer::Viewer::realize();
}

bool
Viewer::realizeDrawables()
{
    for (DrawableVector::iterator i = _drawableVector.begin(); i != _drawableVector.end(); ++i) {
        if (!(*i)->realize(*this)) {
            SG_LOG(SG_VIEW, SG_ALERT, "Unable to realize drawable \"" << (*i)->getName() << "\"!");
            return false;
        }
    }

    return true;
}

bool
Viewer::realizeSlaveCameras()
{
    for (SlaveCameraVector::iterator i = _slaveCameraVector.begin(); i != _slaveCameraVector.end(); ++i) {
        if (!(*i)->realize(*this)) {
            SG_LOG(SG_VIEW, SG_ALERT, "Unable to realize camera \"" << (*i)->getName() << "\"!");
            return false;
        }
    }

    return true;
}

void
Viewer::advance(double)
{
    if (_timeIncrement == SGTimeStamp::fromSec(0)) {
        // Flightgears current scheme - could be improoved
        _simTime = SGTimeStamp::now();
    } else {
        // Giving an explicit time increment makes sense in presence
        // of the video capture where we need deterministic
        // frame times and object positions for each picture.
        _simTime += _timeIncrement;
    }

    // This sets the frame stamps simulation time to simTime
    // and schedules a frame event
    osgViewer::Viewer::advance(_simTime.toSecs());
}

void
Viewer::updateTraversal()
{
#if FG_HAVE_HLA
    if (_viewerFederate.valid()) {
        if (_timeIncrement == SGTimeStamp::fromSec(0)) {
            if (!_viewerFederate->timeAdvanceAvailable()) {
                SG_LOG(SG_NETWORK, SG_ALERT, "Got error from federate update!");
                _viewerFederate->shutdown();
                _viewerFederate = 0;
            }
        } else {
            osg::FrameStamp* frameStamp = getViewerFrameStamp();
            SGTimeStamp timeStamp = SGTimeStamp::fromSec(frameStamp->getSimulationTime());
            if (!_viewerFederate->timeAdvance(timeStamp)) {
                SG_LOG(SG_NETWORK, SG_ALERT, "Got error from federate update!");
                _viewerFederate->shutdown();
                _viewerFederate = 0;
            }
        }
    }
#endif

    osgViewer::Viewer::updateTraversal();

    if (!_renderer->update(*this)) {
        SG_LOG(SG_VIEW, SG_ALERT, "Renderer::update() failed!");
    }
}

bool
Viewer::updateSlaveCameras()
{
    for (SlaveCameraVector::iterator i = _slaveCameraVector.begin(); i != _slaveCameraVector.end(); ++i) {
        if (!(*i)->update(*this)) {
            SG_LOG(SG_VIEW, SG_ALERT, "SlaveCamera::update() failed!");
            return false;
        }
    }
    return true;
}

void
Viewer::setReaderWriterOptions(simgear::SGReaderWriterOptions* readerWriterOptions)
{
    _readerWriterOptions = readerWriterOptions;
}

simgear::SGReaderWriterOptions*
Viewer::getReaderWriterOptions()
{
    return _readerWriterOptions.get();
}

void
Viewer::setSceneData(osg::Node* node)
{
    _sceneDataGroup->removeChildren(0, _sceneDataGroup->getNumChildren());
    insertSceneData(node);
}

void
Viewer::insertSceneData(osg::Node* node)
{
    _sceneDataGroup->addChild(node);
}

bool
Viewer::insertSceneData(const std::string& fileName, const osgDB::Options* options)
{
#if 0
    osg::ProxyNode* proxyNode = new osg::ProxyNode;
    if (options)
        proxyNode->setDatabaseOptions(options->clone(osg::CopyOp()));
    else
        proxyNode->setDatabaseOptions(_readerWriterOptions->clone(osg::CopyOp()));
    proxyNode->setFileName(0, fileName);
    insertSceneData(proxyNode);
    return true;
#else
    osg::ref_ptr<osg::Node> node = osgDB::readRefNodeFile(fileName, options);
    if (!node.valid())
        return false;
    insertSceneData(node.get());
    return true;
#endif
}

osg::Group*
Viewer::getSceneDataGroup()
{
    return _sceneDataGroup.get();
}

class Viewer::_PurgeLevelOfDetailNodesVisitor : public osg::NodeVisitor {
public:
    _PurgeLevelOfDetailNodesVisitor() :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    { }
    virtual ~_PurgeLevelOfDetailNodesVisitor()
    { }
  
    virtual void apply(osg::ProxyNode& node)
    {
        for (unsigned i = 0; i < node.getNumChildren(); ++i) {
            if (node.getFileName(i).empty())
                continue;
            node.removeChildren(i, node.getNumChildren() - i);
            break;
        }

        osg::NodeVisitor::apply(static_cast<osg::Group&>(node));
    }
    virtual void apply(osg::PagedLOD& node)
    {
        for (unsigned i = 0; i < node.getNumChildren(); ++i) {
            if (node.getFileName(i).empty())
                continue;
            node.removeChildren(i, node.getNumChildren() - i);
            break;
        }

        osg::NodeVisitor::apply(static_cast<osg::Group&>(node));
    }
};

void
Viewer::purgeLevelOfDetailNodes()
{
    _PurgeLevelOfDetailNodesVisitor purgeLevelOfDetailNodesVisitor;
    _sceneDataGroup->accept(purgeLevelOfDetailNodesVisitor);
}

osg::GraphicsContext::ScreenIdentifier
Viewer::getDefaultScreenIdentifier()
{
    osg::GraphicsContext::ScreenIdentifier screenIdentifier;
    screenIdentifier.readDISPLAY();
    if (screenIdentifier.displayNum < 0)
        screenIdentifier.displayNum = 0;
    if (screenIdentifier.screenNum < 0)
        screenIdentifier.screenNum = 0;
    return screenIdentifier;
}

osg::GraphicsContext::ScreenIdentifier
Viewer::getScreenIdentifier(const std::string& display)
{
    osg::GraphicsContext::ScreenIdentifier screenIdentifier;
    screenIdentifier.setScreenIdentifier(display);

    osg::GraphicsContext::ScreenIdentifier defaultScreenIdentifier;
    defaultScreenIdentifier = getDefaultScreenIdentifier();
    if (screenIdentifier.hostName.empty())
        screenIdentifier.hostName = defaultScreenIdentifier.hostName;
    if (screenIdentifier.displayNum < 0)
        screenIdentifier.displayNum = defaultScreenIdentifier.displayNum;
    if (screenIdentifier.screenNum < 0)
        screenIdentifier.screenNum = defaultScreenIdentifier.screenNum;
    
    return screenIdentifier;
}

osg::GraphicsContext::ScreenSettings
Viewer::getScreenSettings(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier)
{
    osg::GraphicsContext::ScreenSettings screenSettings;

    osg::GraphicsContext::WindowingSystemInterface* wsi;
    wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (!wsi) {
        SG_LOG(SG_VIEW, SG_ALERT, "No windowing system interface defined!");
        return screenSettings;
    }

    wsi->getScreenSettings(screenIdentifier, screenSettings);
    return screenSettings;
}

osg::GraphicsContext::Traits*
Viewer::getTraits(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier)
{
    osg::DisplaySettings* ds = _displaySettings.get();
    if (!ds)
        ds = osg::DisplaySettings::instance().get();

    osg::GraphicsContext::Traits* traits = new osg::GraphicsContext::Traits(ds);

    traits->hostName = screenIdentifier.hostName;
    traits->displayNum = screenIdentifier.displayNum;
    traits->screenNum = screenIdentifier.screenNum;
            
    // not seriously consider something different
    traits->doubleBuffer = true;

    osg::GraphicsContext::ScreenSettings screenSettings;
    screenSettings = getScreenSettings(screenIdentifier);

    traits->x = 0;
    traits->y = 0;
    traits->width = screenSettings.width;
    traits->height = screenSettings.height;

    return traits;
}

#ifdef __linux__
class Viewer::_ResetScreenSaverSwapCallback : public osg::GraphicsContext::SwapCallback {
public:
    _ResetScreenSaverSwapCallback() :
        _timeStamp(SGTimeStamp::fromSec(0))
    {
    }
    virtual ~_ResetScreenSaverSwapCallback()
    {
    }
    virtual void swapBuffersImplementation(osg::GraphicsContext* graphicsContext)
    {
        graphicsContext->swapBuffersImplementation();

        // This callback must be attached to this type of graphics context
        assert(dynamic_cast<osgViewer::GraphicsWindowX11*>(graphicsContext));

        // Reset the screen saver every 10 seconds
        SGTimeStamp timeStamp = SGTimeStamp::now();
        if (timeStamp < _timeStamp)
            return;
        _timeStamp = timeStamp + SGTimeStamp::fromSec(10);
        // Obviously runs in the draw thread. Thus, use the draw display.
        XResetScreenSaver(static_cast<osgViewer::GraphicsWindowX11*>(graphicsContext)->getDisplay());
    }
private:
    SGTimeStamp _timeStamp;
};
#endif

osg::GraphicsContext*
Viewer::createGraphicsContext(osg::GraphicsContext::Traits* traits)
{
    osg::GraphicsContext::WindowingSystemInterface* wsi;
    wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (!wsi) {
        SG_LOG(SG_VIEW, SG_ALERT, "No windowing system interface defined!");
        return 0;
    }

    osg::GraphicsContext* graphicsContext = wsi->createGraphicsContext(traits);
    if (!graphicsContext) {
        SG_LOG(SG_VIEW, SG_ALERT, "Unable to create window \"" << traits->windowName << "\"!");
        return 0;
    }

#ifdef __linux__
    if (dynamic_cast<osgViewer::GraphicsWindowX11*>(graphicsContext))
        graphicsContext->setSwapCallback(new _ResetScreenSaverSwapCallback);
#endif

    return graphicsContext;
}

#if FG_HAVE_HLA
const HLAViewerFederate*
Viewer::getViewerFederate() const
{
    return _viewerFederate.get();
}

HLAViewerFederate*
Viewer::getViewerFederate()
{
    return _viewerFederate.get();
}

void
Viewer::setViewerFederate(HLAViewerFederate* viewerFederate)
{
    if (!viewerFederate) {
        SG_LOG(SG_VIEW, SG_ALERT, "Viewer::setViewerFederate(): Setting the viewer federate to zero is not supported!");
        return;
    }
    if (_viewerFederate.valid()) {
        SG_LOG(SG_VIEW, SG_ALERT, "Viewer::setViewerFederate(): Setting the viewer federate twice is not supported!");
        return;
    }
    _viewerFederate = viewerFederate;
    _viewerFederate->attachToViewer(this);
}
#endif

} // namespace fgviewer
