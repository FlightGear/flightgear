#include <GL/glut.h>		// needed before pu.h
#include <plib/pu.h>		// plib include


#include <Main/fg_init.hxx>

#include "gui.h"
#include "trackball.h"

// FOR MOUSE VIEW MODE
// stashed trackball(_quat0, 0.0, 0.0, 0.0, 0.0);
static float _quat0[4];

float lastGuiQuat[4];
float curGuiQuat[4];

// To apply our mouse rotation quat to VIEW
// sgPreMultMat4( VIEW, GuiQuat_mat);
// This is here temporarily should be in views.hxx
float GuiQuat_mat[4][4];

void Quat0( void ) {
	curGuiQuat[0] = _quat0[0];
	curGuiQuat[1] = _quat0[1];
	curGuiQuat[2] = _quat0[2];
	curGuiQuat[3] = _quat0[3];
}

void initMouseQuat(void) {
	trackball(_quat0, 0.0, 0.0, 0.0, 0.0);  
	Quat0();
	build_rotmatrix(GuiQuat_mat, curGuiQuat);
}


void reInit(puObject *cb)
{
	BusyCursor(0);
	Quat0();
	build_rotmatrix(GuiQuat_mat, curGuiQuat);
	fgReInitSubsystems();
	BusyCursor(1);
}

