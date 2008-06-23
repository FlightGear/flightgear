#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <plib/pu.h>        // plib include

#include <FDM/flight.hxx>

#include <Main/globals.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx>
#include <Main/renderer.hxx>
#include <Scenery/tilemgr.hxx>
#include <Time/light.hxx>

#include "gui.h"
#include "trackball.h"

// FOR MOUSE VIEW MODE
// stashed trackball(_quat0, 0.0, 0.0, 0.0, 0.0);
static float _quat0[4];

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


void reInit(void)
{
    Quat0();

    static const SGPropertyNode *master_freeze
	= fgGetNode("/sim/freeze/master");

    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", true);
    }

    fgSetBool("/sim/signals/reinit", true);
    cur_fdm_state->unbind();

    // in case user has changed window size as
    // restoreInitialState() overwrites these
    int xsize = fgGetInt("/sim/startup/xsize");
    int ysize = fgGetInt("/sim/startup/ysize");

    build_rotmatrix(GuiQuat_mat, curGuiQuat);

    globals->restoreInitialState();

    // update our position based on current presets
    fgInitPosition();

    // We don't know how to resize the window, so keep the last values 
    //  for xsize and ysize, and don't use the one set initially
    fgSetInt("/sim/startup/xsize",xsize);
    fgSetInt("/sim/startup/ysize",ysize);

    SGTime *t = globals->get_time_params();
    delete t;
    t = fgInitTime();
    globals->set_time_params( t );

    fgReInitSubsystems();

    globals->get_tile_mgr()->update( fgGetDouble("/environment/visibility-m") );
    globals->get_renderer()->resize( xsize, ysize );
    fgSetBool("/sim/signals/reinit", false);

    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", false);
    }
}

