#ifndef _FG_OS_HXX
#define _FG_OS_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string>
#include <vector>

#include <osg/ref_ptr>
#include <osg/Camera>
#include <osg/GraphicsContext>


enum { MOUSE_BUTTON_LEFT,
       MOUSE_BUTTON_MIDDLE,
       MOUSE_BUTTON_RIGHT };

enum { MOUSE_BUTTON_DOWN,
       MOUSE_BUTTON_UP };

enum { MOUSE_CURSOR_NONE,
       MOUSE_CURSOR_POINTER,
       MOUSE_CURSOR_WAIT,
       MOUSE_CURSOR_CROSSHAIR,
       MOUSE_CURSOR_LEFTRIGHT,
       MOUSE_CURSOR_TOPSIDE,
       MOUSE_CURSOR_BOTTOMSIDE,
       MOUSE_CURSOR_LEFTSIDE,
       MOUSE_CURSOR_RIGHTSIDE,
       MOUSE_CURSOR_TOPLEFT,
       MOUSE_CURSOR_TOPRIGHT,
       MOUSE_CURSOR_BOTTOMLEFT,
       MOUSE_CURSOR_BOTTOMRIGHT,
};

enum { KEYMOD_NONE     = 0,
       KEYMOD_RELEASED = 1, // Not a mod key, indicates "up" action
       KEYMOD_SHIFT    = 2,
       KEYMOD_CTRL     = 4,
       KEYMOD_ALT      = 8,
       KEYMOD_META     = 16,
       KEYMOD_SUPER    = 32,
       KEYMOD_HYPER    = 64,
       KEYMOD_MAX      = 128 };

// A note on key codes: none are defined here.  FlightGear has no
// hard-coded interpretations of codes other than modifier keys, so we
// can get away with that.  The only firm requirement is that the
// codes passed to the fgKeyHandler function be correctly interpreted
// by the PUI library.  Users who need to hard-code key codes
// (probably not a good idea in any case) can use the pu.hxx header
// for definitions.

//
// OS integration functions
//

void fgOSInit(int* argc, char** argv);
void fgOSOpenWindow(bool stencil);
void fgOSFullScreen();
int fgOSMainLoop();
void fgOSExit(int code);

void fgSetMouseCursor(int cursor);
int  fgGetMouseCursor();
void fgWarpMouse(int x, int y);

int  fgGetKeyModifiers();

//
// Callbacks and registration API
//

namespace osg { class Camera; class GraphicsContext; }
namespace osgGA { class GUIEventAdapter; }

typedef void (*fgIdleHandler)();
typedef void (*fgDrawHandler)();
typedef void (*fgWindowResizeHandler)(int w, int h);

typedef void (*fgKeyHandler)(int key, int keymod, int mousex, int mousey);
typedef void (*fgMouseClickHandler)(int button, int updown, int x, int y, bool mainWindow, const osgGA::GUIEventAdapter*);
typedef void (*fgMouseMotionHandler)(int x, int y);

void fgRegisterIdleHandler(fgIdleHandler func);
void fgRegisterDrawHandler(fgDrawHandler func);
void fgRegisterWindowResizeHandler(fgWindowResizeHandler func);

void fgRegisterKeyHandler(fgKeyHandler func);
void fgRegisterMouseClickHandler(fgMouseClickHandler func);
void fgRegisterMouseMotionHandler(fgMouseMotionHandler func);
#endif // _FG_OS_HXX
