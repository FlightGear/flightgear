// cockpit.cxx -- routines to draw a cockpit (initial draft)
//
// Written by Michele America, started September 1997.
//
// Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H          
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <simgear/xgl/xgl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/fg_random.h>
#include <simgear/math/polar3d.hxx>

#include <Aircraft/aircraft.hxx>
#include <Include/general.hxx>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>
#include <Time/fg_time.hxx>
#include <GUI/gui.h>

#include "cockpit.hxx"


// This is a structure that contains all data related to
// cockpit/panel/hud system

static pCockpit ac_cockpit;
fntRenderer *HUDtext = 0;
float  HUD_TextSize = 0;
int HUD_style = 0;

float HUD_matrix[16];
// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

float get_latitude( void )
{
    double lat;

    lat = current_aircraft.fdm_state->get_Latitude() * RAD_TO_DEG;

    float flat = lat;
    return(flat);

}

float get_lat_min( void )
{
    double      a, d;

    a = current_aircraft.fdm_state->get_Latitude() * RAD_TO_DEG;    
    if (a < 0.0) {
        a = -a;
    }
    d = (double) ( (int) a);
    float lat_min = (a - d) * 60.0;
    return(lat_min );
}


float get_longitude( void )
{
    double lon;

    lon = current_aircraft.fdm_state->get_Longitude() * RAD_TO_DEG;

    float flon = lon;
    return(flon);
}


char*
get_formated_gmt_time( void )
{
    static char buf[32];
    FGTime *t = FGTime::cur_time_params;
    const struct tm *p = t->getGmt();
    sprintf( buf, "%d/%d/%4d %d:%02d:%02d", 
         p->tm_mon+1, p->tm_mday, 1900 + p->tm_year,
         p->tm_hour, p->tm_min, p->tm_sec);
    return buf;
}


float get_long_min( void )
{
    double  a, d;
    a = current_aircraft.fdm_state->get_Longitude() * RAD_TO_DEG;   
    if (a < 0.0) {
        a = -a;
    }
    d = (double) ( (int) a);
    float lon_min = (a - d) * 60.0; 
    return(lon_min);
}

float get_throttleval( void )
{
    float throttle = controls.get_throttle( 0 );
    return (throttle);     // Hack limiting to one engine
}

float get_aileronval( void )
{
    float aileronval = controls.get_aileron();
    return (aileronval);
}

float get_elevatorval( void )
{
    float elevator_val = (float)controls.get_elevator();
    return elevator_val;
}

float get_elev_trimval( void )
{
    float elevatorval = controls.get_elevator_trim();
    return (elevatorval);
}

float get_rudderval( void )
{
    float rudderval = controls.get_rudder();
    return (rudderval);
}

float get_speed( void )
{
    // Make an explicit function call.
    float speed = current_aircraft.fdm_state->get_V_calibrated_kts()
	* current_options.get_speed_up();
    return( speed );
}

float get_mach(void)
{
	float mach=current_aircraft.fdm_state->get_Mach_number();
	return mach;
}	

float get_aoa( void )
{
    float aoa = current_aircraft.fdm_state->get_Alpha() * RAD_TO_DEG;
    return( aoa );
}

float get_roll( void )
{
    float roll = current_aircraft.fdm_state->get_Phi();
    return( roll );
}

float get_pitch( void )
{
    float pitch = current_aircraft.fdm_state->get_Theta();
    return( pitch );
}

float get_heading( void )
{
    float heading = (current_aircraft.fdm_state->get_Psi() * RAD_TO_DEG);
    return( heading );
}

float get_altitude( void )
{
//  FGState *f;
    // double rough_elev;

//  current_aircraft.fdm_state
    // rough_elev = mesh_altitude(f->get_Longitude() * RAD_TO_ARCSEC,
    //                         f->get_Latitude()  * RAD_TO_ARCSEC);
    float altitude;

    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
        altitude = current_aircraft.fdm_state->get_Altitude();
    } else {
        altitude = (current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER);
    }
    return altitude;
}

float get_agl( void )
{
    float agl;

    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
        agl = (current_aircraft.fdm_state->get_Altitude()
               - scenery.cur_elev * METER_TO_FEET);
    } else {
        agl = (current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER
               - scenery.cur_elev);
    }
    return agl;
}

float get_sideslip( void )
{
    float sideslip = current_aircraft.fdm_state->get_Beta();
    return( sideslip );
}

float get_frame_rate( void )
{
    float frame_rate = general.get_frame_rate();
    return (frame_rate); 
}

float get_fov( void )
{
    float fov = current_options.get_fov(); 
    return (fov);
}

float get_vfc_ratio( void )
{
    float vfc = current_view.get_vfc_ratio();
    return (vfc);
}

float get_vfc_tris_drawn   ( void )
{
    float rendered = current_view.get_tris_rendered();
    return (rendered);
}

float get_vfc_tris_culled   ( void )
{
    float culled = current_view.get_tris_culled();
    return (culled);
}

float get_climb_rate( void )
{
    float climb_rate;
    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
        climb_rate = current_aircraft.fdm_state->get_Climb_Rate() * 60.0;
    } else {
        climb_rate = current_aircraft.fdm_state->get_Climb_Rate() * FEET_TO_METER * 60.0;
    }
    return (climb_rate);
}


float get_view_direction( void )
{
    double view;
 
    view = FG_2PI - current_view.get_view_offset();
    view = ( current_aircraft.fdm_state->get_Psi() + view) * RAD_TO_DEG;
    
    if(view > 360.)
        view -= 360.;
    else if(view<0.)
        view += 360.;
    
    float fview = view;
    return( fview );
}

#ifdef NOT_USED
/****************************************************************************/
/* Convert degrees to dd mm'ss.s" (DMS-Format)                              */
/****************************************************************************/
char *dmshh_format(double degrees)
{
    static char buf[16];    
    int deg_part;
    int min_part;
    double sec_part;
    
    if (degrees < 0)
      degrees = -degrees;

    deg_part = degrees;
    min_part = 60.0 * (degrees - deg_part);
    sec_part = 3600.0 * (degrees - deg_part - min_part / 60.0);

    /* Round off hundredths */
    if (sec_part + 0.005 >= 60.0)
      sec_part -= 60.0, min_part += 1;
    if (min_part >= 60)
      min_part -= 60, deg_part += 1;

    sprintf(buf,"%02d*%02d %05.2f",deg_part,min_part,sec_part);

    return buf;
}
#endif // 0

/****************************************************************************/
/* Convert degrees to dd mm'ss.s'' (DMS-Format)                              */
/****************************************************************************/
static char *toDMS(float a)
{
  int        neg = 0;
  float       d, m, s;
  static char  dms[16];

  if (a < 0.0f) {
    a   = -a;
    neg = 1;
  }
  d = (float) ((int) a); 
  a = (a - d) * 60.0f;
  m = (float) ((int) a);
  s = (a - m) * 60.0f;
  
  if (s > 59.5f) {
    s  = 0.0f;
    m += 1.0f;
  }
  if (m > 59.5f) {
    m  = 0.0f;
    d += 1.0f;
  }
  if (neg)
    d = -d;
  
  sprintf(dms, "%.0f*%02.0f %04.1f", d, m, s);
  return dms;
}


/****************************************************************************/
/* Convert degrees to dd mm.mmm' (DMM-Format)                               */
/****************************************************************************/
static char *toDM(float a)
{
  int        neg = 0;
  float       d, m;
  static char  dm[16];
  
  if (a < 0.0f) {
    a = -a;
    neg = 1;
  }

  d = (float) ( (int) a);
  m = (a - d) * 60.0f;
  
  if (m > 59.5f) {
    m  = 0.0f;
    d += 1.0f;
  }
  if (neg) d = -d;
  
  sprintf(dm, "%.0f*%06.3f", d, m);
  return dm;
}

// Have to set the LatLon display type
//static char *(*fgLatLonFormat)(float) = toDM;
static char *(*fgLatLonFormat)(float);

char *coord_format_lat(float latitude)
{
    static char buf[16];

    sprintf(buf,"%s%c",
//      dmshh_format(latitude),
//      toDMS(latitude),
//      toDM(latitude),
        fgLatLonFormat(latitude),           
        latitude > 0 ? 'N' : 'S');
    return buf;
}

char *coord_format_lon(float longitude)
{
    static char buf[80];

    sprintf(buf,"%s%c",
//      dmshh_format(longitude),
//      toDMS(longitude),
//      toDM(longitude),
        fgLatLonFormat(longitude),
        longitude > 0 ? 'E' : 'W');
    return buf;
}

void fgLatLonFormatToggle( puObject *)
{
    static int toggle = 0;

    if ( toggle ) 
        fgLatLonFormat = toDM;
    else
        fgLatLonFormat = toDMS;
    
    toggle = ~toggle;
}

#ifdef NOT_USED
char *coord_format_latlon(double latitude, double longitude)
{
    static char buf[1024];

    sprintf(buf,"%s%c %s%c",
        dmshh_format(latitude),
        latitude > 0 ? 'N' : 'S',
        dmshh_format(longitude),
        longitude > 0 ? 'E' : 'W');
    return buf;
}
#endif


bool fgCockpitInit( fgAIRCRAFT *cur_aircraft )
{
    FG_LOG( FG_COCKPIT, FG_INFO, "Initializing cockpit subsystem" );

    //  cockpit->code = 1;  /* It will be aircraft dependent */
    //  cockpit->status = 0;

    // If aircraft has HUD specified we will get the specs from its def
    // file. For now we will depend upon hard coding in hud?
    
    // We must insure that the existing instrument link is purged.
    // This is done by deleting the links in the list.
    
    // HI_Head is now a null pointer so we can generate a new list from the
    // current aircraft.

    fgHUDInit( cur_aircraft );
    ac_cockpit = new fg_Cockpit();
    
    if ( current_options.get_panel_status() ) {
        new FGPanel;
    }

    // Have to set the LatLon display type
    fgLatLonFormat = toDM;
    
    FG_LOG( FG_COCKPIT, FG_INFO,
        "  Code " << ac_cockpit->code() << " Status " 
        << ac_cockpit->status() );

//  HUD_TextSize = (current_options.get_xsize() > 1000) ? 10 : 8;
    HUD_TextSize = 10;
    HUDtext = new fntRenderer();
    HUDtext -> setFont      ( guiFntHandle ) ;
    HUDtext -> setPointSize ( HUD_TextSize ) ;
    HUD_TextList.setFont( HUDtext );

    return true;
}


void fgCockpitUpdate( void ) {

    int iwidth   = current_view.get_winWidth();
    int iheight  = current_view.get_winHeight();
    float width  = iwidth;
    float height = iheight;
    
    FG_LOG( FG_COCKPIT, FG_DEBUG,
        "Cockpit: code " << ac_cockpit->code() << " status " 
        << ac_cockpit->status() );

    if ( current_options.get_hud_status() ) {
        // This will check the global hud linked list pointer.
        // If these is anything to draw it will.
        fgUpdateHUD();
    }
#define DISPLAY_COUNTER
#ifdef DISPLAY_COUNTER
    else
    {
        char buf[64];
        float fps    =       get_frame_rate();
//      float tris   = fps * get_vfc_tris_drawn();
//      float culled = fps * get_vfc_tris_culled();
//      sprintf(buf,"%-4.1f  %7.0f  %7.0f", fps, tris, culled);
        sprintf(buf,"%-5.1f", fps);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, width, 0, height);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        
        glColor3f (0.9, 0.4, 0.2);

        guiFnt.drawString( buf,
                           width/2 - guiFnt.getStringWidth(buf)/2,
                           10 );
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
#endif // #ifdef DISPLAY_COUNTER
    
    xglViewport( 0, 0, iwidth, iheight );

    current_panel.update();
}
