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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _OLDHUD_HXX
#define _OLDHUD_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef __CYGWIN__
#include <ieeefp.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <algorithm>    // for_each()
#include <vector>       // STL vector
#include <deque>        // STL double ended queue
#include <fstream>

namespace osg {
    class State;
}

#include <simgear/math/SGMath.hxx>
#include <simgear/constants.h>

#include <Include/fg_typedefs.h>
#include <Aircraft/controls.hxx>
#include <FDM/flight.hxx>
#include <GUI/gui.h>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/viewmgr.hxx>

class FGRunway;

using std::deque;
using std::vector;

#define float_to_int(v) SGMiscf::roundToInt(v)

// some of Norman's crazy optimizations. :-)

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

// Label constants
#define HUD_FONT_SMALL     1
#define HUD_FONT_LARGE     2

enum fgLabelJust{ LEFT_JUST, CENTER_JUST, RIGHT_JUST } ;

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

// in cockpit.cxx
extern float get_throttleval ( void );
extern float get_aileronval  ( void );
extern float get_elevatorval ( void );
extern float get_elev_trimval( void );
extern float get_rudderval   ( void );
extern float get_speed       ( void );
extern float get_aoa         ( void );
extern float get_nlf         ( void );
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
extern float get_mach( void );
extern char *coord_format_lat(float);
extern char *coord_format_lon(float);

extern char *get_formated_gmt_time( void );

enum  hudinstype{ HUDno_instr,
                  HUDscale,
                  HUDlabel,
                  HUDladder,
                  HUDcirc_ladder,
                  HUDhorizon,
                  HUDgauge,
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

    fgLineSeg2D& operator= ( const fgLineSeg2D & image ) { // seems unused
        x0 = image.x0; y0 = image.y0; x1 = image.x1; y1 = image.y1; return *this;
    }

    void draw() const
    {
        glVertex2f(x0, y0);
        glVertex2f(x1, y1);
    }
};

class DrawLineSeg2D {
    public:
        void operator() (const fgLineSeg2D& elem) const {
            elem.draw();
        }
};


#define USE_HUD_TextList
extern fntTexFont        *HUD_Font;
extern float              HUD_TextSize;
extern fntRenderer       *HUDtext;
extern float HUD_matrix[16];

class fgText {
private:
    float x, y;
    string msg;
	bool digit;
    // seems unused   
	
public:
	fgText(float x, float y, const string& c, bool digits=false): x(x), y(y), msg( c), digit( digits) {};	

    fgText( const fgText & image )
		: x(image.x), y(image.y), msg(image.msg), digit(image.digit) { }

	fgText& operator = ( const fgText & image ) {
		x = image.x; y = image.y; msg= image.msg; digit = image.digit;
		return *this;
	}
	
    static int getStringWidth ( const char *str )
    {
        if ( HUDtext && str ) {
            float r, l ;
            HUD_Font->getBBox ( str, HUD_TextSize, 0, &l, &r, NULL, NULL ) ;
            return float_to_int( r - l );
        }
        return 0 ;
    }

    int StringWidth ()
    {
        if ( HUDtext && msg != "") {
            float r, l ;
            HUD_Font->getBBox ( msg.c_str(), HUD_TextSize, 0, &l, &r, NULL, NULL ) ;
            return float_to_int( r - l );
        }
        return 0 ;
    }

    // this code is changed to display Numbers with big/small digits
    // according to MIL Standards for example Altitude above 10000 ft
    // is shown as 10ooo.

    void Draw(fntRenderer *fnt) {
        if (digit) {
            int c=0;
            
            int p=4;

            if (msg[0]=='-') {
                //if negative value then increase the c and p values
                //for '-' sign.  c++;
                p++;
            }
			
			for (string::size_type i = 0; i < msg.size(); i++) {
                if ((msg[i]>='0') && (msg[i]<='9'))
                    c++;
            }
            float orig_size = fnt->getPointSize();
            if (c>p) {
                fnt->setPointSize(HUD_TextSize * 0.8);
                int p2=(c-3)*8;  //advance to the last 3 digits

                fnt->start2f(x+p2,y);
                fnt->puts(msg.c_str() + c - 3); // display last 3 digits

                fnt->setPointSize(HUD_TextSize * 1.2);
              
                fnt->start2f(x,y);
				fnt->puts(msg.substr(0,c-3).c_str());
            } else {
                fnt->setPointSize(HUD_TextSize * 1.2);
                fnt->start2f( x, y );
                fnt->puts(msg.c_str());
            }
            fnt->setPointSize(orig_size);
        } else {
            //if digits not true
            fnt->start2f( x, y );
            fnt->puts( msg.c_str()) ;
        }
    }

    void Draw()
    {
		guiFnt.drawString( msg.c_str(), float_to_int(x), float_to_int(y) );
    }
};

class fgLineList {
    vector < fgLineSeg2D > List;
public:
    fgLineList( void ) {}
    void add( const fgLineSeg2D& seg ) { List.push_back(seg); }
	void erase( void ) { List.clear();}
    void draw( void ) {
        glBegin(GL_LINES);
        for_each( List.begin(), List.end(), DrawLineSeg2D());
        glEnd();
    }
};

class fgTextList {
    fntRenderer *Font;
    vector< fgText > List;
public:
    fgTextList ( void ) { Font = 0; }

    void setFont( fntRenderer *Renderer ) { Font = Renderer; }
    void add( const fgText& String ) { List.push_back(String); }
    void erase( void ) { List.clear(); }
    void draw( void );
};


inline void Text( fgTextList &List, float x, float y, char *s)
{
    List.add( fgText( x, y, s) );
}

inline void Text( fgTextList &List, const fgText &me)
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


class instr_item : public SGReferenced {  // An Abstract Base Class (ABC)
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
    int                digits;

public:
    instr_item( int            x,
                int            y,
                UINT           height,
                UINT           width,
                FLTFNPTR       data_source,
                float          data_scaling,
                UINT           options,
                bool           working = true,
                int            digit = 0);

    virtual ~instr_item ();

    void    set_data_source ( FLTFNPTR fn ) { load_value_fn = fn; }
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
    int     get_digits      ( void ) { return digits;         }
    inline int get_x() const { return scrn_pos.left; }
    inline int get_y() const { return scrn_pos.top; }
    inline int get_width() const { return scrn_pos.right; }
    inline int get_height() const { return scrn_pos.bottom; }

    UINT    huds_vert     (UINT options) { return (options & HUDS_VERT); }
    UINT    huds_left     (UINT options) { return (options & HUDS_LEFT); }
    UINT    huds_right    (UINT options) { return (options & HUDS_RIGHT); }
    UINT    huds_both     (UINT options) {
        return ((options & HUDS_BOTH) == HUDS_BOTH);
    }
    UINT    huds_noticks  (UINT options) { return (options & HUDS_NOTICKS); }
    UINT    huds_notext   (UINT options) { return (options & HUDS_NOTEXT); }
    UINT    huds_top      (UINT options) { return (options & HUDS_TOP); }
    UINT    huds_bottom   (UINT options) { return (options & HUDS_BOTTOM); }

    virtual void display_enable( bool working ) { is_enabled = working;}

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
    void TextString( char *msg, float x, float y, bool digit )
    {
        HUD_TextList.add(fgText(x, y, msg,digit));
    }
    int getStringWidth ( char *str )
    {
        if ( HUDtext && str ) {
            float r, l ;
            HUD_Font->getBBox ( str, HUD_TextSize, 0, &l, &r, NULL, NULL ) ;
            return float_to_int( r - l );
        }
        return 0 ;
    }



};

typedef instr_item *HIptr;

class HUDdraw {
    public:
        void operator() (HIptr elem) const {
            if ( elem->enabled())
                elem->draw();
        }
};


extern int HUD_style;

// instr_item           This class has no other purpose than to maintain
//                      a linked list of instrument and derived class
// object pointers.


class instr_label : public instr_item {
private:
    const char  *pformat;
    
    fgLabelJust justify;
    int         fontSize;
    int         blink;
    string      format_buffer;
    bool        lat;
    bool        lon;
    bool        lbox;
    SGPropertyNode_ptr lon_node;
    SGPropertyNode_ptr lat_node;

public:
    instr_label(const SGPropertyNode *);
    virtual void draw(void);
};


//
// fgRunway_instr   This class is responsible for rendering the active runway
//                  in the hud (if visible).
class runway_instr : public instr_item {
private:
        void boundPoint(const sgdVec3& v, sgdVec3& m);
        bool boundOutsidePoints(sgdVec3& v, sgdVec3& m);
        bool drawLine(const sgdVec3& a1, const sgdVec3& a2,
                const sgdVec3& p1, const sgdVec3& p2);
        void drawArrow();
        FGRunway* get_active_runway();
        void get_rwy_points(sgdVec3 *points);
        void setLineWidth(void);

        sgdVec3 points3d[6], points2d[6];
        double mm[16],pm[16], arrowScale, arrowRad, lnScale;
        double scaleDist, default_pitch, default_heading;
        GLint view[4];
        FGRunway* runway;
        FGViewer* cockpit_view;
        unsigned short stippleOut, stippleCen;
        bool drawIA, drawIAAlways;
        RECT location;
        POINT center;

public:
    runway_instr(const SGPropertyNode *);

    virtual void draw( void );
    void setArrowRotationRadius(double radius);
    // Scales the runway indication arrow
    void setArrowScale(double scale);
    // Draws arrow when runway is not visible in HUD if draw=true
    void setDrawArrow(bool draw);
    // Always draws arrow if draw=true;
    void setDrawArrowAlways(bool draw);
    // Sets the maximum line scale
    void setLineScale(double scale);
    // Sets the distance where to start scaling the lines
    void setScaleDist(double dist_nm);
    // Sets the stipple pattern of the outline of the runway
    void setStippleOutline(unsigned short stipple);
    // Sets the stipple patter of the center line of the runway
    void setStippleCenterline(unsigned short stipple);
};


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
                 float        show_range,
                 float        max_value,
                 float        min_value,
                 float        disp_scaling,
                 UINT         major_divs,
                 UINT         minor_divs,
                 UINT         rollover,
                 int          dp_showing,
                 bool         working = true);

    virtual void draw    ( void ) {}; // No-op here. Defined in derived classes.
    UINT   div_min       ( void ) { return Min_div;}
    UINT   div_max       ( void ) { return Maj_div;}
    float  min_val       ( void ) { return Minimum_value;}
    float  max_val       ( void ) { return Maximum_value;}
    UINT   modulo        ( void ) { return Modulo; }
    float  factor        ( void ) { return scale_factor;}
    float  range_to_show ( void ) { return range_shown;}
};

// hud_card                This class displays the indicated quantity on
//                         a scale that moves past the pointer. It may be
// horizontal or vertical, read above(left) or below(right) of the base
// line.

class hud_card : public instr_scale {
private:
    float  val_span;
    string type;
    float  half_width_units;
    bool   draw_tick_bottom;
    bool   draw_tick_top;
    bool   draw_tick_right;
    bool   draw_tick_left;
    bool   draw_cap_bottom;
    bool   draw_cap_top;
    bool   draw_cap_right;
    bool   draw_cap_left;
    float  marker_offset;
    bool   pointer;
    string pointer_type;
    string tick_type;
    string tick_length;
    float  radius;
    float  maxValue;
    float  minValue;
    int    divisions;
    int    zoom;
    UINT   Maj_div;
    UINT   Min_div;

public:
    hud_card(const SGPropertyNode *);
    //    virtual void display_enable( bool setting );			// FIXME
    virtual void draw(void);
    void circles(float, float, float);
    void fixed(float, float, float, float, float, float);
    void zoomed_scale(int, int);
};


class gauge_instr : public instr_scale {
public:
    gauge_instr(const SGPropertyNode *);
    virtual void draw( void );       // Required method in base class
};


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
                      bool      working,
                      UINT      options );

    float current_ch1( void ) { return (float)alt_data_source(); }
    float current_ch2( void ) { return (float)get_value(); }
    virtual void draw( void ) {}
};


class fgTBI_instr : public dual_instr_item {
private:
    UINT BankLimit;
    UINT SlewLimit;
    UINT scr_hole;
    bool tsi;
    float rad;

public:
    fgTBI_instr(const SGPropertyNode *);

    UINT bank_limit(void) { return BankLimit; }
    UINT slew_limit(void) { return SlewLimit; }

    virtual void draw(void);
};


class HudLadder : public dual_instr_item {
private:
    UINT   width_units;
    int    div_units;
    UINT   minor_div;
    UINT   label_pos;
    UINT   scr_hole;
    float  vmax;
    float  vmin;
    float  factor;
    string hudladder_type;
    bool   frl;
    bool   target_spot;
    bool   velocity_vector;
    bool   drift_marker;
    bool   alpha_bracket;
    bool   energy_marker;
    bool   climb_dive_marker;
    bool   glide_slope_marker;
    float  glide_slope;
    bool   energy_worm;
    bool   waypoint_marker;
    int    zenith;
    int    nadir;
    int    hat;

    // The Ladder has it's own temporary display lists
    fgTextList         TextList;
    fgLineList         LineList;
    fgLineList         StippleLineList;

public:
    HudLadder(const SGPropertyNode *);

    virtual void draw(void);
    void drawZenith(float, float, float);
    void drawNadir(float, float, float);

    void Text(float x, float y, char *s)
    {
        TextList.add(fgText(x, y, s));
    }

    void Line(float x1, float y1, float x2, float y2)
    {
        LineList.add(fgLineSeg2D(x1, y1, x2, y2));
    }

    void StippleLine(float x1, float y1, float x2, float y2)
    {
        StippleLineList.add(fgLineSeg2D(x1, y1, x2, y2));
    }
};


extern int  fgHUDInit();
extern int  fgHUDInit2();
extern void fgUpdateHUD( osg::State* );
extern void fgUpdateHUD( osg::State*, GLfloat x_start, GLfloat y_start,
                         GLfloat x_end, GLfloat y_end );




class HUD_Properties : public SGPropertyChangeListener {
public:
    HUD_Properties();
    void valueChanged(SGPropertyNode *n);
    void setColor() const;
    bool isVisible() const { return _visible; }
    bool isAntialiased() const { return _antialiased; }
    bool isTransparent() const { return _transparent; }
    float alphaClamp() const { return _cl; }

private:
    float clamp(float f) { return f < 0.0f ? 0.0f : f > 1.0f ? 1.0f : f; }
    SGPropertyNode_ptr _current;
    SGPropertyNode_ptr _visibility;
    SGPropertyNode_ptr _antialiasing;
    SGPropertyNode_ptr _transparency;
    SGPropertyNode_ptr _red, _green, _blue, _alpha;
    SGPropertyNode_ptr _alpha_clamp;
    SGPropertyNode_ptr _brightness;
    bool _visible;
    bool _antialiased;
    bool _transparent;
    float _r, _g, _b, _a, _cl;
};

#endif // _OLDHUD_H
