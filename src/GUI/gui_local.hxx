#include "trackball.h"

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
