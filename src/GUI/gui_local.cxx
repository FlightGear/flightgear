#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>        // needed before pu.h
#include <plib/pu.h>        // plib include

#include <FDM/flight.hxx>

#include <Main/globals.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx>
#include <Scenery/tilemgr.hxx>
#include <Time/light.hxx>

#include "gui.h"
#include "trackball.h"

// from main.cxx
extern void fgReshape(int, int);

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
    // BusyCursor(0);
    Quat0();

    static const SGPropertyNode *master_freeze
	= fgGetNode("/sim/freeze/master");

    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", true);
    }

    cur_fdm_state->unbind();

    // in case user has changed window size as
    // restoreInitialState() overwrites these
    int xsize = fgGetInt("/sim/startup/xsize");
    int ysize = fgGetInt("/sim/startup/ysize");

    build_rotmatrix(GuiQuat_mat, curGuiQuat);

    globals->restoreInitialState();

    // Unsuccessful KLUDGE to fix the 'every other time'
    // problem when doing a 'reset' after a 'goto airport'
	
    // string AptId( fgGetString("/sim/startup/airport-id") );
    // if( AptId.c_str() != "\0" )
    //      fgSetPosFromAirportID( AptId );
	
    SGTime *t = globals->get_time_params();
    delete t;
    t = fgInitTime();
    globals->set_time_params( t );

    fgReInitSubsystems();

    // reduntant(fgReInitSubsystems) ?
    global_tile_mgr.update( fgGetDouble("/position/longitude-deg"),
                            fgGetDouble("/position/latitude-deg") );
    
    cur_light_params.Update();

    fgReshape( xsize, ysize );

    // BusyCursor(1);
    
    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", false);
    }
}

