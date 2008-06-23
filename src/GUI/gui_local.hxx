#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include "trackball.h"

#if defined(WIN32) || defined(__CYGWIN32__)
#define WIN32_CURSOR_TWEAKS
// uncomment this for cursor to turn off when menu is disabled
// #define WIN32_CURSOR_TWEAKS_OFF
#elif (GLUT_API_VERSION >= 4 || GLUT_XLIB_IMPLEMENTATION >= 9)
#define X_CURSOR_TWEAKS
#endif

typedef enum {
	MOUSE_POINTER,
	MOUSE_YOKE,
	MOUSE_VIEW
} MouseMode;

extern MouseMode mouse_mode;
extern int gui_menu_on;

extern float curGuiQuat[4];
extern float GuiQuat_mat[4][4];

extern void initMouseQuat( void );
extern void Quat0( void );

extern void reInit(void);
