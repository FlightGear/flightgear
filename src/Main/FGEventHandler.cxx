#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osg/Math>
#include <osg/Viewport>
#include <osgViewer/Viewer>

#include <plib/pu.h>
#include <Main/fg_props.hxx>
#include "CameraGroup.hxx"
#include "FGEventHandler.hxx"
#include "WindowSystemAdapter.hxx"
#include "renderer.hxx"

#if !defined(X_DISPLAY_MISSING)
#define X_DOUBLE_SCROLL_BUG 1
#endif

#ifdef SG_MAC
// hack - during interactive resize on Mac, OSG queues and then flushes
// a large number of resize events, without doing any drawing.
extern void puCleanUpJunk ( void ) ;
#endif

namespace flightgear
{
const int displayStatsKey = 1;
const int printStatsKey = 2;


// The manipulator is responsible for updating a Viewer's camera. Its
// event handling method is also a convenient place to run the FG idle
// and draw handlers.

FGEventHandler::FGEventHandler() :
    idleHandler(0),
    keyHandler(0),
    mouseClickHandler(0),
    mouseMotionHandler(0),
    statsHandler(new FGStatsHandler),
    statsEvent(new osgGA::GUIEventAdapter),
    statsType(osgViewer::StatsHandler::NO_STATS),
    currentModifiers(0),
    resizable(true),
    mouseWarped(false),
    scrollButtonPressed(false),
    changeStatsCameraRenderOrder(false)
{
    using namespace osgGA;
    statsHandler->setKeyEventTogglesOnScreenStats(displayStatsKey);
    statsHandler->setKeyEventPrintsOutStats(printStatsKey);
    statsEvent->setEventType(GUIEventAdapter::KEYDOWN);

    // OSG reports NumPad keycodes independent of the NumLock modifier.
    // Both KP-4 and KP-Left are reported as KEY_KP_Left (0xff96), so we
    // have to generate the locked keys ourselves.
    numlockKeyMap[GUIEventAdapter::KEY_KP_Insert]  = '0';
    numlockKeyMap[GUIEventAdapter::KEY_KP_End] = '1';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Down] = '2';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Page_Down] = '3';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Left] = '4';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Begin] = '5';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Right] = '6';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Home] = '7';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Up] = '8';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Page_Up] = '9';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Delete] = '.';

    // mapping when NumLock is off
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_Insert]     = PU_KEY_INSERT;
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_End]        = PU_KEY_END;
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_Down]       = PU_KEY_DOWN;
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_Page_Down]  = PU_KEY_PAGE_DOWN;
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_Left]       = PU_KEY_LEFT;
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_Begin]      = '5';
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_Right]      = PU_KEY_RIGHT;
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_Home]       = PU_KEY_HOME;
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_Up]         = PU_KEY_UP;
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_Page_Up]    = PU_KEY_PAGE_UP;
    noNumlockKeyMap[GUIEventAdapter::KEY_KP_Delete]     = 127;

    for (int i = 0; i < 128; i++)
        release_keys[i] = i;
}

namespace
{
// Translate OSG modifier mask to FG modifier mask.
int osgToFGModifiers(int modifiers)
{
    int result = 0;
    if (modifiers & osgGA::GUIEventAdapter::MODKEY_SHIFT)
        result |= KEYMOD_SHIFT;

    if (modifiers & osgGA::GUIEventAdapter::MODKEY_CTRL)
        result |= KEYMOD_CTRL;

    if (modifiers & osgGA::GUIEventAdapter::MODKEY_ALT)
        result |= KEYMOD_ALT;

    if (modifiers & osgGA::GUIEventAdapter::MODKEY_META)
        result |= KEYMOD_META;

    if (modifiers & osgGA::GUIEventAdapter::MODKEY_SUPER)
        result |= KEYMOD_SUPER;

    if (modifiers & osgGA::GUIEventAdapter::MODKEY_HYPER)
        result |= KEYMOD_HYPER;
    return result;
}
}

#if 0
void FGEventHandler::init(const osgGA::GUIEventAdapter& ea,
                          osgGA::GUIActionAdapter& us)
{
    currentModifiers = osgToFGModifiers(ea.getModKeyMask());
    (void)handle(ea, us);
}
#endif

// Calculate event coordinates in the viewport of the GUI camera, if
// possible. Otherwise return false and (-1, -1).
namespace
{
bool
eventToViewport(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us,
                int& x, int& y)
{
    x = -1;
    y = -1;

    const osg::GraphicsContext* eventGC = ea.getGraphicsContext();
    const osg::GraphicsContext::Traits* traits = eventGC->getTraits();
    osg::Camera* guiCamera = getGUICamera(CameraGroup::getDefault());
    if (!guiCamera)
        return false;
    osg::Viewport* vport = guiCamera->getViewport();
    if (!vport)
        return false;
    
    // Scale x, y to the dimensions of the window
    double wx = (((ea.getX() - ea.getXmin()) / (ea.getXmax() - ea.getXmin()))
                 * (float)traits->width);
    double wy = (((ea.getY() - ea.getYmin()) / (ea.getYmax() - ea.getYmin()))
                 * (float)traits->height);
    if (vport->x() <= wx && wx <= vport->x() + vport->width()
        && vport->y() <= wy && wy <= vport->y() + vport->height()) {
        // Finally, into viewport coordinates. Change y to "increasing
        // downwards".
        x = wx - vport->x();
        y = vport->height() - (wy - vport->y());
        return true;
    } else {
        return false;
    }
}
}

bool FGEventHandler::handle(const osgGA::GUIEventAdapter& ea,
                            osgGA::GUIActionAdapter& us)
{
    int x = 0;
    int y = 0;

    switch (ea.getEventType()) {
    case osgGA::GUIEventAdapter::FRAME:
        mouseWarped = false;
        handleStats(us);
        return true;
    case osgGA::GUIEventAdapter::KEYDOWN:
    case osgGA::GUIEventAdapter::KEYUP:
    {
        int key, modmask;
        handleKey(ea, key, modmask);
        eventToViewport(ea, us, x, y);
        if (keyHandler)
            (*keyHandler)(key, modmask, x, y);
        return true;
    }
    case osgGA::GUIEventAdapter::PUSH:
    case osgGA::GUIEventAdapter::RELEASE:
    {
        bool mainWindow = eventToViewport(ea, us, x, y);
        int button = 0;
        switch (ea.getButton()) {
        case osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON:
            button = 0;
            break;
        case osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON:
            button = 1;
            break;
        case osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON:
            button = 2;
            break;
        }
        if (mouseClickHandler)
            (*mouseClickHandler)(button,
                                 (ea.getEventType()
                                  == osgGA::GUIEventAdapter::RELEASE), x, y, mainWindow, &ea);
        return true;
    }
    case osgGA::GUIEventAdapter::SCROLL:
    {
        bool mainWindow = eventToViewport(ea, us, x, y);
#ifdef X_DOUBLE_SCROLL_BUG
        scrollButtonPressed = !scrollButtonPressed;
        if (!scrollButtonPressed) // Drop the button release event
            return true;
#endif
        int button;
        if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_2D) {
            if (ea.getScrollingDeltaY() > 0)
                button = 3;
            else if (ea.getScrollingDeltaY() < 0)
                button = 4;
            else
                button = -1;
        } else if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP)
            button = 3;
        else
            button = 4;
        if (mouseClickHandler && button != -1) {
            (*mouseClickHandler)(button, 0, x, y, mainWindow, &ea);
            (*mouseClickHandler)(button, 1, x, y, mainWindow, &ea);
        }
        return true;
    }
    case osgGA::GUIEventAdapter::MOVE:
    case osgGA::GUIEventAdapter::DRAG:
        // If we warped the mouse, then disregard all pointer motion
        // events for this frame. We really want to flush the event
        // queue of mouse events, but don't have the ability to do
        // that with osgViewer.
        if (mouseWarped)
            return true;
        if (eventToViewport(ea, us, x, y) && mouseMotionHandler)
            (*mouseMotionHandler)(x, y);
        return true;
    case osgGA::GUIEventAdapter::RESIZE:
        SG_LOG(SG_VIEW, SG_DEBUG, "FGEventHandler::handle: RESIZE event " << ea.getWindowHeight() << " x " << ea.getWindowWidth() << ", resizable: " << resizable);
        CameraGroup::getDefault()->resized();
        if (resizable)
          globals->get_renderer()->resize(ea.getWindowWidth(), ea.getWindowHeight());
        statsHandler->handle(ea, us);
      #ifdef SG_MAC
        // work around OSG Cocoa-Viewer issue with resize event handling,
        // where resize events are queued up, then dispatched in a batch, with
        // no interveningd drawing calls.
        puCleanUpJunk();
      #endif
        return true;
     case osgGA::GUIEventAdapter::CLOSE_WINDOW:
    case osgGA::GUIEventAdapter::QUIT_APPLICATION:
        fgOSExit(0);
        return true;
    default:
        return false;
    }
}

void FGEventHandler::handleKey(const osgGA::GUIEventAdapter& ea, int& key,
                               int& modifiers)
{
    using namespace osgGA;
    key = ea.getKey();
    // XXX Probably other translations are needed too.
    switch (key) {
    case GUIEventAdapter::KEY_Escape:      key = 0x1b; break;
    case GUIEventAdapter::KEY_Return:      key = '\n'; break;
    case GUIEventAdapter::KEY_BackSpace:   key = '\b'; break;
    case GUIEventAdapter::KEY_Delete:      key = 0x7f; break;
    case GUIEventAdapter::KEY_Tab:         key = '\t'; break;
    case GUIEventAdapter::KEY_Left:        key = PU_KEY_LEFT;      break;
    case GUIEventAdapter::KEY_Up:          key = PU_KEY_UP;        break;
    case GUIEventAdapter::KEY_Right:       key = PU_KEY_RIGHT;     break;
    case GUIEventAdapter::KEY_Down:        key = PU_KEY_DOWN;      break;
    case GUIEventAdapter::KEY_Page_Up:     key = PU_KEY_PAGE_UP;   break;
    case GUIEventAdapter::KEY_Page_Down:   key = PU_KEY_PAGE_DOWN; break;
    case GUIEventAdapter::KEY_Home:        key = PU_KEY_HOME;      break;
    case GUIEventAdapter::KEY_End:         key = PU_KEY_END;       break;
    case GUIEventAdapter::KEY_Insert:      key = PU_KEY_INSERT;    break;
    case GUIEventAdapter::KEY_F1:          key = PU_KEY_F1;        break;
    case GUIEventAdapter::KEY_F2:          key = PU_KEY_F2;        break;
    case GUIEventAdapter::KEY_F3:          key = PU_KEY_F3;        break;
    case GUIEventAdapter::KEY_F4:          key = PU_KEY_F4;        break;
    case GUIEventAdapter::KEY_F5:          key = PU_KEY_F5;        break;
    case GUIEventAdapter::KEY_F6:          key = PU_KEY_F6;        break;
    case GUIEventAdapter::KEY_F7:          key = PU_KEY_F7;        break;
    case GUIEventAdapter::KEY_F8:          key = PU_KEY_F8;        break;
    case GUIEventAdapter::KEY_F9:          key = PU_KEY_F9;        break;
    case GUIEventAdapter::KEY_F10:         key = PU_KEY_F10;       break;
    case GUIEventAdapter::KEY_F11:         key = PU_KEY_F11;       break;
    case GUIEventAdapter::KEY_F12:         key = PU_KEY_F12;       break;
    case GUIEventAdapter::KEY_KP_Enter:    key = '\r'; break;
    case GUIEventAdapter::KEY_KP_Add:      key = '+';  break;
    case GUIEventAdapter::KEY_KP_Divide:   key = '/';  break;
    case GUIEventAdapter::KEY_KP_Multiply: key = '*';  break;
    case GUIEventAdapter::KEY_KP_Subtract: key = '-';  break;
    }
    osgGA::GUIEventAdapter::EventType eventType = ea.getEventType();

#ifdef __APPLE__
    // Num Lock is always true on Mac
    std::map<int, int>::iterator numPadIter = numlockKeyMap.find(key);
    if (numPadIter != numlockKeyMap.end()) {
        key = numPadIter->second;
    }
#else
    if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_NUM_LOCK)
    {
        // NumLock on: map to numeric keys
        std::map<int, int>::iterator numPadIter = numlockKeyMap.find(key);
        if (numPadIter != numlockKeyMap.end()) {
            key = numPadIter->second;
        }
    }
    else
    {
        // NumLock off: map to PU arrow keys
        std::map<int, int>::iterator numPadIter = noNumlockKeyMap.find(key);
        if (numPadIter != noNumlockKeyMap.end()) {
            key = numPadIter->second;
        }
    }
#endif

    modifiers = osgToFGModifiers(ea.getModKeyMask());
    currentModifiers = modifiers;
    if (eventType == osgGA::GUIEventAdapter::KEYUP)
        modifiers |= KEYMOD_RELEASED;

    // Release the letter key, for which the key press was reported. This
    // is to deal with Ctrl-press -> a-press -> Ctrl-release -> a-release
    // correctly.
    if (key >= 0 && key < 128) {
        if (modifiers & KEYMOD_RELEASED) {
            key = release_keys[key];
        } else {
            release_keys[key] = key;
            if (key >= 1 && key <= 26) {
                release_keys[key + '@'] = key;
                release_keys[key + '`'] = key;
            } else if (key >= 'A' && key <= 'Z') {
                release_keys[key - '@'] = key;
                release_keys[tolower(key)] = key;
            } else if (key >= 'a' && key <= 'z') {
                release_keys[key - '`'] = key;
                release_keys[toupper(key)] = key;
            }
        }
    }
}

void FGEventHandler::handleStats(osgGA::GUIActionAdapter& us)
{
    static SGPropertyNode_ptr display = fgGetNode("/sim/rendering/on-screen-statistics", true);
    static SGPropertyNode_ptr print = fgGetNode("/sim/rendering/print-statistics", true);

    int type = display->getIntValue() % osgViewer::StatsHandler::LAST;
    if (type != statsType) {
        statsEvent->setKey(displayStatsKey);
        do {
            statsType = (statsType + 1) % osgViewer::StatsHandler::LAST;
            statsHandler->handle(*statsEvent, us);
            if (changeStatsCameraRenderOrder) {
                statsHandler->getCamera()->setRenderOrder(osg::Camera::POST_RENDER, 99999);
                changeStatsCameraRenderOrder = false;
            }
        } while (statsType != type);

        display->setIntValue(statsType);
    }

    if (print->getBoolValue()) {
        statsEvent->setKey(printStatsKey);
        statsHandler->handle(*statsEvent, us);
        print->setBoolValue(false);
    }
}

void eventToWindowCoords(const osgGA::GUIEventAdapter* ea,
                         double& x, double& y)
{
    using namespace osg;
    const GraphicsContext* gc = ea->getGraphicsContext();
    const GraphicsContext::Traits* traits = gc->getTraits() ;
    // Scale x, y to the dimensions of the window
    x = (((ea->getX() - ea->getXmin()) / (ea->getXmax() - ea->getXmin()))
         * (double)traits->width);
    y = (((ea->getY() - ea->getYmin()) / (ea->getYmax() - ea->getYmin()))
         * (double)traits->height);
    if (ea->getMouseYOrientation() == osgGA::GUIEventAdapter::Y_INCREASING_DOWNWARDS)
        y = (double)traits->height - y;
}

void eventToWindowCoordsYDown(const osgGA::GUIEventAdapter* ea,
                              double& x, double& y)
{
    using namespace osg;
    const GraphicsContext* gc = ea->getGraphicsContext();
    const GraphicsContext::Traits* traits = gc->getTraits() ;
    // Scale x, y to the dimensions of the window
    x = (((ea->getX() - ea->getXmin()) / (ea->getXmax() - ea->getXmin()))
         * (double)traits->width);
    y = (((ea->getY() - ea->getYmin()) / (ea->getYmax() - ea->getYmin()))
         * (double)traits->height);
    if (ea->getMouseYOrientation() == osgGA::GUIEventAdapter::Y_INCREASING_UPWARDS)
        y = (double)traits->height - y;
}
}
