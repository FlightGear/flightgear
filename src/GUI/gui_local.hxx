#include "trackball.h"

#if defined(WIN32) || defined(__CYGWIN32__)
#define WIN32_CURSOR_TWEAKS
#elif (GLUT_API_VERSION >= 4 || GLUT_XLIB_IMPLEMENTATION >= 9)
#define X_CURSOR_TWEAKS
#endif

typedef enum {
	MOUSE_POINTER,
	MOUSE_YOKE,
	MOUSE_VIEW
} MouseMode;

extern MouseMode mouse_mode;

extern float lastGuiQuat[4];
extern float curGuiQuat[4];
extern float GuiQuat_mat[4][4];

extern void initMouseQuat( void );
extern void Quat0( void );

class puObject;
extern void reInit(puObject *cb);
