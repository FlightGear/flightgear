#ifndef _MSC_VER // MSVC really needs a definition for wchar_t
#define _WCHAR_T_DEFINED 1 // Glut needs this, or else it tries to
                           // redefine it
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <simgear/compiler.h>

#include SG_GLUT_H

#include <plib/pu.h>

#include <Scenery/scenery.hxx>
#include "fg_os.hxx"
#include "globals.hxx"
#include "renderer.hxx"
#include "fg_props.hxx"

//
// Native glut callbacks.
// These translate the glut event model into osgGA events
//

static osg::ref_ptr<osgViewer::Viewer> viewer;
static osg::ref_ptr<osg::Camera> mainCamera;
static osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> gw;

static int GlutModifiers = 0;
static unsigned int getOSGModifiers(int modifiers);

static void callKeyHandler(int k, int mods, int x, int y)
{

    int key = 0;
    switch (k) {
    case 0x1b:            key = osgGA::GUIEventAdapter::KEY_Escape;  break;
    case '\n':            key = osgGA::GUIEventAdapter::KEY_Return; break;
    case '\b':            key = osgGA::GUIEventAdapter::KEY_BackSpace; break;
    case 0x7f:            key = osgGA::GUIEventAdapter::KEY_Delete; break;
    case '\t':            key = osgGA::GUIEventAdapter::KEY_Tab; break;
    case PU_KEY_LEFT:     key = osgGA::GUIEventAdapter::KEY_Left;      break;
    case PU_KEY_UP:       key = osgGA::GUIEventAdapter::KEY_Up;        break;
    case PU_KEY_RIGHT:    key = osgGA::GUIEventAdapter::KEY_Right;     break;
    case PU_KEY_DOWN:     key = osgGA::GUIEventAdapter::KEY_Down;      break;
    case PU_KEY_PAGE_UP:   key = osgGA::GUIEventAdapter::KEY_Page_Up;   break;
    case PU_KEY_PAGE_DOWN: key = osgGA::GUIEventAdapter::KEY_Page_Down; break;
    case PU_KEY_HOME:     key = osgGA::GUIEventAdapter::KEY_Home;      break;
    case PU_KEY_END:      key = osgGA::GUIEventAdapter::KEY_End;       break;
    case PU_KEY_INSERT:   key = osgGA::GUIEventAdapter::KEY_Insert;    break;
    case PU_KEY_F1:       key = osgGA::GUIEventAdapter::KEY_F1;        break;
    case PU_KEY_F2:       key = osgGA::GUIEventAdapter::KEY_F2;        break;
    case PU_KEY_F3:       key = osgGA::GUIEventAdapter::KEY_F3;        break;
    case PU_KEY_F4:       key = osgGA::GUIEventAdapter::KEY_F4;        break;
    case PU_KEY_F5:       key = osgGA::GUIEventAdapter::KEY_F5;        break;
    case PU_KEY_F6:       key = osgGA::GUIEventAdapter::KEY_F6;        break;
    case PU_KEY_F7:       key = osgGA::GUIEventAdapter::KEY_F7;        break;
    case PU_KEY_F8:       key = osgGA::GUIEventAdapter::KEY_F8;        break;
    case PU_KEY_F9:       key = osgGA::GUIEventAdapter::KEY_F9;        break;
    case PU_KEY_F10:      key = osgGA::GUIEventAdapter::KEY_F10;       break;
    case PU_KEY_F11:      key = osgGA::GUIEventAdapter::KEY_F11;       break;
    case PU_KEY_F12:      key = osgGA::GUIEventAdapter::KEY_F12;       break;
    default: key = k; break;
    }
    unsigned int osgModifiers = getOSGModifiers(mods);
    osgGA::EventQueue& queue = *gw->getEventQueue();
    queue.getCurrentEventState()->setModKeyMask(osgModifiers);
    if (mods & KEYMOD_RELEASED)
        queue.keyRelease((osgGA::GUIEventAdapter::KeySymbol)key);
    else
        queue.keyPress((osgGA::GUIEventAdapter::KeySymbol)key);
}

static void GLUTmotion (int x, int y)
{
    if (!gw.valid())
        return;
    gw->getEventQueue()->mouseMotion(x, y);
}

static void GLUTmouse (int button, int updown, int x, int y)
{
    GlutModifiers = glutGetModifiers();
    if (gw.valid()) {
        if (updown == 0)
            gw->getEventQueue()->mouseButtonPress(x, y, button+1);
        else
            gw->getEventQueue()->mouseButtonRelease(x, y, button+1);
    }
}

static void GLUTspecialkeyup(int k, int x, int y)
{
    GlutModifiers = glutGetModifiers();
    callKeyHandler(256 + k, fgGetKeyModifiers() | KEYMOD_RELEASED, x, y);
}

static void GLUTspecialkey(int k, int x, int y)
{
    GlutModifiers = glutGetModifiers();
    callKeyHandler(256 + k, fgGetKeyModifiers(), x, y);
}

static unsigned char release_keys[256];

static void GLUTkeyup(unsigned char k, int x, int y)
{
    GlutModifiers = glutGetModifiers();
    callKeyHandler(release_keys[k], fgGetKeyModifiers() | KEYMOD_RELEASED, x, y);
}

static void GLUTkey(unsigned char k, int x, int y)
{
    GlutModifiers = glutGetModifiers();
    release_keys[k] = k;
    if (k >= 1 && k <= 26) {
        release_keys[k + '@'] = k;
        release_keys[k + '`'] = k;
    } else if (k >= 'A' && k <= 'Z') {
        release_keys[k - '@'] = k;
        release_keys[tolower(k)] = k;
    } else if (k >= 'a' && k <= 'z') {
        release_keys[k - '`'] = k;
        release_keys[toupper(k)] = k;
    }
    callKeyHandler(k, fgGetKeyModifiers(), x, y);
}

static void GLUTdraw()
{
    viewer->frame();
    glutSwapBuffers();
    glutPostRedisplay();
}

static void GLUTreshape(int w, int h)
{
    // update the window dimensions, in case the window has been resized.
    gw->resized(gw->getTraits()->x, gw->getTraits()->y, w, h);
    gw->getEventQueue()->windowResize(gw->getTraits()->x, gw->getTraits()->y,
                                      w, h);
}

//
// fg_os API definition
//

void fgOSInit(int* argc, char** argv)
{
    glutInit(argc, argv);
}

void fgOSFullScreen()
{
    glutFullScreen();
}

void fgOSMainLoop()
{
    glutMainLoop();
}

void fgOSExit(int code)
{
    exit(code);
}

static int CurrentCursor = MOUSE_CURSOR_POINTER;

int fgGetMouseCursor()
{
    return CurrentCursor;
}

void fgSetMouseCursor(int cursor)
{
    CurrentCursor = cursor;
    if     (cursor == MOUSE_CURSOR_NONE)      cursor = GLUT_CURSOR_NONE;
    else if(cursor == MOUSE_CURSOR_POINTER)   cursor = GLUT_CURSOR_INHERIT;
    else if(cursor == MOUSE_CURSOR_WAIT)      cursor = GLUT_CURSOR_WAIT;
    else if(cursor == MOUSE_CURSOR_CROSSHAIR) cursor = GLUT_CURSOR_CROSSHAIR;
    else if(cursor == MOUSE_CURSOR_LEFTRIGHT) cursor = GLUT_CURSOR_LEFT_RIGHT;
    // Otherwise, pass it through unchanged...
    glutSetCursor(cursor);
}

void fgWarpMouse(int x, int y)
{
    globals->get_renderer()->getManipulator()->setMouseWarped();
    glutWarpPointer(x, y);
}

int fgGetKeyModifiers()
{
    int result = 0;
    if(GlutModifiers & GLUT_ACTIVE_SHIFT) result |= KEYMOD_SHIFT;
    if(GlutModifiers & GLUT_ACTIVE_CTRL)  result |= KEYMOD_CTRL;
    if(GlutModifiers & GLUT_ACTIVE_ALT)   result |= KEYMOD_ALT;
    return result;
}

static unsigned int getOSGModifiers(int glutModifiers)
{
    unsigned int result = 0;
    if (glutModifiers & GLUT_ACTIVE_SHIFT)
        result |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if (glutModifiers & GLUT_ACTIVE_CTRL)
        result |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if(glutModifiers & GLUT_ACTIVE_ALT)
        result |= osgGA::GUIEventAdapter::MODKEY_ALT;
    return result;
}

void fgOSOpenWindow(int w, int h, int bpp, bool alpha,
                    bool stencil, bool fullscreen)
{
    int mode = GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE;
    if(alpha) mode |= GLUT_ALPHA;
    if(stencil && bpp > 16) mode |= GLUT_STENCIL;

    glutInitDisplayMode(mode);
    glutInitWindowSize(w, h);
    if(!fgGetBool("/sim/startup/game-mode")) {
        glutCreateWindow("FlightGear");
    } else {
        char game_mode_str[20];
        SGPropertyNode *p = fgGetNode("/sim/frame-rate-throttle-hz", false);
        if (p) {
            int hz = p->getIntValue();
            snprintf(game_mode_str, 20, "%dx%d:%d@%d", w, h, bpp, hz);
        } else {
            snprintf(game_mode_str, 20, "%dx%d:%d", w, h, bpp);
        }
        glutGameModeString( game_mode_str );
        glutEnterGameMode();
    }

    // Register these here.  Calling them before the window is open
    // crashes.
    glutMotionFunc(GLUTmotion);
    glutPassiveMotionFunc(GLUTmotion);
    glutMouseFunc(GLUTmouse);
    glutSpecialUpFunc(GLUTspecialkeyup);
    glutSpecialFunc(GLUTspecialkey);
    glutKeyboardUpFunc(GLUTkeyup);
    glutKeyboardFunc(GLUTkey);
    glutDisplayFunc(GLUTdraw);
    glutReshapeFunc(GLUTreshape);
    // XXX
    int realw = w;
    int realh = h;
    viewer = new osgViewer::Viewer;
    gw = viewer->setUpViewerAsEmbeddedInWindow(0, 0, realw, realh);
    viewer->setDatabasePager(FGScenery::getPagerSingleton());
    // now the main camera ...
    //osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    osg::ref_ptr<osg::Camera> camera = viewer->getCamera();
    mainCamera = camera;
    osg::Camera::ProjectionResizePolicy rsp = osg::Camera::VERTICAL;
    // If a viewport isn't set on the camera, then it's hard to dig it
    // out of the SceneView objects in the viewer, and the coordinates
    // of mouse events are somewhat bizzare.
    camera->setViewport(new osg::Viewport(0, 0, realw, realh));
    camera->setProjectionResizePolicy(rsp);
    //viewer->addSlave(camera.get());
    viewer->setCameraManipulator(globals->get_renderer()->getManipulator());
    // Let FG handle the escape key with a confirmation
    viewer->setKeyEventSetsDone(0);
    osgViewer::StatsHandler* statsHandler = new osgViewer::StatsHandler;
    statsHandler->setKeyEventTogglesOnScreenStats('*');
    statsHandler->setKeyEventPrintsOutStats(0);
    viewer->addEventHandler(statsHandler);
    // The viewer won't start without some root.
    viewer->setSceneData(new osg::Group);
    globals->get_renderer()->setViewer(viewer.get());
}

// Noop; the graphics context is always current
void fgMakeCurrent()
{
}

bool fgOSIsMainCamera(const osg::Camera*)
{
  return true;
}

bool fgOSIsMainContext(const osg::GraphicsContext*)
{
  return true;
}
