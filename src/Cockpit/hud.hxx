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


#ifndef _HUD_HXX
#define _HUD_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdlib.h>
#include <string.h>

//#ifdef HAVE_VALUES_H
//#  include <values.h>  // for MAXINT
//#endif

#include <algorithm>    // for_each()
#include <vector>       // STL vector
#include <deque>        // STL double ended queue
#include STL_FSTREAM

#include <simgear/constants.h>

#include <Include/fg_typedefs.h>
#include <Aircraft/aircraft.hxx>
#include <Aircraft/controls.hxx>
#include <FDM/flight.hxx>
#include <GUI/gui.h>
#include <Main/globals.hxx>
#include <Main/viewmgr.hxx>
#include <Airports/runways.hxx>

#include "hud_opts.hxx"
#include <plib/sg.h>

SG_USING_STD(deque);
SG_USING_STD(vector);
SG_USING_NAMESPACE(std);


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

// DAY, NIGHT and brightness levels need to be visible where dialogs and
// controls can be used to set intensity and appropriate color. This will
// be moved.
// Hud general constants
#define HUD_DAY                1
#define HUD_NIGHT              2
#define HUD_BRT_BLACK          3
#define HUD_BRT_DARK           4
#define HUD_BRT_MEDIUM         5
#define HUD_BRT_LIGHT          6

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
//extern char *coord_format_latlon(float latitude, float longitude);  // cockpit.cxx

// $$$ begin - added, VS Renganathan, 13 Oct 2K
// #define FIGHTER_HUD
extern float get_anzg (void);
extern float get_Vx (void);
extern float get_Vy (void);
extern float get_Vz (void);
extern float get_Ax (void);
extern float get_Ay (void);
extern float get_Az (void);
extern int get_iaux1 (void);
extern int get_iaux2 (void);
extern int get_iaux3 (void);
extern int get_iaux4 (void);
extern int get_iaux5 (void);
extern int get_iaux6 (void);
extern int get_iaux7 (void);
extern int get_iaux8 (void);
extern int get_iaux9 (void);
extern int get_iaux10 (void);
extern int get_iaux11 (void);
extern int get_iaux12 (void);
extern float get_aux1(void);
extern float get_aux2(void);
extern float get_aux3(void);
extern float get_aux4(void);
extern float get_aux5 (void);
extern float get_aux6 (void);
extern float get_aux7 (void);
extern float get_aux8(void);
extern float get_aux9(void);
extern float get_aux10(void);
extern float get_aux11(void);
extern float get_aux12(void);
extern float get_aux13(void);
extern float get_aux14(void);
extern float get_aux15(void);
extern float get_aux16(void);
extern float get_aux17(void);
extern float get_aux18(void);
// $$$ end - added, VS Renganathan, 13 Oct 2K

extern char *get_formated_gmt_time( void );
extern void fgHUDReshape(void);

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

    fgLineSeg2D& operator= ( const fgLineSeg2D & image ) {
        x0 = image.x0; y0 = image.y0; x1 = image.x1; y1 = image.y1; return *this;
    }

    ~fgLineSeg2D() {}
    
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
    char msg[64];
public:
    int digit; //suma
    fgText(float x = 0, float y = 0, char *c = NULL,int digits=0): x(x), y(y) //suma
    { 
        strcpy(msg,c);
        digit=digits; //suma
    }
		   
    fgText( const fgText & image )
        : x(image.x), y(image.y),digit(image.digit) {strcpy(msg,image.msg);} //suma

    fgText& operator = ( const fgText & image ) {
        strcpy(msg,image.msg); x = image.x; y = image.y;digit=image.digit; //suma
        return *this;   
    }

    ~fgText() {msg[0]='\0';}

    int getStringWidth ( char *str )
    {
        if ( HUDtext && str ) {
            float r, l ;
            HUD_Font->getBBox ( str, HUD_TextSize, 0, &l, &r, NULL, NULL ) ;
            return FloatToInt( r - l );
        }
        return 0 ;
    }
    
    int StringWidth (void )
    {
        if ( HUDtext && strlen( msg )) {
            float r, l ;
            HUD_Font->getBBox ( msg, HUD_TextSize, 0, &l, &r, NULL, NULL ) ;
            return FloatToInt( r - l );
        }
        return 0 ;
    }
   
    // this code is changed to display Numbers with big/small digits
    // according to MIL Standards for example Altitude above 10000 ft
    // is shown as 10ooo.  begin suma

    void Draw(fntRenderer *fnt,int digits) {
        if(digits==1) {
            int c=0,i=0;
            char *t=msg;
            int p=4;

            if(t[0]=='-') {
                //if negative value then increase the c and p values
                //for '-' sign.  c++;
                p++;
            }
            char *tmp=msg;
            while(tmp[i]!='\0') {
                if((tmp[i]>='0') && (tmp[i]<='9'))  
                    c++;
                i++;
            }
            if(c>p) {
                fnt->setPointSize(HUD_TextSize * 0.8);
                int p1=c-3;
                char *tmp1=msg+p1;
                int p2=p1*8;
 			  
                fnt->start2f(x+p2,y);
                fnt->puts(tmp1);

                fnt->setPointSize(HUD_TextSize * 1.2);
                char tmp2[64];
                strncpy(tmp2,msg,p1);
                tmp2[p1]='\0';
			 
                fnt->start2f(x,y);
                fnt->puts(tmp2);
            } else {
                fnt->setPointSize(HUD_TextSize * 1.2);
                fnt->start2f( x, y );
                fnt->puts(tmp);
            }
        } else {
            //if digits not equal to 1
            fnt->setPointSize(HUD_TextSize * 0.8);
            fnt->start2f( x, y );
            fnt->puts( msg ) ;
        }
    }
    //end suma

    void Draw()
    {
        guiFnt.drawString( msg, FloatToInt(x), FloatToInt(y) );
    }
};

class fgLineList {
    vector < fgLineSeg2D > List;
public:
    fgLineList( void ) {}
    ~fgLineList( void ) {}
    void add( const fgLineSeg2D& seg ) { List.push_back(seg); }
    void erase( void ) { List.erase( List.begin(), List.end() ); }
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
    ~fgTextList( void ) {}
    
    void setFont( fntRenderer *Renderer ) { Font = Renderer; }
    void add( const fgText& String ) { List.push_back(String); }
    void erase( void ) { List.erase( List.begin(), List.end() ); }
    
    void draw( void ) {
        if( Font == 0 )
            return;
        vector < fgText > :: iterator curString = List.begin();
        vector < fgText > :: iterator lastString = List.end();

        glPushAttrib( GL_COLOR_BUFFER_BIT );
        glEnable    ( GL_ALPHA_TEST   ) ;
        glEnable    ( GL_BLEND        ) ;
        glAlphaFunc ( GL_GREATER, 0.1 ) ;
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;

        Font->begin();
        for( ; curString != lastString; curString++ ) {
            curString->Draw(Font,curString->digit); //suma
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
    int		       digits;        //suma
  
public:
    instr_item( int            x,
                int            y,
                UINT           height,
                UINT           width,
                FLTFNPTR       data_source,
                float          data_scaling,
                UINT           options,
                bool           working  = true,
                int	       digit = 0); //suma

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
    int     get_digits	    ( void ) { return digits;	 } //suma

    UINT    huds_vert     (UINT options) { return( options  & HUDS_VERT ); }
    UINT    huds_left     (UINT options) { return( options  & HUDS_LEFT ); }
    UINT    huds_right    (UINT options) { return( options  & HUDS_RIGHT ); }
    UINT    huds_both     (UINT options) {
        return( (options & HUDS_BOTH) == HUDS_BOTH );
    }
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
    void TextString( char *msg, float x, float y,int digit ) //suma
    {
        HUD_TextList.add(fgText(x, y, msg,digit)); //suma  
    }
    int getStringWidth ( char *str )
    {
        if ( HUDtext && str ) {
            float r, l ;
            HUD_Font->getBBox ( str, HUD_TextSize, 0, &l, &r, NULL, NULL ) ;
            return FloatToInt( r - l );
        }
        return 0 ;
    }

    //code to draw ticks as small circles
    void drawOneCircle(float x1, float y1, float r)
    {
        glBegin(GL_LINE_LOOP);  // Use polygon to approximate a circle 
        for(int count=0; count<25; count++) {             
            float cosine = r * cos(count * 2 * SG_PI/10.0); 
            float sine =   r * sin(count * 2 * SG_PI/10.0); 
            glVertex2f(cosine+x1, sine+y1);
        }     
        glEnd(); 
    }
    
};

typedef instr_item *HIptr;

class HUDdraw {
    public:
        void operator() (HIptr elem) const {
            if( elem->enabled())
                elem->draw();
        }
};

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
    bool		lat;
    bool		lon;
    bool		lbox;

public:
    instr_label( int          x,
                 int          y,
                 UINT         width,
                 UINT         height,
                 FLTFNPTR     data_source,
                 const char  *label_format,
                 const char  *pre_label_string,
                 const char  *post_label_string,
                 float        scale_data,
                 UINT         options,
                 fgLabelJust  justification,
                 int          font_size,
                 int          blinking,
                 bool		  latitude,
                 bool		  longitude,
                 bool		  label_box,
                 bool         working,
                 int          digit ); //suma);

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
               const char  *pre_label_string,
               const char  *post_label_string,
               float       scale_data,
               UINT         options,
               fgLabelJust  justification,
               int          font_size,
               int          blinking,
               bool         working,
               int	    digits =0 );//suma

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
               const char  *pre_label_string,
               const char  *post_label_string,
               float       scale_data,
               UINT         options,
               fgLabelJust  justification,
               int          font_size,
               int          blinking,
               bool         working,
               int	    digit=0); //suma


    ~lon_label();

    lon_label( const lon_label & image);
    lon_label & operator = (const lon_label & rhs );
    virtual void draw( void );       // Required method in base class
};

typedef lon_label * pLonlabel;

//
// fgRunway_instr	This class is responsible for rendering the active runway
//			in the hud (if visible).
class runway_instr : public instr_item 
{
private:	
	void boundPoint(const sgdVec3& v, sgdVec3& m);
	bool boundOutsidePoints(sgdVec3& v, sgdVec3& m);
	bool drawLine(const sgdVec3& a1, const sgdVec3& a2, const sgdVec3& p1, const sgdVec3& p2);
	void drawArrow();
	bool get_active_runway(FGRunway& rwy);
	void get_rwy_points(sgdVec3 *points);
	void setLineWidth(void);

	sgdVec3 points3d[6],points2d[6];
	double mm[16],pm[16], arrowScale, arrowRad, lnScale, scaleDist, default_pitch, default_heading;
	GLint view[4];
	FGRunway runway;
	FGViewer* cockpit_view;	
	unsigned short stippleOut,stippleCen;
	bool drawIA,drawIAAlways;
	RECT location;
	POINT center; 
	
public:
    runway_instr( int    x,
                  int    y,			
				  int	 width,
				  int    height,
               	  float  scale_data,
               	  bool   working = true);

    virtual void draw( void );       // Required method in base class
	void setArrowRotationRadius(double radius);
	void setArrowScale(double scale); // Scales the runway indication arrow
	void setDrawArrow(bool draw);	 // Draws arrow when runway is not visible in HUD if draw=true
	void setDrawArrowAlways(bool draw); //Always draws arrow if draw=true;
	void setLineScale(double scale); //Sets the maximum line scale
	void setScaleDist(double dist_nm); //Sets the distance where to start scaling the lines
	void setStippleOutline(unsigned short stipple); //Sets the stipple pattern of the outline of the runway
	void setStippleCenterline(unsigned short stipple); //Sets the stipple patter of the center line of the runway
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
                 float       show_range,
                 float       max_value,
                 float       min_value,
                 float       disp_scaling,
                 UINT         major_divs,
                 UINT         minor_divs,
                 UINT         rollover,
                 int          dp_showing,
                 bool         working = true);

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
    string type;
    float half_width_units;
    bool  draw_tick_bottom;
    bool  draw_tick_top;
    bool  draw_tick_right;
    bool  draw_tick_left;
    bool  draw_cap_bottom;
    bool  draw_cap_top;
    bool  draw_cap_right;
    bool  draw_cap_left;
    float marker_offset;
    bool  pointer;
    string  pointer_type;
    string  tick_type;
    string  tick_length;
    float   radius; //suma
    float   maxValue; //suma
    float   minValue; //suma
    int		divisions; //suma
    int     zoom; //suma
    UINT	Maj_div; //suma
    UINT	Min_div; //suma
	
    
public:
    hud_card( int      x,
              int      y,
              UINT     width,
              UINT     height,
              FLTFNPTR load_fn,
              UINT     options,
              float    maxValue,
              float    minValue,
              float    disp_scaling,
              UINT     major_divs,
              UINT     minor_divs,
              UINT     modulator,
              int      dp_showing,
              float    value_span,
	      string   type,
	      bool     draw_tick_bottom,
	      bool     draw_tick_top,
	      bool     draw_tick_right,
	      bool     draw_tick_left,
	      bool     draw_cap_bottom,
	      bool     draw_cap_top,
	      bool     draw_cap_right,
	      bool     draw_cap_left,
	      float    marker_offset,
	      bool     pointer,
	      string   pointer_type,
              string  tick_type,
              string  tick_length,
              bool     working,
              float    radius, //suma
              int      divisions, //suma
              int       zoom //suma
            );


    ~hud_card();
    hud_card( const hud_card & image);
    hud_card & operator = (const hud_card & rhs );
    //    virtual void display_enable( bool setting );
    virtual void draw( void );       // Required method in base class
    void circles(float,float,float); // suma
    void fixed(float,float,float,float,float,float); //suma
    void zoomed_scale(int,int); //suma
};

typedef hud_card * pCardScale;

class gauge_instr : public instr_scale {
public:
    gauge_instr( int       x,
                 int       y,
                 UINT      width,
                 UINT      height,
                 FLTFNPTR  load_fn,
                 UINT      options,
                 float     disp_scaling,
                 float     maxValue,
                 float     minValue,
                 UINT      major_divs,
                 UINT      minor_divs,
                 int       dp_showing,
                 UINT      modulus,
                 bool      working);

    ~gauge_instr();
    gauge_instr( const gauge_instr & image);
    gauge_instr & operator = (const gauge_instr & rhs );
    virtual void draw( void );       // Required method in base class
};

typedef gauge_instr * pGaugeInst;
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

    virtual ~dual_instr_item() {};
    dual_instr_item( const dual_instr_item & image);
    dual_instr_item & operator = (const dual_instr_item & rhs );

    float current_ch1( void ) { return (float)alt_data_source();}
    float current_ch2( void ) { return (float)get_value();}
    virtual void draw ( void ) { }
};

class fgTBI_instr : public dual_instr_item 
{
private:
    UINT BankLimit;
    UINT SlewLimit;
    UINT scr_hole;
    bool tsi;  //suma
    float rad; //suma

public:
    fgTBI_instr( int       x,
                 int       y,
                 UINT      width,
                 UINT      height,
                 FLTFNPTR  chn1_source,
                 FLTFNPTR  chn2_source,
                 float    maxBankAngle,
                 float    maxSlipAngle,
                 UINT      gap_width,
                 bool      working,
                 bool	   tsi, //suma
                 float     rad); //suma

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
    float  vmax;
    float  vmin;
    float  factor;
    string hudladder_type;
    bool   frl;
    bool   target_spot;
    bool   velocity_vector;
    bool   drift_marker;
    bool   alpha_bracket;
    bool	energy_marker;
    bool	climb_dive_marker;
    bool	glide_slope_marker;
    float	glide_slope;
    bool	energy_worm;
    bool	waypoint_marker;
    int     zenith; //suma
    int     nadir; //suma
    int		hat; //suma
    

    // The Ladder has it's own temporary display lists
    fgTextList         TextList;
    fgLineList         LineList;
    fgLineList         StippleLineList;

public:
    HudLadder( const string&    name,
	       int       x,
               int       y,
               UINT      width,
               UINT      height,
	       float	 factor,
               FLTFNPTR  ptch_source,
               FLTFNPTR  roll_source,
               float     span_units,
               float     division_units,
               float     minor_division,
               UINT      screen_hole,
               UINT      lbl_pos,
	       bool	 frl,
	       bool	 target_spot,
	       bool     velocity_vector,
	       bool     drift_marker,
	       bool     alpha_bracket,
	       bool	 energy_marker,
	       bool	 climb_dive_marker,
	       bool	 glide_slope_marker,
	       float	 glide_slope,
	       bool	 energy_worm,
	       bool	 waypoint_marker,
               bool  working,
               int   zenith, //suma
               int   nadir, //suma
               int      hat
             ); //suma


    ~HudLadder();

    HudLadder( const HudLadder & image );
    HudLadder & operator = ( const HudLadder & rhs );
    virtual void draw( void );
    void drawZenith(float,float,float); //suma
    void drawNadir(float, float, float); //suma

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

extern int  fgHUDInit( fgAIRCRAFT * /* current_aircraft */ );
extern int  fgHUDInit2( fgAIRCRAFT * /* current_aircraft */ );
extern void fgUpdateHUD( void );
extern void fgUpdateHUD( GLfloat x_start, GLfloat y_start,
                         GLfloat x_end, GLfloat y_end );

/*
bool AddHUDInstrument( instr_item *pBlackBox );
void DrawHUD ( void );
bool DamageInstrument( INSTR_HANDLE unit );
bool RepairInstrument( INSTR_HANDLE unit );


void fgUpdateHUD ( Hptr hud );
void fgUpdateHUD2( Hptr hud ); // Future use?
void fgHUDSetTimeMode( Hptr hud, int time_of_day );
*/




class HUD_Properties : public SGPropertyChangeListener {
public:
    HUD_Properties();
    void valueChanged(SGPropertyNode *n);
    void setColor() const;
    bool isVisible() const { return _visible; }
    bool isAntialiased() const { return _antialiased; }

private:
    float clamp(float f) { return f < 0.0f ? 0.0f : f > 1.0f ? 1.0f : f; }
    vector<SGPropertyNode_ptr> _colors;
    SGPropertyNode_ptr _which;
    SGPropertyNode_ptr _brightness;
    SGPropertyNode_ptr _alpha;
    SGPropertyNode_ptr _visibility;
    SGPropertyNode_ptr _antialiasing;
    bool _visible, _antialiased;
    float _r, _g, _b, _a;
};

#endif // _HUD_H
