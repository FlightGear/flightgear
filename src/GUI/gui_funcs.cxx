/**************************************************************************
 * gui_funcs.cxx
 *
 * Based on gui.cxx and renamed on 2002/08/13 by Erik Hofman.
 *
 * Written 1998 by Durk Talsma, started Juni, 1998.  For the flight gear
 * project.
 *
 * Additional mouse supported added by David Megginson, 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include SG_GL_H

#if defined(FX) && defined(XMESA)
#  include <GL/xmesa.h>
#endif

#include STL_FSTREAM
#include STL_STRING

#include <stdlib.h>
#include <string.h>

// for help call back
#ifdef WIN32
# include <shellapi.h>
# ifdef __CYGWIN__
#  include <sys/cygwin.h>
# endif
#endif

#include <plib/ssg.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/screen/screen-dump.hxx>

#include <Include/general.hxx>
#include <Aircraft/aircraft.hxx>
#include <Airports/simple.hxx>
#include <Autopilot/auto_gui.hxx>
#include <Cockpit/panel.hxx>
#include <Controls/controls.hxx>
#include <FDM/flight.hxx>
#include <Main/main.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_io.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/renderer.hxx>
#include <Main/viewmgr.hxx>
#include <GUI/new_gui.hxx>

#if defined( WIN32 ) && !defined( __CYGWIN__ ) && !defined(__MINGW32__)
#  include <simgear/screen/win32-printer.h>
#  include <simgear/screen/GlBitmaps.h>
#endif

#include "gui.h"
#include "gui_local.hxx"
#include "preset_dlg.hxx"
#include "prop_picker.hxx"
#include "sgVec3Slider.hxx"

SG_USING_STD(string);
SG_USING_STD(cout);

extern void fgHUDalphaAdjust( puObject * );

// from cockpit.cxx
extern void fgLatLonFormatToggle( puObject *);

#if defined( TR_HIRES_SNAP)
#include <simgear/screen/tr.h>
extern void fgUpdateHUD( GLfloat x_start, GLfloat y_start,
                         GLfloat x_end, GLfloat y_end );
#endif


// TODO: remove after the last hardcoded dialog has died
char *gui_msg_OK     = "OK";
char *gui_msg_NO     = "NO";
char *gui_msg_YES    = "YES";
char *gui_msg_CANCEL = "CANCEL";
char *gui_msg_RESET  = "RESET";


const __fg_gui_fn_t __fg_gui_fn[] = {

        // File
        {"reInit", reInit},
#ifdef TR_HIRES_SNAP
        {"dumpHiResSnapShot", fgHiResDumpWrapper},
#endif
        {"dumpSnapShot", fgDumpSnapShotWrapper},
#if defined( WIN32 ) && !defined( __CYGWIN__) && !defined(__MINGW32__)
        {"printScreen", printScreen},
#endif

        //View
        {"guiTogglePanel", guiTogglePanel},
        {"fgHUDalphaAdjust", fgHUDalphaAdjust},
        {"prop_pickerView", prop_pickerView},

        // Environment
        {"fgPresetAirport", fgPresetAirport},
        {"fgPresetRunway", fgPresetRunway},
        {"fgPresetOffsetDistance", fgPresetOffsetDistance},
        {"fgPresetAltitude", fgPresetAltitude},
        {"fgPresetGlideslope", fgPresetGlideslope},
        {"fgPresetAirspeed", fgPresetAirspeed},
        {"fgPresetCommit", fgPresetCommit},

        // Autopilot
        {"NewAltitude", NewAltitude},
	{"NewHeading", NewHeading},
        {"AddWayPoint", AddWayPoint},
        {"PopWayPoint", PopWayPoint},
        {"ClearRoute", ClearRoute},
        {"fgLatLonFormatToggle", fgLatLonFormatToggle},

        // Help
        {"helpCb", helpCb},

        // Structure termination
        {"", NULL}
};


/* ================ General Purpose Functions ================ */

// General Purpose Message Box
void mkDialog (const char *txt)
{
    NewGUI *gui = (NewGUI *)globals->get_subsystem("gui");
    SGPropertyNode_ptr dlg = gui->getDialog("message");
    if (!dlg)
        return;

    dlg->setStringValue("text/label", txt);
    dlg->setStringValue("button/legend", "OK");
    gui->showDialog("message");
}

// Message Box to report an error.
void guiErrorMessage (const char *txt)
{
    SG_LOG(SG_GENERAL, SG_ALERT, txt);
    mkDialog(txt);
}

// Message Box to report a throwable (usually an exception).
void guiErrorMessage (const char *txt, const sg_throwable &throwable)
{
    string msg = txt;
    msg += '\n';
    msg += throwable.getFormattedMessage();
    if (!throwable.getOrigin().empty()) {
      msg += "\n (reported by ";
      msg += throwable.getOrigin();
      msg += ')';
    }
    SG_LOG(SG_GENERAL, SG_ALERT, msg);
    mkDialog(msg.c_str());
}



/* -----------------------------------------------------------------------
the Gui callback functions 
____________________________________________________________________*/


// Hier Neu :-) This is my newly added code
// Added by David Findlay <nedz@bigpond.com>
// on Sunday 3rd of December


// This is the accessor function
void guiTogglePanel(puObject *cb)
{
  if (fgGetBool("/sim/panel/visibility"))
    fgSetBool("/sim/panel/visibility", false);
  else
    fgSetBool("/sim/panel/visibility", true);

  globals->get_renderer()->resize(fgGetInt("/sim/startup/xsize"),
                                  fgGetInt("/sim/startup/ysize"));
}

void helpCb (puObject *)
{
    string command;
	
#if defined(FX) && !defined(WIN32)
#  if defined(XMESA_FX_FULLSCREEN) && defined(XMESA_FX_WINDOW)
    if ( globals->get_fullscreen() ) {
        globals->set_fullscreen(false);
        XMesaSetFXmode( XMESA_FX_WINDOW );
    }
#  endif
#endif
	
    SGPath path( globals->get_fg_root() );
    path.append( "Docs/index.html" );
	
#if !defined(WIN32)

    string help_app = fgGetString("/sim/startup/browser-app");

    if ( system("xwininfo -name Netscape > /dev/null 2>&1") == 0 ) {
        command = help_app + " -remote \"openURL(" + path.str() + ")\"";
    } else {
        command = help_app + " " + path.str();
    }
    command += " &";
    system( command.c_str() );

#else // WIN32

    // Look for favorite browser
    char Dummy[1024], ExecName[1024], browserParameter[1024];
    char win32_name[1024];
# ifdef __CYGWIN__
    cygwin32_conv_to_full_win32_path(path.c_str(),win32_name);
# else
    strcpy(win32_name,path.c_str());
# endif
    Dummy[0] = 0;
    FindExecutable(win32_name, Dummy, ExecName);
    sprintf(browserParameter, "file:///%s", win32_name);
    ShellExecute ( NULL, "open", ExecName, browserParameter, Dummy,
                   SW_SHOWNORMAL ) ;

#endif
	
    mkDialog ("Help started in your web browser window.");
}

#if defined( TR_HIRES_SNAP)
void fgHiResDump()
{
    FILE *f;
    string message;
    bool show_pu_cursor = false;
    char *filename = new char [24];
    static int count = 1;

    static const SGPropertyNode *master_freeze
        = fgGetNode("/sim/freeze/master");

    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", true);
    }

    TurnCursorOff();
    if ( !puCursorIsHidden() ) {
	show_pu_cursor = true;
	puHideCursor();
    }

    FGRenderer *renderer = globals->get_renderer();
    renderer->init();
    renderer->resize( fgGetInt("/sim/startup/xsize"),
                      fgGetInt("/sim/startup/ysize") );

    // we need two render frames here to clear the menu and cursor
    // ... not sure why but doing an extra fgRenderFrame() shouldn't
    // hurt anything
    renderer->update( true );
    renderer->update( true );

    // This ImageSize stuff is a temporary hack
    // should probably use 128x128 tile size and
    // support any image size

    // This should be a requester to get multiplier from user
    int multiplier = fgGetInt("/sim/startup/hires-multiplier", 3);
    int width = fgGetInt("/sim/startup/xsize");
    int height = fgGetInt("/sim/startup/ysize");
	
    /* allocate buffer large enough to store one tile */
    GLubyte *tile = (GLubyte *)malloc(width * height * 3 * sizeof(GLubyte));
    if (!tile) {
        printf("Malloc of tile buffer failed!\n");
        return;
    }

    int imageWidth  = multiplier*width;
    int imageHeight = multiplier*height;

    /* allocate buffer to hold a row of tiles */
    GLubyte *buffer
        = (GLubyte *)malloc(imageWidth * height * 3 * sizeof(GLubyte));
    if (!buffer) {
        free(tile);
        printf("Malloc of tile row buffer failed!\n");
        return;
    }
    TRcontext *tr = trNew();
    trTileSize(tr, width, height, 0);
    trTileBuffer(tr, GL_RGB, GL_UNSIGNED_BYTE, tile);
    trImageSize(tr, imageWidth, imageHeight);
    trRowOrder(tr, TR_TOP_TO_BOTTOM);
    sgFrustum *frustum = ssgGetFrustum();
    trFrustum(tr,
              frustum->getLeft(), frustum->getRight(),
              frustum->getBot(),  frustum->getTop(), 
              frustum->getNear(), frustum->getFar());
	
    /* Prepare ppm output file */
    while (count < 1000) {
        snprintf(filename, 24, "fgfs-screen-%03d.ppm", count++);
        if ( (f = fopen(filename, "r")) == NULL )
            break;
        fclose(f);
    }
    f = fopen(filename, "wb");
    if (!f) {
        printf("Couldn't open image file: %s\n", filename);
        free(buffer);
        free(tile);
        return;
    }
    fprintf(f,"P6\n");
    fprintf(f,"# ppm-file created by %s\n", "trdemo2");
    fprintf(f,"%i %i\n", imageWidth, imageHeight);
    fprintf(f,"255\n");

    /* just to be safe... */
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    /* Because the HUD and Panel change the ViewPort we will
     * need to handle some lowlevel stuff ourselves */
    int ncols = trGet(tr, TR_COLUMNS);
    int nrows = trGet(tr, TR_ROWS);

    bool do_hud = fgGetBool("/sim/hud/visibility");
    GLfloat hud_col_step = 640.0 / ncols;
    GLfloat hud_row_step = 480.0 / nrows;
	
    bool do_panel = fgPanelVisible();
    GLfloat panel_col_step = globals->get_current_panel()->getWidth() / ncols;
    GLfloat panel_row_step = globals->get_current_panel()->getHeight() / nrows;
	
    /* Draw tiles */
    int more = 1;
    while (more) {
        trBeginTile(tr);
        int curColumn = trGet(tr, TR_CURRENT_COLUMN);
        int curRow =  trGet(tr, TR_CURRENT_ROW);
        renderer->update( false );
        if ( do_hud )
            fgUpdateHUD( curColumn*hud_col_step,      curRow*hud_row_step,
                         (curColumn+1)*hud_col_step, (curRow+1)*hud_row_step );
        if (do_panel)
            globals->get_current_panel()->update(
                                   curColumn*panel_col_step, panel_col_step,
                                   curRow*panel_row_step,    panel_row_step );
        more = trEndTile(tr);

        /* save tile into tile row buffer*/
        int curTileWidth = trGet(tr, TR_CURRENT_TILE_WIDTH);
        int bytesPerImageRow = imageWidth * 3*sizeof(GLubyte);
        int bytesPerTileRow = (width) * 3*sizeof(GLubyte);
        int xOffset = curColumn * bytesPerTileRow;
        int bytesPerCurrentTileRow = (curTileWidth) * 3*sizeof(GLubyte);
        int i;
        for (i=0;i<height;i++) {
            memcpy(buffer + i*bytesPerImageRow + xOffset, /* Dest */
                   tile + i*bytesPerTileRow,              /* Src */
                   bytesPerCurrentTileRow);               /* Byte count*/
        }

        if (curColumn == trGet(tr, TR_COLUMNS)-1) {
            /* write this buffered row of tiles to the file */
            int curTileHeight = trGet(tr, TR_CURRENT_TILE_HEIGHT);
            int bytesPerImageRow = imageWidth * 3*sizeof(GLubyte);
            int i;
            for (i=0;i<curTileHeight;i++) {
                /* Remember, OpenGL images are bottom to top.  Have to reverse. */
                GLubyte *rowPtr = buffer + (curTileHeight-1-i) * bytesPerImageRow;
                fwrite(rowPtr, 1, imageWidth*3, f);
            }
        }

    }

    renderer->resize( width, height );

    trDelete(tr);

    fclose(f);

    message = "Snapshot saved to \"";
    message += filename;
    message += '".';
    mkDialog (message.c_str());

    free(tile);
    free(buffer);

    delete [] filename;

    if ( show_pu_cursor ) {
        puShowCursor();
    }

    TurnCursorOn();

    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", false);
    }
}
#endif // #if defined( TR_HIRES_SNAP)


#if defined( WIN32 ) && !defined( __CYGWIN__) && !defined(__MINGW32__)

void rotateView( double roll, double pitch, double yaw )
{
	// rotate view
}

GlBitmap *b1 = NULL;
GLubyte *hiResScreenCapture( int multiplier )
{
    float oldfov = fgGetDouble("/sim/current-view/field-of-view");
    float fov = oldfov / multiplier;
    FGViewer *v = globals->get_current_view();
    fgSetDouble("/sim/current-view/field-of-view", fov);
    globals->get_renderer()->init();
    int cur_width = fgGetInt("/sim/startup/xsize");
    int cur_height = fgGetInt("/sim/startup/ysize");
    delete( b1 );
    // New empty (mostly) bitmap
    b1 = new GlBitmap( GL_RGB, 1, 1, (unsigned char *)"123" );
    int x,y;
    for ( y = 0; y < multiplier; y++ ) {
	for ( x = 0; x < multiplier; x++ ) {
	    globals->get_renderer()->resize( cur_width, cur_height );
	    // pan to tile
	    rotateView( 0, (y*fov)-((multiplier-1)*fov/2), (x*fov)-((multiplier-1)*fov/2) );
	    globals->get_renderer()->update( false );
	    // restore view
	    GlBitmap b2;
	    b1->copyBitmap( &b2, cur_width*x, cur_height*y );
	}
    }
    fgSetDouble("/sim/current-view/field-of-view", oldfov);
    return b1->getBitmap();
}
#endif

#if defined( WIN32 ) && !defined( __CYGWIN__) && !defined(__MINGW32__)
// win32 print screen function
void printScreen ( puObject *obj ) {
    bool show_pu_cursor = false;
    TurnCursorOff();
    if ( !puCursorIsHidden() ) {
	show_pu_cursor = true;
	puHideCursor();
    }
    // BusyCursor( 0 );

    CGlPrinter p( CGlPrinter::PRINT_BITMAP );
    int cur_width = fgGetInt("/sim/startup/xsize");
    int cur_height = fgGetInt("/sim/startup/ysize");
    p.Begin( "FlightGear", cur_width*3, cur_height*3 );
    p.End( hiResScreenCapture(3) );

    // BusyCursor(1);
    if ( show_pu_cursor ) {
	puShowCursor();
    }
    TurnCursorOn();
}
#endif // #ifdef WIN32


void fgDumpSnapShotWrapper ( puObject *obj ) {
    fgDumpSnapShot();
}


void fgHiResDumpWrapper ( puObject *obj ) {
    fgHiResDump();
}


// do a screen snap shot
void fgDumpSnapShot () {
    bool show_pu_cursor = false;
    char *filename = new char [24];
    string message;
    static int count = 1;

    static const SGPropertyNode *master_freeze
	= fgGetNode("/sim/freeze/master");

    bool freeze = master_freeze->getBoolValue();
    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", true);
    }

    TurnCursorOff();
    if ( !puCursorIsHidden() ) {
	show_pu_cursor = true;
	puHideCursor();
    }

    FGRenderer *renderer = globals->get_renderer();
    renderer->init();
    renderer->resize( fgGetInt("/sim/startup/xsize"),
                        fgGetInt("/sim/startup/ysize") );

    // we need two render frames here to clear the menu and cursor
    // ... not sure why but doing an extra fgRenderFrame() shouldn't
    // hurt anything
    renderer->update( true );
    renderer->update( true );

    while (count < 1000) {
        FILE *fp;
        snprintf(filename, 24, "fgfs-screen-%03d.ppm", count++);
        if ( (fp = fopen(filename, "r")) == NULL )
            break;
        fclose(fp);
    }

    if ( sg_glDumpWindow( filename,
			  fgGetInt("/sim/startup/xsize"), 
			  fgGetInt("/sim/startup/ysize")) ) {
	message = "Snapshot saved to \"";
	message += filename;
	message += "\".";
    } else {
        message = "Failed to save to \"";
	message += filename;
	message += "\".";
    }

    mkDialog (message.c_str());

    delete [] filename;

    if ( show_pu_cursor ) {
	puShowCursor();
    }

    TurnCursorOn();

    if ( !freeze ) {
        fgSetBool("/sim/freeze/master", false);
    }
}

