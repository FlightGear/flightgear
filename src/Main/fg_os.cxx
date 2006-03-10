#ifndef _MSC_VER // MSVC really needs a definition for wchar_t
#define _WCHAR_T_DEFINED 1 // Glut needs this, or else it tries to
                           // redefine it
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include SG_GLUT_H

#include <plib/pu.h>

#include "fg_props.hxx"
#include "fg_os.hxx"

//
// fg_os callback registration APIs
// (These are not glut-specific)
//

static fgIdleHandler IdleHandler = 0;
static fgDrawHandler DrawHandler = 0;
static fgWindowResizeHandler WindowResizeHandler = 0;
static fgKeyHandler KeyHandler = 0;
static fgMouseClickHandler MouseClickHandler = 0;
static fgMouseMotionHandler MouseMotionHandler = 0;

// We need to flush all pending mouse move events past a mouse warp to avoid
// a race condition ending in warping twice and having huge increments for the
// second warp.
// I am not aware of such a flush function in glut. So we emulate that by
// ignoring mouse move events between a warp mouse and the next frame.
static bool mouseWarped = false;

void fgRegisterIdleHandler(fgIdleHandler func)
{
    IdleHandler = func;
}

void fgRegisterDrawHandler(fgDrawHandler func)
{
    DrawHandler = func;
}

void fgRegisterWindowResizeHandler(fgWindowResizeHandler func)
{
    WindowResizeHandler = func;
}

void fgRegisterKeyHandler(fgKeyHandler func)
{
    KeyHandler = func;
}

void fgRegisterMouseClickHandler(fgMouseClickHandler func)
{
    MouseClickHandler = func;
}

void fgRegisterMouseMotionHandler(fgMouseMotionHandler func)
{
    MouseMotionHandler = func;
}

//
// Native glut callbacks.
// These translate the glut event model into fg*Handler callbacks
//

static int GlutModifiers = 0;

static void callKeyHandler(int k, int mods, int x, int y)
{
    int puiup = mods & KEYMOD_RELEASED ? PU_UP : PU_DOWN;
    if(puKeyboard(k, puiup))
        return;
    if(KeyHandler) (*KeyHandler)(k, mods, x, y);
}

static void GLUTmotion (int x, int y)
{
    if (mouseWarped)
        return;
    if(MouseMotionHandler) (*MouseMotionHandler)(x, y);
}

static void GLUTmouse (int button, int updown, int x, int y)
{
    GlutModifiers = glutGetModifiers();
    if(MouseClickHandler) (*MouseClickHandler)(button, updown, x, y);
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

static void GLUTkeyup(unsigned char k, int x, int y)
{
    GlutModifiers = glutGetModifiers();
    callKeyHandler(k, fgGetKeyModifiers() | KEYMOD_RELEASED, x, y);
}

static void GLUTkey(unsigned char k, int x, int y)
{
    GlutModifiers = glutGetModifiers();
    callKeyHandler(k, fgGetKeyModifiers(), x, y);
}

static void GLUTidle()
{
    if(IdleHandler) (*IdleHandler)();
    mouseWarped = false;
}

static void GLUTdraw()
{
    if(DrawHandler) (*DrawHandler)();
    glutSwapBuffers();
    mouseWarped = false;
}

static void GLUTreshape(int w, int h)
{
    if(WindowResizeHandler) (*WindowResizeHandler)(w, h);
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
    mouseWarped = true;
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

void fgRequestRedraw()
{
    glutPostRedisplay();
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
    glutIdleFunc(GLUTidle);
    glutDisplayFunc(GLUTdraw);
    glutReshapeFunc(GLUTreshape);
}

