#include <stdlib.h>

#include <SDL/SDL.h>
#include <plib/pu.h>

#include "fg_os.hxx"

//
// fg_os callback registration APIs
//

static fgIdleHandler IdleHandler = 0;
static fgDrawHandler DrawHandler = 0;
static fgKeyHandler KeyHandler = 0;
static fgMouseClickHandler MouseClickHandler = 0;
static fgMouseMotionHandler MouseMotionHandler = 0;

static int CurrentModifiers = 0;
static int CurrentMouseX = 0;
static int CurrentMouseY = 0;
static bool NeedRedraw = false;

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
    // Noop.  SDL does not support window resize.
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

void fgOSOpenWindow(int w, int h, int bpp, bool alpha, bool fullscreen)
{
    int cbits = (bpp <= 16) ?  5 :  8;
    int zbits = (bpp <= 16) ? 16 : 32;

    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE); // FIXME: handle errors

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, cbits);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, cbits);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, cbits);
    if(alpha)
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zbits);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    int vidmask = SDL_OPENGL;
    if(fullscreen) vidmask |= SDL_FULLSCREEN;
    SDL_SetVideoMode(w, h, 16, vidmask); // FIXME: handle errors

    SDL_WM_SetCaption("FlightGear", "FlightGear");

    // This enables keycode translation (e.g. capital letters when
    // shift is pressed, as well as i18n input methods).  Eventually,
    // we may want to port the input maps to specify <mod-shift>
    // explicitly, and will turn this off.
    SDL_EnableUNICODE(1);

    // FIXME: we may not get what we asked for (especially in full
    // screen modes), so these need to be propagated back to the
    // property tree for the rest of the code to inspect...
    //
    // SDL_Surface* screen = SDL_GetVideoSurface();
    // int realw = screen->w;
    // int realh = screen->h;
}

static void handleKey(int key, int keyup)
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
    if(keyup) CurrentModifiers &= ~modmask;
    else      CurrentModifiers |= modmask;
    if(modmask == 0 && KeyHandler)
        (*KeyHandler)(key, CurrentModifiers, CurrentMouseX, CurrentMouseY);
}

// FIXME: need to export this and get the rest of FlightGear to use
// it, the SDL hook is required to do things like reset the video
// mode and key repeat.  Integrate with existing fgExit() in util.cxx.
void fgOSExit(int code)
{
    SDL_Quit();
    exit(code);
}

void fgOSMainLoop()
{
    while(1) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            switch(e.type) {
            case SDL_QUIT:
                fgOSExit(0);
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                handleKey(e.key.keysym.unicode, e.key.state == SDL_RELEASED);
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

// Figure out what to do with these
void fgSetMouseCursor(int cursor) {}
int  fgGetMouseCursor() { return MOUSE_CURSOR_POINTER; }
