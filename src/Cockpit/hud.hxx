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

#include <vector>       // STL vector
#include <deque>        // STL double ended queue

#include <fg_typedefs.h>
#include <fg_constants.h>
#include <Aircraft/aircraft.hxx>
#include <FDM/flight.hxx>
#include <Controls/controls.hxx>
#include <GUI/gui.h>
#include <Math/mat3.h>

FG_USING_STD(deque);
FG_USING_STD(vector);

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


#define HORIZON_FIXED   1
#define HORIZON_MOVING  2
#define LABEL_COUNTER   1
#define LABEL_WARNING   2

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
// #define HUD_HORIZONTAL       2
// #define HUD_FREEFLOAT        3

// Ladder orientation modes
// #define HUD_LEFT         1
// #define HUD_RIGHT            2
// #define HUD_TOP              1
// #define HUD_BOTTOM           2
// #define HUD_V_LEFT           1
// #define HUD_V_RIGHT          2
// #define HUD_H_TOP            1
// #define HUD_H_BOTTOM         2


// Ladder sub-types
// #define HUD_LIM              1
// #define HUD_NOLIM            2
// #define HUD_CIRC         3

// #define HUD_INSTR_LADDER 1
// #define HUD_INSTR_CLADDER    2
// #define HUD_INSTR_HORIZON    3
// #define HUD_INSTR_LABEL      4

// in cockpit.cxx
extern float get_throttleval ( void );
extern float get_aileronval  ( void );
extern float get_elevatorval ( void );
extern float get_elev_trimval( void );
extern float get_rudderval   ( void );
extern float get_speed       ( void );
extern float get_aoa         ( void );
extern float get_roll        ( void );
extern float get_pitch       ( void );
extern float get_heading     ( void );
extern float get_view_direction( void );
extern float get_altitude    ( void );
extern float get_agl         ( void );
extern float get_sideslip    ( void );
extern float get_frame_rate  ( void );
extern float get_latitude    ( void );
extern float get_lat_min     ( void );
extern float get_longitude   ( void );
extern float get_long_min    ( void );
extern float get_fov         ( void );
extern float get_vfc_ratio   ( void );
extern float get_vfc_tris_drawn   ( void );
extern float get_vfc_tris_culled   ( void );
extern float get_climb_rate  ( void );
extern char *coord_format_lat(float);
extern char *coord_format_lon(float);
//extern char *coord_format_latlon(float latitude, float longitude);  // cockpit.cxx

extern char *get_formated_gmt_time( void );

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

class fgLineSeg2D {
private:
    GLfloat x0, y0, x1, y1;

public:
    fgLineSeg2D( GLfloat a = 0, GLfloat b =0, GLfloat c = 0, GLfloat d =0 )
        : x0(a), y0(b),  x1(c), y1(d) {}

    fgLineSeg2D( const fgLineSeg2D & image )
        : x0(image.x0), y0(image.y0), x1(image.x1), y1(image.y1) {}

    fgLineSeg2D& operator= ( const fgLineSeg2D & image ) {
        x0 = image.x0; y0 = image.y0; x1 = image.x1; y1 = image.y1; return *this;
    }

    ~fgLineSeg2D() {}
    
    void draw()
    {
        glVertex2f(x0, y0);
        glVertex2f(x1, y1);
    }
};

#define USE_HUD_TextList
extern float              HUD_TextSize;
extern fntRenderer       *HUDtext;
extern float HUD_matrix[16];

class fgText {
private:
    float x, y;
    char msg[32];
public:
    fgText( float x = 0, float y = 0, char *c = NULL )
        : x(x), y(y) {strncpy(msg,c,32-1);}

    fgText( const fgText & image )
        : x(image.x), y(image.y) {strcpy(msg,image.msg);}

    fgText& operator = ( const fgText & image ) {
        strcpy(msg,image.msg); x = image.x; y = image.y;
        return *this;
    }

    ~fgText() {msg[0]='\0';}

    int getStringWidth ( char *str )
    {
        if ( HUDtext && str )
        {
            float r, l ;
            guiFntHandle->getBBox ( str, HUD_TextSize, 0, &l, &r, NULL, NULL ) ;
            return FloatToInt( r - l );
        }
        return 0 ;
    }
    
    int StringWidth (void )
    {
        if ( HUDtext && strlen( msg ))
        {
            float r, l ;
            guiFntHandle->getBBox ( msg, HUD_TextSize, 0, &l, &r, NULL, NULL ) ;
            return FloatToInt( r - l );
        }
        return 0 ;
    }
    
    void Draw(fntRenderer *fnt)
    {
        fnt->start2f( x, y );
        fnt->puts   ( msg ) ;
    }

    void Draw()
    {
        puDrawString ( guiFnt, msg, FloatToInt(x), FloatToInt(y) );
    }
};

class fgLineList {
    vector < fgLineSeg2D > List;
public:
    fgLineList( void ) {}
    ~fgLineList( void ) {}
    void add( fgLineSeg2D seg ) { List.push_back(seg); }
    void erase( void ) { List.erase( List.begin(), List.end() ); }
    void draw( void ) {
        vector < fgLineSeg2D > :: iterator curSeg;
        vector < fgLineSeg2D > :: iterator lastSeg;
        curSeg  = List.begin();
        lastSeg = List.end();
        glBegin(GL_LINES);
        for ( ; curSeg != lastSeg; curSeg++ ) {
            curSeg->draw();
        }
        glEnd();
    }
};

class fgTextList {
    fntRenderer *Font;
    vector< fgText > List;
public:
    fgTextList ( void ) { Font = 0; }
    ~fgTextList( void ) {}
    
    void setFont( fntRenderer *Renderer ) { Font = Renderer; }
    void add( fgText String ) { List.push_back(String); }
    void erase( void ) { List.erase( List.begin(), List.end() ); }
    
    void draw( void ) {
        vector < fgText > :: iterator curString;
        vector < fgText > :: iterator lastString;
        if( Font == 0 ) return;
        curString  = List.begin();
        lastString = List.end();
        glPushAttrib( GL_COLOR_BUFFER_BIT );
        glEnable    ( GL_ALPHA_TEST   ) ;
        glEnable    ( GL_BLEND        ) ;
        glAlphaFunc ( GL_GREATER, 0.1 ) ;
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;

        Font->begin();
        for( ; curString != lastString; curString++ ) {
            curString->Draw(Font);
        }
        Font->end();

        glDisable ( GL_TEXTURE_2D ) ;
        glPopAttrib();
    }
};


inline void Text( fgTextList &List, float x, float y, char *s)
{
    List.add( fgText( x, y, s) );
}

inline void Text( fgTextList &List, fgText &me)
{
    List.add(me);
}

inline void Line( fgLineList &List, float x1, float y1, float x2, float y2)
{
    List.add(fgLineSeg2D(x1,y1,x2,y2));
}


// Declare our externals
extern fgTextList         HUD_TextList;
extern fgLineList         HUD_LineList;
extern fgLineList         HUD_StippleLineList;


class instr_item {  // An Abstract Base Class (ABC)
  private:
    static UINT        instances;     // More than 64K instruments? Nah!
    static int         brightness;
    static glRGBTRIPLE color;

    UINT               handle;
    RECT               scrn_pos;      // Framing - affects scale dimensions
                                    // and orientation. Vert vs Horz, etc.
    FLTFNPTR           load_value_fn;
    float              disp_factor;   // Multiply by to get numbers shown on scale.
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
                FLTFNPTR       data_source,
                float         data_scaling,
                UINT           options,
                bool           working      = true);

    instr_item( const instr_item & image );

    instr_item & operator = ( const instr_item & rhs );
    virtual ~instr_item ();

    int     get_brightness  ( void ) { return brightness;}
    RECT    get_location    ( void ) { return scrn_pos;  }
    bool    is_broken       ( void ) { return broken;    }
    bool    enabled         ( void ) { return is_enabled;}
    bool    data_available  ( void ) { return !!load_value_fn; }
    float   get_value       ( void ) { return load_value_fn(); }
    float   data_scaling    ( void ) { return disp_factor; }
    UINT    get_span        ( void ) { return scr_span;  }
    POINT   get_centroid    ( void ) { return mid_span;  }
    UINT    get_options     ( void ) { return opts;      }

    UINT    huds_vert     (UINT options) { return( options  & HUDS_VERT ); }
    UINT    huds_left     (UINT options) { return( options  & HUDS_LEFT ); }
    UINT    huds_right    (UINT options) { return( options  & HUDS_RIGHT ); }
    UINT    huds_both     (UINT options) { return( (options & HUDS_BOTH) == HUDS_BOTH ); }
    UINT    huds_noticks  (UINT options) { return( options  & HUDS_NOTICKS ); }
    UINT    huds_notext   (UINT options) { return( options  & HUDS_NOTEXT ); }
    UINT    huds_top      (UINT options) { return( options  & HUDS_TOP ); }
    UINT    huds_bottom   (UINT options) { return( options  & HUDS_BOTTOM ); }
  
    virtual void display_enable( bool working ) { is_enabled = !! working;}

    virtual void update( void );
    virtual void break_display ( bool bad );
    virtual void SetBrightness( int illumination_level ); // fgHUDSetBright...
    void         SetPosition  ( int x, int y, UINT width, UINT height );
    UINT         get_Handle( void );
    virtual void draw( void ) = 0;   // Required method in derived classes
    
    void drawOneLine( float x1, float y1, float x2, float y2)
    {
        HUD_LineList.add(fgLineSeg2D(x1,y1,x2,y2));
    }
    void drawOneStippleLine( float x1, float y1, float x2, float y2)
    {
        HUD_StippleLineList.add(fgLineSeg2D(x1,y1,x2,y2));
    }
    void TextString( char *msg, float x, float y )
    {
        HUD_TextList.add(fgText(x, y, msg));        
    }
    int getStringWidth ( char *str )
    {
        if ( HUDtext && str )
        {
            float r, l ;
            guiFntHandle->getBBox ( str, HUD_TextSize, 0, &l, &r, NULL, NULL ) ;
            return FloatToInt( r - l );
        }
        return 0 ;
    }
    
};

typedef instr_item *HIptr;
//typedef deque <  instr_item * > hud_deque_type;
//typedef hud_deque_type::iterator hud_deque_iterator;
//typedef hud_deque_type::const_iterator hud_deque_const_iterator;

extern deque< instr_item *> HUD_deque;
extern int HUD_style;
//extern hud_deque_type HUD_deque;

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
    char format_buffer[80];

  public:
    instr_label( int          x,
                 int          y,
                 UINT         width,
                 UINT         height,
                 FLTFNPTR     data_source,
                 const char  *label_format,
                 const char  *pre_label_string  = 0,
                 const char  *post_label_string = 0,
                 float       scale_data        = 1.0,
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


class lat_label : public instr_item {
  private:
    const char *pformat;
    const char *pre_str;
    const char *post_str;
    fgLabelJust justify;
    int         fontSize;
    int         blink;
    char format_buffer[80];

  public:
    lat_label( int          x,
                 int          y,
                 UINT         width,
                 UINT         height,
                 FLTFNPTR     data_source,
                 const char  *label_format,
                 const char  *pre_label_string  = 0,
                 const char  *post_label_string = 0,
                 float       scale_data        = 1.0,
                 UINT         options           = HUDS_TOP,
                 fgLabelJust  justification     = CENTER_JUST,
                 int          font_size         = SMALL,
                 int          blinking          = NOBLINK,
                 bool         working           = true);

    ~lat_label();

    lat_label( const lat_label & image);
    lat_label & operator = (const lat_label & rhs );
    virtual void draw( void );       // Required method in base class
};

typedef lat_label * pLatlabel;

class lon_label : public instr_item {
  private:
    const char *pformat;
    const char *pre_str;
    const char *post_str;
    fgLabelJust justify;
    int         fontSize;
    int         blink;
    char format_buffer[80];

  public:
    lon_label( int          x,
                 int          y,
                 UINT         width,
                 UINT         height,
                 FLTFNPTR     data_source,
                 const char  *label_format,
                 const char  *pre_label_string  = 0,
                 const char  *post_label_string = 0,
                 float       scale_data        = 1.0,
                 UINT         options           = HUDS_TOP,
                 fgLabelJust  justification     = CENTER_JUST,
                 int          font_size         = SMALL,
                 int          blinking          = NOBLINK,
                 bool         working           = true);

    ~lon_label();

    lon_label( const lon_label & image);
    lon_label & operator = (const lon_label & rhs );
    virtual void draw( void );       // Required method in base class
};

typedef lon_label * pLonlabel;

//
// instr_scale           This class is an abstract base class for both moving
//                       scale and moving needle (fixed scale) indicators. It
// does not draw itself, but is not instanciable.
//

class instr_scale : public instr_item {
  private:
    float range_shown;   // Width Units.
    float Maximum_value; //                ceiling.
    float Minimum_value; // Representation floor.
    float scale_factor;  // factor => screen units/range values.
    UINT   Maj_div;       // major division marker units
    UINT   Min_div;       // minor division marker units
    UINT   Modulo;        // Roll over point
    int    signif_digits; // digits to show to the right.

  public:
    instr_scale( int          x,
                 int          y,
                 UINT         width,
                 UINT         height,
                 FLTFNPTR     load_fn,
                 UINT         options,
                 float       show_range,
                 float       max_value    = 100.0,
                 float       min_value    =   0.0,
                 float       disp_scaling =   1.0,
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
    float min_val      ( void ) { return Minimum_value;}
    float max_val      ( void ) { return Maximum_value;}
    UINT   modulo       ( void ) { return Modulo; }
    float factor       ( void ) { return scale_factor;}
    float range_to_show( void ) { return range_shown;}
};

// hud_card_               This class displays the indicated quantity on
//                         a scale that moves past the pointer. It may be
// horizontal or vertical, read above(left) or below(right) of the base
// line.

class hud_card : public instr_scale {
  private:
    float val_span;
    float half_width_units;
    
  public:
    hud_card( int      x,
              int      y,
              UINT     width,
              UINT     height,
              FLTFNPTR load_fn,
              UINT     options,
              float   maxValue      = 100.0,
              float   minValue      =   0.0,
              float   disp_scaling  =   1.0,
              UINT     major_divs    =  10,
              UINT     minor_divs    =   5,
              UINT     modulator     = 100,
              int      dp_showing    =   2,
              float   value_span    = 100.0,
              bool     working       = true);

    ~hud_card();
    hud_card( const hud_card & image);
    hud_card & operator = (const hud_card & rhs );
//    virtual void display_enable( bool setting );
    virtual void draw( void );       // Required method in base class
};

typedef hud_card * pCardScale;

class guage_instr : public instr_scale {
  public:
    guage_instr( int       x,
                 int       y,
                 UINT      width,
                 UINT      height,
                 FLTFNPTR  load_fn,
                 UINT      options,
                 float    disp_scaling = 1.0,
                 float    maxValue     = 100,
                 float    minValue     =   0,
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
    FLTFNPTR alt_data_source;

  public:
    dual_instr_item ( int       x,
                      int       y,
                      UINT      width,
                      UINT      height,
                      FLTFNPTR  chn1_source,
                      FLTFNPTR  chn2_source,
                      bool      working     = true,
                      UINT      options  = HUDS_TOP);

    virtual ~dual_instr_item() {};
    dual_instr_item( const dual_instr_item & image);
    dual_instr_item & operator = (const dual_instr_item & rhs );

    float current_ch1( void ) { return (float)alt_data_source();}
    float current_ch2( void ) { return (float)get_value();}
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
                 FLTFNPTR  chn1_source  = get_roll,
                 FLTFNPTR  chn2_source  = get_sideslip,
                 float    maxBankAngle = 45.0,
                 float    maxSlipAngle =  5.0,
                 UINT      gap_width    =  5,
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
    float vmax;
    float vmin;
    float factor;

    fgTextList         TextList;
    fgLineList         LineList;
    fgLineList         StippleLineList;

  public:
    HudLadder( int       x,
               int       y,
               UINT      width,
               UINT      height,
               FLTFNPTR  ptch_source    = get_roll,
               FLTFNPTR  roll_source    = get_pitch,
               float     span_units     = 45.0,
               float     division_units = 10.0,
               float     minor_division =  0.0,
               UINT      screen_hole    =   70,
               UINT      lbl_pos        =    0,
               bool      working        = true );

    ~HudLadder();

    HudLadder( const HudLadder & image );
    HudLadder & operator = ( const HudLadder & rhs );
    virtual void draw( void );
    
    void Text( float x, float y, char *s)
    {
        TextList.add( fgText( x, y, s) );
    }

    void Line( float x1, float y1, float x2, float y2)
    {
        LineList.add(fgLineSeg2D(x1,y1,x2,y2));
    }

    void StippleLine( float x1, float y1, float x2, float y2)
    {
        StippleLineList.add(fgLineSeg2D(x1,y1,x2,y2));
    }
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
//extern void strokeString(float xx,
//                       float yy,
//                       char *msg,
//                       void *font = GLUT_STROKE_ROMAN)

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
