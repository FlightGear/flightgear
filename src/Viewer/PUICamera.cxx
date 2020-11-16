// Copyright (C) 2017 James Turner
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

#include "PUICamera.hxx"

#include <osg/StateSet>
#include <osg/State>
#include <osg/Texture2D>
#include <osg/Version>
#include <osg/RenderInfo>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/BlendFunc>

#include <osg/NodeVisitor>
#include <osgGA/GUIEventAdapter>
#include <osgGA/GUIEventHandler>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>

#include <simgear/scene/util/SGNodeMasks.hxx>

#include <GUI/FlightGear_pu.h>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/locale.hxx>
#include <Viewer/CameraGroup.hxx>
#include <Viewer/FGEventHandler.hxx>
#include <Viewer/renderer.hxx>

#include <Input/input.hxx>
#include <Input/FGMouseInput.hxx>

// Old versions of PUI are missing these defines
#ifndef PU_SCROLL_UP_BUTTON
#define PU_SCROLL_UP_BUTTON 3
#endif
#ifndef PU_SCROLL_DOWN_BUTTON
#define PU_SCROLL_DOWN_BUTTON 4
#endif

using namespace flightgear;

double static_pixelRatio = 1.0;

class PUIDrawable : public osg::Drawable
{
public:
    PUIDrawable()
    {
        setUseDisplayList(false);
        setDataVariance(Object::DYNAMIC);
    }
    
    void drawImplementation(osg::RenderInfo& renderInfo) const override
    {
        osg::State* state = renderInfo.getState();
        state->setActiveTextureUnit(0);
        state->setClientActiveTextureUnit(0);
        state->disableAllVertexArrays();
        
        state->applyMode(GL_FOG, false);
        state->applyMode(GL_DEPTH_TEST, false);
        state->applyMode(GL_LIGHTING, false);

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushClientAttrib(~0u);

        glEnable    ( GL_BLEND        ) ;
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
        
        // reset pixel storage stuff for PLIB FNT drawing via glBitmap
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glPixelStorei(GL_PACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        
        puDisplay();

        glPopClientAttrib();
        glPopAttrib();
    }
    
    osg::Object* cloneType() const override { return new PUIDrawable; }
    osg::Object* clone(const osg::CopyOp&) const override { return new PUIDrawable; }

private:
};

class PUIEventHandler : public osgGA::GUIEventHandler
{
public:
    PUIEventHandler(PUICamera* cam) :
        _puiCamera(cam)
    {
        _mouse0RightButtonNode = fgGetNode("/devices/status/mice/mouse[0]/button[2]", true);
    }
    
    bool handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa, osg::Object *, osg::NodeVisitor *nv) override
    {
        if (ea.getHandled()) return false;
        
        // PUI expects increasing downward mouse coords
        const int fixedY = (ea.getMouseYOrientation() == osgGA::GUIEventAdapter::Y_INCREASING_UPWARDS) ?
                            ea.getWindowHeight() - ea.getY() : ea.getY();
        const int scaledX = static_cast<int>(ea.getX() / static_pixelRatio);
        const int scaledY = static_cast<int>(fixedY / static_pixelRatio);
        
        switch(ea.getEventType())
        {
            case(osgGA::GUIEventAdapter::DRAG):
                if (!_is_dragging)
                    return false;
                // No break
            case(osgGA::GUIEventAdapter::MOVE):
                return puMouse(scaledX, scaledY);

            case(osgGA::GUIEventAdapter::PUSH):
            case(osgGA::GUIEventAdapter::RELEASE):
            {
                // during splash/reset, either of these can return nullptr
                const auto input = globals->get_subsystem<FGInput>();
                const auto mouseSubsystem = input ? input->get_subsystem<FGMouseInput>() : nullptr;
                if (mouseSubsystem && !mouseSubsystem->isActiveModePassThrough()) {
                    return false;
                }

                bool mouse_up = (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE);
                bool handled = puMouse(osgButtonToPUI(ea), mouse_up, scaledX, scaledY);
                if (!mouse_up && handled)
                {
                    _is_dragging = true;
                }
                // Release drag if no more buttons are pressed
                else if (mouse_up && !ea.getButtonMask())
                {
                    _is_dragging = false;
                }
                return handled;
            }

            case(osgGA::GUIEventAdapter::KEYDOWN):
            case(osgGA::GUIEventAdapter::KEYUP):
            {
                const bool isKeyRelease = (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP);
                const int key = flightgear::FGEventHandler::translateKey(ea);
                bool handled = puKeyboard(key, isKeyRelease);
                return handled;
            }

            case osgGA::GUIEventAdapter::SCROLL:
            {
                const int button = buttonForScrollEvent(ea);
                if (button != PU_NOBUTTON) {
                    // sent both down and up events for a single scroll, for
                    // compatability
                    bool handled = puMouse(button, PU_DOWN, scaledX, scaledY);
                    handled |= puMouse(button, PU_UP, scaledX, scaledY);
                    return handled;
                }
                return false;
            }

            case (osgGA::GUIEventAdapter::RESIZE):
                _puiCamera->resizeUi(ea.getWindowWidth(), ea.getWindowHeight());
                break;

            default:
                return false;
        }
        return false;
    }
private:
    int osgButtonToPUI(const osgGA::GUIEventAdapter &ea) const
    {
        switch (ea.getButton()) {
        case osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON:
            return 0;
        case osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON:
            return 1;
        case osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON:
            return 2;
        }

        return 0;
    }
    
    int buttonForScrollEvent(const osgGA::GUIEventAdapter &ea) const
    {
        if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_2D) {
            int button = PU_NOBUTTON;
            if (ea.getScrollingDeltaY() > 0)
                button = PU_SCROLL_UP_BUTTON;
            else if (ea.getScrollingDeltaY() < 0)
                button = PU_SCROLL_DOWN_BUTTON;
            
#if defined(SG_MAC)
            // bug https://code.google.com/p/flightgear-bugs/issues/detail?id=1286
            // Mac (Cocoa) interprets shift+wheel as horizontal scroll
            if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT) {
                if (ea.getScrollingDeltaX() > 0) {
                    button = PU_SCROLL_UP_BUTTON;
                } else if (ea.getScrollingDeltaX() < 0) {
                    button = PU_SCROLL_DOWN_BUTTON;
                }
            }
#endif
            return button;
        } else if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP) {
            return PU_SCROLL_UP_BUTTON;
        }

        return PU_SCROLL_DOWN_BUTTON;
    }

    PUICamera*  _puiCamera = nullptr;
    bool        _is_dragging = false;

    SGPropertyNode_ptr _mouse0RightButtonNode;
};

// The pu getWindow callback is supposed to return a window ID that
// would allow drawing a GUI on different windows. All that stuff is
// broken in multi-threaded OSG, and we only have one GUI "window"
// anyway, so just return a constant.
int PUICamera::puGetWindow()
{
    return 1;
}

void PUICamera::puGetWindowSize(int* width, int* height)
{
    *width = 0;
    *height = 0;
    osg::Camera* camera = getGUICamera(CameraGroup::getDefault());
    if (!camera)
        return;

    osg::Viewport* vport = camera->getViewport();
    *width = static_cast<int>(vport->width() / static_pixelRatio);
    *height = static_cast<int>(vport->height() / static_pixelRatio);
}

void PUICamera::initPUI()
{
    puSetWindowFuncs(PUICamera::puGetWindow, nullptr,
                     PUICamera::puGetWindowSize, nullptr);
    puRealInit();
}
PUICamera::PUICamera() :
    osg::Camera()
{
}

PUICamera::~PUICamera()
{
    SG_LOG(SG_GL, SG_INFO, "Deleting PUI camera");

    // depending on if we're doing shutdown or reset, various things can be
    // null here.
    auto renderer = globals->get_renderer();
    auto view = renderer ? renderer->getView() : nullptr;
    if (view) {
        view->removeEventHandler(_eventHandler);
    }
}

void PUICamera::init(osg::Group* parent, osgViewer::View* view)
{
    setName("PUI FBO camera");
    
    _fboTexture = new osg::Texture2D;
    _fboTexture->setInternalFormat(GL_RGBA);
    _fboTexture->setResizeNonPowerOfTwoHint(false);
    _fboTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _fboTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    // setup the camera as render to texture
    setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    setViewMatrix(osg::Matrix::identity());
    setClearMask( GL_COLOR_BUFFER_BIT );
    setClearColor( osg::Vec4( 0.0, 0.0, 0.0, 0.0 ) );
    setAllowEventFocus(false);
    setCullingActive(false);
    setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    setRenderOrder(osg::Camera::PRE_RENDER);
    attach(osg::Camera::COLOR_BUFFER, _fboTexture);

    // set the camera's node mask, ensure the pick bit is clear
    setNodeMask(SG_NODEMASK_GUI_BIT);

// geode+drawable to call puDisplay, as a child of this FBO-camera
    osg::Geode* geode = new osg::Geode;
    geode->setName("PUIDrawableGeode");
    geode->addDrawable(new PUIDrawable);
    addChild(geode);

// geometry (full-screen quad) to draw the output
    _fullScreenQuad = new osg::Geometry;
    _fullScreenQuad = osg::createTexturedQuadGeometry(osg::Vec3(0.0, 0.0, 0.0),
                                               osg::Vec3(200.0, 0.0, 0.0),
                                               osg::Vec3(0.0, 200.0, 0.0));
    _fullScreenQuad->setSupportsDisplayList(false);
    _fullScreenQuad->setName("PUI fullscreen quad");

// state used for drawing the quad (not for rendering PUI, that's done in the
// drawbale above)
    osg::StateSet* stateSet = _fullScreenQuad->getOrCreateStateSet();
    stateSet->setRenderBinDetails(1001, "RenderBin");
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
    stateSet->setTextureAttribute(0, _fboTexture);
    
    // use GL_ONE becuase we pre-multiplied by alpha when building the FBO texture
    stateSet->setAttribute(new osg::BlendFunc(osg::BlendFunc::ONE, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

// geode to display the FSquad in the parent scene (which is GUI camera)
    osg::Geode* fsQuadGeode = new osg::Geode;
    fsQuadGeode->addDrawable(_fullScreenQuad);
    fsQuadGeode->setName("PUI fullscreen Geode");
    fsQuadGeode->setNodeMask(SG_NODEMASK_GUI_BIT);
    parent->addChild(this);
    parent->addChild(fsQuadGeode);
    
    osg::Camera* camera = getGUICamera(CameraGroup::getDefault());
    if (camera) {
        osg::Viewport* vport = camera->getViewport();
        resizeUi(vport->width(), vport->height());
    }

    // push_front so we keep the order of event handlers the opposite of
    // the rendering order (i.e top-most UI layer has the front-most event
    // handler)
    _eventHandler = new PUIEventHandler(this);
    view->getEventHandlers().push_front(_eventHandler);
}

// remove once we require OSG 3.4
void PUICamera::manuallyResizeFBO(int width, int height)
{
    _fboTexture->setTextureSize(width, height);
    _fboTexture->dirtyTextureObject();
}

void PUICamera::resizeUi(int width, int height)
{
    static_pixelRatio = fgGetDouble("/sim/rendering/gui-pixel-ratio", 1.0);
    const int scaledWidth = static_cast<int>(width / static_pixelRatio);
    const int scaledHeight = static_cast<int>(height / static_pixelRatio);
    
    setViewport(0, 0, scaledWidth, scaledHeight);

    osg::Camera::resize(scaledWidth, scaledHeight);
    resizeAttachments(scaledWidth, scaledHeight);

    const float puiZ = 1.0;
    // resize the full-screen quad
    osg::Vec3Array* fsQuadVertices = static_cast<osg::Vec3Array*>(_fullScreenQuad->getVertexArray());
    (*fsQuadVertices)[0] = osg::Vec3(0.0, height, puiZ);
    (*fsQuadVertices)[1] = osg::Vec3(0.0, 0.0, puiZ);
    (*fsQuadVertices)[2] = osg::Vec3(width, 0.0, puiZ);
    (*fsQuadVertices)[3] = osg::Vec3(width, height, puiZ);

    fsQuadVertices->dirty();
}

