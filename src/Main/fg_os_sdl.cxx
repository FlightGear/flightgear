#include <stdlib.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include <SDL/SDL.h>
#include <plib/pu.h>

#include "fg_os.hxx"

//
// fg_os callback registration APIs
//

static fgIdleHandler IdleHandler = 0;
static fgDrawHandler DrawHandler = 0;
static fgWindowResizeHandler WindowResizeHandler = 0;
static fgKeyHandler KeyHandler = 0;
static fgMouseClickHandler MouseClickHandler = 0;
static fgMouseMotionHandler MouseMotionHandler = 0;

static int CurrentModifiers = 0;
static int CurrentMouseX = 0;
static int CurrentMouseY = 0;
static int CurrentMouseCursor = MOUSE_CURSOR_POINTER;
static bool NeedRedraw = false;
static int VidMask = SDL_OPENGL|SDL_RESIZABLE;

void fgRegisterIdleHandler(fgIdleHandler func)
{
    IdleHandler = func;
}

void fgRegisterDrawHandler(fgDrawHandler func)
{
    DrawHandler = func;
    NeedRedraw = true;
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
// fg_os implementation
//
static void initCursors();

void fgOSOpenWindow(int w, int h, int bpp,
                    bool alpha, bool stencil, bool fullscreen)
{
    int cbits = (bpp <= 16) ?  5 :  8;
    int zbits = (bpp <= 16) ? 16 : 24;

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
    if (SDL_SetVideoMode(w, h, 16, VidMask) == 0)
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
    // SDL_Surface* screen = SDL_GetVideoSurface();
    // int realw = screen->w;
    // int realh = screen->h;
}


struct keydata {
    keydata() : unicode(0), mod(0) {};
    Uint16 unicode;
    unsigned int mod;
} keys[SDLK_LAST];


static void handleKey(int key, int raw, int keyup)
{
    int modmask = 0;
    switch(key) {
    case SDLK_RSHIFT: modmask = KEYMOD_SHIFT; break;
    case SDLK_LSHIFT: modmask = KEYMOD_SHIFT; break;
    case SDLK_RCTRL:  modmask = KEYMOD_CTRL;  break;
    case SDLK_LCTRL:  modmask = KEYMOD_CTRL;  break;
    case SDLK_RALT:   modmask = KEYMOD_ALT;   break;
    case SDLK_LALT:   modmask = KEYMOD_ALT;   break;

    case SDLK_LEFT:     key = PU_KEY_LEFT;      break;
    case SDLK_UP:       key = PU_KEY_UP;        break;
    case SDLK_RIGHT:    key = PU_KEY_RIGHT;     break;
    case SDLK_DOWN:     key = PU_KEY_DOWN;      break;
    case SDLK_PAGEUP:   key = PU_KEY_PAGE_UP;   break;
    case SDLK_PAGEDOWN: key = PU_KEY_PAGE_DOWN; break;
    case SDLK_HOME:     key = PU_KEY_HOME;      break;
    case SDLK_END:      key = PU_KEY_END;       break;
    case SDLK_INSERT:   key = PU_KEY_INSERT;    break;
    case SDLK_F1:       key = PU_KEY_F1;        break;
    case SDLK_F2:       key = PU_KEY_F2;        break;
    case SDLK_F3:       key = PU_KEY_F3;        break;
    case SDLK_F4:       key = PU_KEY_F4;        break;
    case SDLK_F5:       key = PU_KEY_F5;        break;
    case SDLK_F6:       key = PU_KEY_F6;        break;
    case SDLK_F7:       key = PU_KEY_F7;        break;
    case SDLK_F8:       key = PU_KEY_F8;        break;
    case SDLK_F9:       key = PU_KEY_F9;        break;
    case SDLK_F10:      key = PU_KEY_F10;       break;
    case SDLK_F11:      key = PU_KEY_F11;       break;
    case SDLK_F12:      key = PU_KEY_F12;       break;
    }

    // Keypad codes.  This is a situation where we *don't* want the
    // Unicode cooking for our input.  Oddly, neither PUI nor Glut
    // define these anywhere, so I lifted the numbers out of
    // FlightGear's keyboard.xml.  Some unused code are therefore
    // missing.
    switch(raw) {
    case SDLK_KP0:      key = 364; break;
    case SDLK_KP1:      key = 363; break;
    case SDLK_KP2:      key = 359; break;
    case SDLK_KP3:      key = 361; break;
    case SDLK_KP4:      key = 356; break;
    case SDLK_KP5:      key = 309; break;
    case SDLK_KP6:      key = 358; break;
    case SDLK_KP7:      key = 362; break;
    case SDLK_KP8:      key = 357; break;
    case SDLK_KP9:      key = 360; break;
    case SDLK_KP_ENTER: key = 269; break;
    }

    int keymod = 0;
    if(keyup) {
        CurrentModifiers &= ~modmask;
        keymod = CurrentModifiers | KEYMOD_RELEASED;
    } else {
        CurrentModifiers |= modmask;
        keymod = CurrentModifiers & ~KEYMOD_RELEASED;
    }

    if(modmask == 0 && KeyHandler) {
        if (keymod & KEYMOD_RELEASED) {
            if (keys[raw].mod) {
                key = keys[raw].unicode;
                keys[raw].mod = 0;
            }
        } else {
            keys[raw].mod = keymod;
            keys[raw].unicode = key;
        }
        (*KeyHandler)(key, keymod, CurrentMouseX, CurrentMouseY);
    }
}

// FIXME: Integrate with existing fgExit() in util.cxx.
void fgOSExit(int code)
{
    exit(code);
}

void fgOSMainLoop()
{
    while(1) {
        SDL_Event e;
        int key;
        while(SDL_PollEvent(&e)) {
            switch(e.type) {
            case SDL_QUIT:
                fgOSExit(0);
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                key = e.key.keysym.unicode;
                if(key == 0) key = e.key.keysym.sym;
                handleKey(key, e.key.keysym.sym, e.key.state == SDL_RELEASED);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                // Note offset: SDL uses buttons 1,2,3 not 0,1,2
                CurrentMouseX = e.button.x;
                CurrentMouseY = e.button.y;
                if(MouseClickHandler)
                    (*MouseClickHandler)(e.button.button - 1,
                                         e.button.state == SDL_RELEASED,
                                         e.button.x, e.button.y);
                break;
            case SDL_MOUSEMOTION:
                CurrentMouseX = e.motion.x;
                CurrentMouseY = e.motion.y;
                if(MouseMotionHandler)
                    (*MouseMotionHandler)(e.motion.x, e.motion.y);
                break;
            case SDL_VIDEORESIZE:
                if (SDL_SetVideoMode(e.resize.w, e.resize.h, 16, VidMask) == 0)
                    throw sg_throwable(string("Failed to set SDL video mode: ")
                            + SDL_GetError());

                if (WindowResizeHandler)
                    (*WindowResizeHandler)(e.resize.w, e.resize.h);
                break;
            }
        }
        if(IdleHandler) (*IdleHandler)();
        if(NeedRedraw && DrawHandler) {
            (*DrawHandler)();
            SDL_GL_SwapBuffers();
            NeedRedraw = false;
        }
    }
}

int fgGetKeyModifiers()
{
    return CurrentModifiers;
}

void fgWarpMouse(int x, int y)
{
    SDL_Event e[10];
    SDL_PumpEvents();
    SDL_PeepEvents(e, 10, SDL_GETEVENT, SDL_MOUSEMOTIONMASK);
    SDL_WarpMouse(x, y);
}

void fgRequestRedraw()
{
    NeedRedraw = true;
}

void fgOSInit(int* argc, char** argv)
{
    // Nothing to do here.  SDL has no command line options.
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
    char *img[32]; // '.' == white, '#' == black, ' ' == transparent
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
