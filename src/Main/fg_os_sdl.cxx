#include <stdlib.h>

#include <osgViewer/ViewerEventHandlers>
#include <osgViewer/Viewer>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include <SDL/SDL.h>

#include <Scenery/scenery.hxx>
#include "fg_os.hxx"
#include "globals.hxx"
#include "renderer.hxx"
#include "fg_props.hxx"
#include "WindowSystemAdapter.hxx"

using namespace flightgear;

//
// fg_os callback registration APIs
//

static int CurrentModifiers = 0;
static int CurrentMouseX = 0;
static int CurrentMouseY = 0;
static int CurrentMouseCursor = MOUSE_CURSOR_POINTER;
static int VidMask = SDL_OPENGL|SDL_RESIZABLE;

//
// fg_os implementation
//
static void initCursors();

static osg::ref_ptr<osgViewer::Viewer> viewer;
static osg::ref_ptr<osg::Camera> mainCamera;
static osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> gw;

void fgOSOpenWindow(bool stencil)
{
    int w = fgGetInt("/sim/startup/xsize");
    int h = fgGetInt("/sim/startup/ysize");
    int bpp = fgGetInt("/sim/rendering/bits-per-pixel");
    bool alpha = fgGetBool("/sim/rendering/clouds3d-enable");
    bool fullscreen = fgGetBool("/sim/startup/fullscreen");
    int cbits = (bpp <= 16) ?  5 :  8;
    int zbits = (bpp <= 16) ? 16 : 24;
    WindowSystemAdapter* wsa = WindowSystemAdapter::getWSA();

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE) == -1)
        throw sg_throwable(string("Failed to initialize SDL: ")
                           + SDL_GetError());
    atexit(SDL_Quit);

    SDL_WM_SetCaption("FlightGear", "FlightGear");

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, cbits);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, cbits);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, cbits);
    if(alpha)
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    if(bpp > 16 && stencil)
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zbits);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if(fullscreen) {
        VidMask |= SDL_FULLSCREEN;
    }
    SDL_Surface* screen = SDL_SetVideoMode(w, h, 16, VidMask);
    if ( screen == 0)
        throw sg_throwable(string("Failed to set SDL video mode: ")
                                   + SDL_GetError());

    // This enables keycode translation (e.g. capital letters when
    // shift is pressed, as well as i18n input methods).  Eventually,
    // we may want to port the input maps to specify <mod-shift>
    // explicitly, and will turn this off.
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    initCursors();
    fgSetMouseCursor(MOUSE_CURSOR_POINTER);

    // FIXME: we may not get what we asked for (especially in full
    // screen modes), so these need to be propagated back to the
    // property tree for the rest of the code to inspect...
    //
    int realw = screen->w;
    int realh = screen->h;
    viewer = new osgViewer::Viewer;
    viewer->setDatabasePager(FGScenery::getPagerSingleton());
    gw = viewer->setUpViewerAsEmbeddedInWindow(0, 0, realw, realh);
    GraphicsWindow* window = wsa->registerWindow(gw.get(), string("main"));
    window->flags |= GraphicsWindow::GUI;
    // now the main camera ...
    osg::Camera* camera = new osg::Camera;
    mainCamera = camera;
    // If a viewport isn't set on the camera, then it's hard to dig it
    // out of the SceneView objects in the viewer, and the coordinates
    // of mouse events are somewhat bizzare.
    camera->setViewport(new osg::Viewport(0, 0, realw, realh));
    camera->setProjectionResizePolicy(osg::Camera::FIXED);
    Camera3D* cam3D = wsa->registerCamera3D(window, camera, string("main"));
    cam3D->flags |= Camera3D::MASTER; 
    // Add as a slave for compatibility with the non-embedded osgViewer.
    viewer->addSlave(camera);
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

// Cheap trick to avoid typing GUIEventAdapter over and over...
class SDLKeyTranslator : osgGA::GUIEventAdapter
{
public:
    static int handleKey(int key, int raw, int keyup);
};

int SDLKeyTranslator::handleKey(int key, int raw, int keyup)
{
    using namespace osgGA;
    
    int modmask = 0;
    int osgKey = 0;
    if (key == 0)
        key = raw;
    // Don't pass capslock or numlock to the FGManipulator; SDL
    // already transforms the key properly, so FGManipulator will get
    // confused.
    if (key == SDLK_CAPSLOCK || key == SDLK_NUMLOCK)
        return -1;
    switch(key) {
    case SDLK_RSHIFT: modmask = KEYMOD_SHIFT;  osgKey = KEY_Shift_R;  break;
    case SDLK_LSHIFT: modmask = KEYMOD_SHIFT;  osgKey = KEY_Shift_L;  break;
    case SDLK_RCTRL:  modmask = KEYMOD_CTRL;  osgKey = KEY_Control_R;  break;
    case SDLK_LCTRL:  modmask = KEYMOD_CTRL;  osgKey = KEY_Control_L;  break;
    case SDLK_RALT:   modmask = KEYMOD_ALT;   osgKey = KEY_Alt_R;  break;
    case SDLK_LALT:   modmask = KEYMOD_ALT;   osgKey = KEY_Alt_L;  break;
    case SDLK_RMETA:  modmask = KEYMOD_META;  osgKey = KEY_Meta_R;  break;
    case SDLK_LMETA:  modmask = KEYMOD_META;  osgKey = KEY_Meta_L;  break;
    case SDLK_RSUPER: modmask = KEYMOD_SUPER; osgKey = KEY_Super_R;  break;
    case SDLK_LSUPER: modmask = KEYMOD_SUPER; osgKey = KEY_Super_L;  break;

    case SDLK_LEFT:  osgKey = KEY_Left;  break;
    case SDLK_UP:  osgKey = KEY_Up;  break;
    case SDLK_RIGHT:  osgKey = KEY_Right;  break;
    case SDLK_DOWN:  osgKey = KEY_Down;  break;
    case SDLK_PAGEUP:  osgKey = KEY_Page_Up;  break;
    case SDLK_PAGEDOWN:  osgKey = KEY_Page_Down;  break;
    case SDLK_HOME:  osgKey = KEY_Home;  break;
    case SDLK_END:  osgKey = KEY_End;  break;
    case SDLK_INSERT:  osgKey = KEY_Insert;  break;
    case SDLK_F1:  osgKey = KEY_F1;  break;
    case SDLK_F2:  osgKey = KEY_F2;  break;
    case SDLK_F3:  osgKey = KEY_F3;  break;
    case SDLK_F4:  osgKey = KEY_F4;  break;
    case SDLK_F5:  osgKey = KEY_F5;  break;
    case SDLK_F6:  osgKey = KEY_F6;  break;
    case SDLK_F7:  osgKey = KEY_F7;  break;
    case SDLK_F8:  osgKey = KEY_F8;  break;
    case SDLK_F9:  osgKey = KEY_F9;  break;
    case SDLK_F10:  osgKey = KEY_F10;  break;
    case SDLK_F11:  osgKey = KEY_F11;  break;
    case SDLK_F12:  osgKey = KEY_F12;  break;
    default:
        osgKey = key;
    }
    int keymod = 0;
    if(keyup) {
        CurrentModifiers &= ~modmask;
        keymod = CurrentModifiers | KEYMOD_RELEASED;
    } else {
        CurrentModifiers |= modmask;
        keymod = CurrentModifiers & ~KEYMOD_RELEASED;
    }
    return osgKey;
}

// FIXME: Integrate with existing fgExit() in util.cxx.
void fgOSExit(int code)
{
    viewer->setDone(true);
    exit(code);
}

// originally from osgexamples/osgviewerSDL.cpp
bool convertEvent(SDL_Event& event, osgGA::EventQueue& eventQueue)
{
    using namespace osgGA;
    switch (event.type) {

    case SDL_MOUSEMOTION:
        eventQueue.mouseMotion(event.motion.x, event.motion.y);
        return true;

    case SDL_MOUSEBUTTONDOWN:
        eventQueue.mouseButtonPress(event.button.x, event.button.y, event.button.button);
        return true;

    case SDL_MOUSEBUTTONUP:
        eventQueue.mouseButtonRelease(event.button.x, event.button.y, event.button.button);
        return true;

    case SDL_KEYUP:
    case SDL_KEYDOWN:
    {
        int realKey = SDLKeyTranslator::handleKey(event.key.keysym.unicode,
                                                  event.key.keysym.sym,
                                                  event.type == SDL_KEYUP);
        if (realKey < -1)
            return true;
        if (event.type == SDL_KEYUP)
            eventQueue.keyRelease((osgGA::GUIEventAdapter::KeySymbol)realKey);
        else
            eventQueue.keyPress((osgGA::GUIEventAdapter::KeySymbol)realKey);
        return true;
    }
    case SDL_VIDEORESIZE:
        if (SDL_SetVideoMode(event.resize.w, event.resize.h, 16, VidMask) == 0)
            throw sg_throwable(string("Failed to set SDL video mode: ")
                               + SDL_GetError());
        eventQueue.windowResize(0, 0, event.resize.w, event.resize.h );
        return true;

    default:
        break;
    }
    return false;
}

void fgOSMainLoop()
{
    while(1) {
        SDL_Event e;
        int key;
        while(SDL_PollEvent(&e)) {
            // pass the SDL event into the viewers event queue
            convertEvent(e, *(gw->getEventQueue()));

            switch(e.type) {
            case SDL_QUIT:
                fgOSExit(0);
                break;
            case SDL_VIDEORESIZE:
                gw->resized(0, 0, e.resize.w, e.resize.h );
                break;
            }
        }
        // draw the new frame
        viewer->frame();

        // Swap Buffers
        SDL_GL_SwapBuffers();
    }
}

int fgGetKeyModifiers()
{
    return CurrentModifiers;
}

void fgWarpMouse(int x, int y)
{
    globals->get_renderer()->getManipulator()->setMouseWarped();
    SDL_WarpMouse(x, y);
}

void fgOSInit(int* argc, char** argv)
{
    WindowSystemAdapter::setWSA(new WindowSystemAdapter);
}

void fgOSFullScreen()
{
    // Noop.  SDL must set fullscreen at window open time and cannot
    // change modes.
}

static struct cursor_rec {
    int name;
    SDL_Cursor* sdlCursor;
    int w;
    int h;
    int hotx;
    int hoty;
    const char *img[32]; // '.' == white, '#' == black, ' ' == transparent
} cursors[] = {
    { MOUSE_CURSOR_POINTER, 0, // must be first!
      10, 16, 1, 1,
      { "..        ",
        ".#.       ",
        ".##.      ",
        ".###.     ",
        ".####.    ",
        ".#####.   ",
        ".######.  ",
        ".#######. ",
        ".########.",
        ".#####....",
        ".##.##.   ",
        ".#. .##.  ",
        "..  .##.  ",
        "     .##. ",
        "     .##. ",
        "      ..  " } },
    { MOUSE_CURSOR_CROSSHAIR, 0,
      17, 17, 8, 8,
      { "       ...       ",
        "       .#.       ",
        "       .#.       ",
        "       .#.       ",
        "       .#.       ",
        "       .#.       ",
        "       .#.       ",
        "........#........",
        ".#######.#######.",
        "........#........",
        "       .#.       ",
        "       .#.       ",
        "       .#.       ",
        "       .#.       ",
        "       .#.       ",
        "       .#.       ",
        "       ...       " } },
    { MOUSE_CURSOR_WAIT, 0,
      16, 16, 7, 7,
      { "  .########.    ",
        "  .########.    ",
        "  .########.    ",
        " .##########.   ",
        ".##....#...##.  ",
        "##.....#....##..",
        "#......#.....###",
        "#.....###....###",
        "#.....###....###",
        "#....#.......###",
        "##..#.......##..",
        ".##........##.  ",
        " .##########.   ",
        "  .########.    ",
        "  .########.    ",
        "  .########.    " } },
    { MOUSE_CURSOR_LEFTRIGHT, 0,
      17, 9, 8, 4,
      { "    ..     ..    ",
        "   .#.     .#.   ",
        "  .##.......##.  ",
        " .#############. ",
        ".####.......####.",
        " .#############. ",
        "  .##.......##.  ",
        "   .#.     .#.   ",
        "    ..     ..    " } },
    { MOUSE_CURSOR_NONE, 0, // must come last!
      1, 1, 0, 0, { " " } },
};

#define NCURSORS (sizeof(cursors)/sizeof(struct cursor_rec))

void fgSetMouseCursor(int cursor)
{
    if(cursor == MOUSE_CURSOR_NONE) {
        SDL_ShowCursor(SDL_DISABLE);
        return;
    }
    SDL_ShowCursor(SDL_ENABLE);
    for(unsigned int i=0; i<NCURSORS; i++) {
        if(cursor == cursors[i].name) {
            CurrentMouseCursor = cursor;
            SDL_SetCursor(cursors[i].sdlCursor);
            return;
        }
    }
    // Default to pointer
    CurrentMouseCursor = MOUSE_CURSOR_POINTER;
    SDL_SetCursor(cursors[0].sdlCursor);
}

int fgGetMouseCursor()
{
    return CurrentMouseCursor;
}

static void initCursors()
{
    unsigned char mask[128], img[128];
    for(unsigned int i=0; i<NCURSORS; i++) {
        if(cursors[i].name == MOUSE_CURSOR_NONE) break;
        for(int j=0; j<128; j++) mask[j] = img[j] = 0;
        for(int y=0; y<cursors[i].h; y++) {
            for(int x=0; x<cursors[i].w; x++) {
                int byte = (4 * y) + (x >> 3);
                int bit = 1 << (7 - (x & 7));
                int pix = cursors[i].img[y][x];
                if(pix != ' ') { mask[byte] |= bit; }
                if(pix == '#') { img[byte] |= bit; }
            }
        }
        cursors[i].sdlCursor = SDL_CreateCursor(img, mask, 32, 32, 
                                                cursors[i].hotx,
                                                cursors[i].hoty);
    }
}

bool fgOSIsMainCamera(const osg::Camera*)
{
  return true;
}

bool fgOSIsMainContext(const osg::GraphicsContext*)
{
  return true;
}

osg::GraphicsContext* fgOSGetMainContext()
{
    return gw.get();
}
