// hud.hxx -- hud defines and prototypes (initial draft)
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
// (Log is kept at end of this file)


#ifndef _HUD_HXX
#define _HUD_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_VALUES_H
#  include <values.h>  // for MAXINT
#endif

#include <fg_typedefs.h>
#include <fg_constants.h>
#include <Aircraft/aircraft.hxx>
#include <Flight/flight.hxx>
#include <Controls/controls.h>

#include <deque>        // STL double ended queue

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif

#ifndef WIN32
  typedef struct {
      int x, y;
  } POINT;
 
  typedef struct {
      int top, bottom, left, right;
  } RECT;
#endif

// View mode definitions

enum VIEW_MODES{ HUD_VIEW, PANEL_VIEW, CHASE_VIEW, TOWER_VIEW };

// DAY, NIGHT and brightness levels need to be visible where dialogs and
// controls can be used to set intensity and appropriate color. This will
// be moved.
// Hud general constants
#define DAY                1
#define NIGHT              2
#define BRT_BLACK          3
#define BRT_DARK           4
#define BRT_MEDIUM         5
#define BRT_LIGHT          6
#define SIZE_SMALL         7
#define SIZE_LARGE         8

// Label constants
#define SMALL              1
#define LARGE              2

#define BLINK              3
#define NOBLINK            4

enum fgLabelJust{ LEFT_JUST, CENTER_JUST, RIGHT_JUST } ;

// Ladder constants
#define NONE               1
#define UPPER_LEFT         2
#define UPPER_CENTER       3
#define UPPER_RIGHT        4
#define CENTER_RIGHT       5
#define LOWER_RIGHT        6
#define LOWER_CENTER       7
#define LOWER_LEFT         8
#define CENTER_LEFT        9
#define SOLID_LINES       10
#define DASHED_LINES      11
#define DASHED_NEG_LINES  12


#define HORIZON_FIXED	1
#define HORIZON_MOVING	2
#define LABEL_COUNTER	1
#define LABEL_WARNING	2

#define HUDS_AUTOTICKS           0x0001
#define HUDS_VERT                0x0002
#define HUDS_HORZ                0x0000
#define HUDS_TOP                 0x0004
#define HUDS_BOTTOM              0x0008
#define HUDS_LEFT     HUDS_TOP
#define HUDS_RIGHT    HUDS_BOTTOM
#define HUDS_BOTH     (HUDS_LEFT | HUDS_RIGHT)
#define HUDS_NOTICKS             0x0010
#define HUDS_ARITHTIC            0x0020
#define HUDS_DECITICS            0x0040
#define HUDS_NOTEXT              0x0080

// Ladder orientaion
// #define HUD_VERTICAL        1
// #define HUD_HORIZONTAL		2
// #define HUD_FREEFLOAT		3

// Ladder orientation modes
// #define HUD_LEFT    		1
// #define HUD_RIGHT         	2
// #define HUD_TOP           	1
// #define HUD_BOTTOM        	2
// #define HUD_V_LEFT    		1
// #define HUD_V_RIGHT         	2
// #define HUD_H_TOP           	1
// #define HUD_H_BOTTOM        	2


// Ladder sub-types
// #define HUD_LIM				1
// #define HUD_NOLIM			2
// #define HUD_CIRC			3

// #define HUD_INSTR_LADDER	1
// #define HUD_INSTR_CLADDER	2
// #define HUD_INSTR_HORIZON	3
// #define HUD_INSTR_LABEL		4

extern double get_throttleval ( void );
extern double get_aileronval  ( void );
extern double get_elevatorval ( void );
extern double get_elev_trimval( void );
extern double get_rudderval   ( void );
extern double get_speed       ( void );
extern double get_aoa         ( void );
extern double get_roll        ( void );
extern double get_pitch       ( void );
extern double get_heading     ( void );
extern double get_altitude    ( void );
extern double get_agl         ( void );
extern double get_sideslip    ( void );
extern double get_frame_rate  ( void );
extern double get_latitude    ( void );
extern double get_lat_min     ( void );
extern double get_longitude   ( void );
extern double get_long_min    ( void );
extern double get_fov         ( void );
extern double get_vfc_ratio   ( void );
extern double get_vfc_tris_drawn   ( void );
extern double get_climb_rate  ( void );

enum  hudinstype{ HUDno_instr,
              HUDscale,
              HUDlabel,
              HUDladder,
              HUDcirc_ladder,
              HUDhorizon,
              HUDguage,
              HUDdual_inst,
              HUDmoving_scale,
              HUDtbi
              };

typedef struct gltagRGBTRIPLE { // rgbt
    GLfloat Blue;
    GLfloat Green;
    GLfloat Red;
} glRGBTRIPLE;

class instr_item {  // An Abstract Base Class (ABC)
  private:
    static UINT        instances;     // More than 64K instruments? Nah!
    static int         brightness;
    static glRGBTRIPLE color;

    UINT               handle;
    RECT               scrn_pos;      // Framing - affects scale dimensions
                                    // and orientation. Vert vs Horz, etc.
    DBLFNPTR           load_value_fn;
    double             disp_factor;   // Multiply by to get numbers shown on scale.
    UINT               opts;
    bool               is_enabled;
    bool               broken;
    UINT               scr_span;      // Working values for draw;
    POINT              mid_span;      //

  public:
    instr_item( int            x,
                int            y,
                UINT           height,
                UINT           width,
                DBLFNPTR       data_source,
                double         data_scaling,
                UINT           options,
                bool           working      = true);

    instr_item( const instr_item & image );

    instr_item & operator = ( const instr_item & rhs );
    virtual ~instr_item ();

    int          get_brightness  ( void ) { return brightness;}
    RECT         get_location    ( void ) { return scrn_pos;  }
    bool         is_broken       ( void ) { return broken;    }
    bool         enabled         ( void ) { return is_enabled;}
    bool         data_available  ( void ) { return !!load_value_fn; }
    double       get_value       ( void ) { return load_value_fn(); }
    double       data_scaling    ( void ) { return disp_factor; }
    UINT         get_span        ( void ) { return scr_span;  }
    POINT        get_centroid    ( void ) { return mid_span;  }
    UINT         get_options     ( void ) { return opts;      }

    virtual void display_enable( bool working ) { is_enabled = !! working;}


    virtual void update( void );
    virtual void break_display ( bool bad );
    virtual void SetBrightness( int illumination_level ); // fgHUDSetBright...
    void         SetPosition  ( int x, int y, UINT width, UINT height );
    UINT    get_Handle( void );
    virtual void draw( void ) = 0;   // Required method in derived classes
};

typedef instr_item *HIptr;
extern deque< instr_item *> HUD_deque;

// instr_item           This class has no other purpose than to maintain
//                      a linked list of instrument and derived class
// object pointers.


class instr_label : public instr_item {
  private:
    const char *pformat;
    const char *pre_str;
    const char *post_str;
    fgLabelJust justify;
    int         fontSize;
    int         blink;

  public:
    instr_label( int          x,
                 int          y,
                 UINT         width,
                 UINT         height,
                 DBLFNPTR     data_source,
                 const char  *label_format,
                 const char  *pre_label_string  = 0,
                 const char  *post_label_string = 0,
                 double       scale_data        = 1.0,
                 UINT         options           = HUDS_TOP,
                 fgLabelJust  justification     = CENTER_JUST,
                 int          font_size         = SMALL,
                 int          blinking          = NOBLINK,
                 bool         working           = true);

    ~instr_label();

    instr_label( const instr_label & image);
    instr_label & operator = (const instr_label & rhs );
    virtual void draw( void );       // Required method in base class
};

typedef instr_label * pInstlabel;

//
// instr_scale           This class is an abstract base class for both moving
//                       scale and moving needle (fixed scale) indicators. It
// does not draw itself, but is not instanciable.
//

class instr_scale : public instr_item {
  private:
    double range_shown;   // Width Units.
    double Maximum_value; //                ceiling.
    double Minimum_value; // Representation floor.
    double scale_factor;  // factor => screen units/range values.
    UINT   Maj_div;       // major division marker units
    UINT   Min_div;       // minor division marker units
    UINT   Modulo;        // Roll over point
    int    signif_digits; // digits to show to the right.

  public:
    instr_scale( int          x,
                 int          y,
                 UINT         width,
                 UINT         height,
                 DBLFNPTR     load_fn,
                 UINT         options,
                 double       show_range,
                 double       max_value    = 100.0,
                 double       min_value    =   0.0,
                 double       disp_scaling =   1.0,
                 UINT         major_divs   =    10,
                 UINT         minor_divs   =     5,
                 UINT         rollover     =     0,
                 int          dp_showing   =     2,
                 bool         working      =  true);

    virtual ~instr_scale();
    instr_scale( const instr_scale & image);
    instr_scale & operator = (const instr_scale & rhs);

    virtual void draw   ( void ) {}; // No-op here. Defined in derived classes.
    UINT   div_min      ( void ) { return Min_div;}
    UINT   div_max      ( void ) { return Maj_div;}
    double min_val      ( void ) { return Minimum_value;}
    double max_val      ( void ) { return Maximum_value;}
    UINT   modulo       ( void ) { return Modulo; }
    double factor       ( void ) { return scale_factor;}
    double range_to_show( void ) { return range_shown;}
};

// hud_card_               This class displays the indicated quantity on
//                         a scale that moves past the pointer. It may be
// horizontal or vertical, read above(left) or below(right) of the base
// line.

class hud_card : public instr_scale {
  private:
    double val_span;
    double half_width_units;

  public:
    hud_card( int      x,
              int      y,
              UINT     width,
              UINT     height,
              DBLFNPTR load_fn,
              UINT     options,
              double   maxValue      = 100.0,
              double   minValue      =   0.0,
              double   disp_scaling  =   1.0,
              UINT     major_divs    =  10,
              UINT     minor_divs    =   5,
              UINT     modulator     = 100,
              int      dp_showing    =   2,
              double   value_span    = 100.0,
              bool     working       = true);

    ~hud_card();
    hud_card( const hud_card & image);
    hud_card & operator = (const hud_card & rhs );
//    virtual void display_enable( bool setting );
    virtual void draw( void );       // Required method in base class
};

typedef hud_card * pCardScale;

class guage_instr : public instr_scale {
  private:

  public:
    guage_instr( int       x,
                 int       y,
                 UINT      width,
                 UINT      height,
                 DBLFNPTR  load_fn,
                 UINT      options,
                 double    disp_scaling = 1.0,
                 double    maxValue     = 100,
                 double    minValue     =   0,
                 UINT      major_divs   =  50,
                 UINT      minor_divs   =   0,
                 int       dp_showing   =   2,
                 UINT      modulus      =   0,
                 bool      working      = true);

    ~guage_instr();
    guage_instr( const guage_instr & image);
    guage_instr & operator = (const guage_instr & rhs );
    virtual void draw( void );       // Required method in base class
};

typedef guage_instr * pGuageInst;
//
// dual_instr_item         This class was created to form the base class
//                         for both panel and HUD Turn Bank Indicators.

class dual_instr_item : public instr_item {
  private:
    DBLFNPTR alt_data_source;

  public:
    dual_instr_item ( int       x,
                      int       y,
                      UINT      width,
                      UINT      height,
                      DBLFNPTR  chn1_source,
                      DBLFNPTR  chn2_source,
                      bool      working     = true,
                      UINT      options  = HUDS_TOP);

    virtual ~dual_instr_item() {};
    dual_instr_item( const dual_instr_item & image);
    dual_instr_item & operator = (const dual_instr_item & rhs );

    double current_ch1( void ) { return alt_data_source();}
    double current_ch2( void ) { return get_value();}
    virtual void draw ( void ) { }
};

class fgTBI_instr : public dual_instr_item {
  private:
    UINT BankLimit;
    UINT SlewLimit;
    UINT scr_hole;

  public:
    fgTBI_instr( int       x,
                 int       y,
                 UINT      width,
                 UINT      height,
                 DBLFNPTR  chn1_source  = get_roll,
                 DBLFNPTR  chn2_source  = get_sideslip,
                 double    maxBankAngle = 45.0,
                 double    maxSlipAngle =  5.0,
                 UINT      gap_width    =  5.0,
                 bool      working      =  true);

    fgTBI_instr( const fgTBI_instr & image);
    fgTBI_instr & operator = (const fgTBI_instr & rhs );

    ~fgTBI_instr();

    UINT bank_limit( void ) { return BankLimit;}
    UINT slew_limit( void ) { return SlewLimit;}

    virtual void draw( void );       // Required method in base class
};

typedef fgTBI_instr * pTBI;

class HudLadder : public dual_instr_item {
  private:
    UINT   width_units;
    int    div_units;
    UINT   minor_div;
    UINT   label_pos;
    UINT   scr_hole;
    double vmax;
    double vmin;
    double factor;

  public:
    HudLadder( int       x,
               int       y,
               UINT      width,
               UINT      height,
               DBLFNPTR  ptch_source    = get_roll,
               DBLFNPTR  roll_source    = get_pitch,
               double    span_units     = 45.0,
               double    division_units = 10.0,
               double    minor_division =  0.0,
               UINT      screen_hole    =   70,
               UINT      lbl_pos        =    0,
               bool      working        = true );

    ~HudLadder();

    HudLadder( const HudLadder & image );
    HudLadder & operator = ( const HudLadder & rhs );
    virtual void draw( void );
};


//using namespace std;
//deque <instr_item>  * Hdeque_ptr;

extern void HUD_brightkey( bool incr_bright );
extern int  fgHUDInit( fgAIRCRAFT * /* current_aircraft */ );
extern int  fgHUDInit2( fgAIRCRAFT * /* current_aircraft */ );
extern void fgUpdateHUD( void );

extern void drawOneLine ( UINT x1, UINT y1, UINT x2, UINT y2);
extern void drawOneLine ( RECT &rect);
extern void textString  ( int x,
                          int y,
                          char *msg,
                          void *font = GLUT_BITMAP_8_BY_13);
extern void strokeString( int x,
                          int y,
                          char *msg,
                          void *font = GLUT_STROKE_ROMAN,
                          float theta = 0);
/*
bool AddHUDInstrument( instr_item *pBlackBox );
void DrawHUD ( void );
bool DamageInstrument( INSTR_HANDLE unit );
bool RepairInstrument( INSTR_HANDLE unit );


void fgUpdateHUD ( Hptr hud );
void fgUpdateHUD2( Hptr hud ); // Future use?
void fgHUDSetTimeMode( Hptr hud, int time_of_day );
*/

#endif // _HUD_H

// $Log$
// Revision 1.15  1998/10/16 23:27:27  curt
// C++-ifying.
//
// Revision 1.14  1998/09/29 14:56:33  curt
// c++-ified comments.
//
// Revision 1.13  1998/09/29 02:01:09  curt
// Added a "rate of climb" indicator.
//
// Revision 1.12  1998/08/24 20:05:17  curt
// Added a second minimalistic HUD.
// Added code to display the number of triangles rendered.
//
// Revision 1.11  1998/07/24 21:36:55  curt
// Ran dos2unix to get rid of extraneous ^M's.  Tweaked parameter in
// ImageGetRawData() to match usage.
//
// Revision 1.10  1998/07/13 21:28:02  curt
// Converted the aoa scale to a radio altimeter.
//
// Revision 1.9  1998/07/13 21:00:48  curt
// Integrated Charlies latest HUD updates.
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.8  1998/07/03 13:16:29  curt
// Added Charlie Hotchkiss's HUD updates and improvementes.
//
// Revision 1.6  1998/06/03 00:43:28  curt
// No .h when including stl stuff.
//
// Revision 1.5  1998/05/17 16:58:13  curt
// Added a View Frustum Culling ratio display to the hud.
//
// Revision 1.4  1998/05/16 13:04:15  curt
// New updates from Charlie Hotchkiss.
//
// Revision 1.3  1998/05/13 18:27:55  curt
// Added an fov to hud display.
//
// Revision 1.2  1998/05/11 18:13:12  curt
// Complete C++ rewrite of all cockpit code by Charlie Hotchkiss.
//
// Revision 1.15  1998/02/23 19:07:57  curt
// Incorporated Durk's Astro/ tweaks.  Includes unifying the sun position
// calculation code between sun display, and other FG sections that use this
// for things like lighting.
//
// Revision 1.14  1998/02/21 14:53:14  curt
// Added Charlie's HUD changes.
//
// Revision 1.13  1998/02/20 00:16:22  curt
// Thursday's tweaks.
//
// Revision 1.12  1998/02/19 13:05:52  curt
// Incorporated some HUD tweaks from Michelle America.
// Tweaked the sky's sunset/rise colors.
// Other misc. tweaks.
//
// Revision 1.11  1998/02/16 13:38:42  curt
// Integrated changes from Charlie Hotchkiss.
//
// Revision 1.11  1998/02/16 13:38:42  curt
// Integrated changes from Charlie Hotchkiss.
//
// Revision 1.10  1998/02/12 21:59:42  curt
// Incorporated code changes contributed by Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.8  1998/02/07 15:29:35  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.7  1998/02/03 23:20:15  curt
// Lots of little tweaks to fix various consistency problems discovered by
// Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
// passed arguments along to the real printf().  Also incorporated HUD changes
// by Michele America.
//
// Revision 1.6  1998/01/22 02:59:30  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.5  1998/01/19 19:27:01  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.4  1998/01/19 18:40:21  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.3  1997/12/30 16:36:41  curt
// Merged in Durk's changes ...
//
// Revision 1.2  1997/12/10 22:37:40  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.1  1997/08/29 18:03:22  curt
// Initial revision.
//
