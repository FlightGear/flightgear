// hud.cxx -- hud defines and prototypes
//
// Written by Michele America, started September 1997.
//
// Copyright (C) 1997  Michele F. America  - micheleamerica@geocities.com
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

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>

#include STL_STRING
#include STL_FSTREAM

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#ifdef __BORLANDC__
#  define exception c_exception
#endif

#include <math.h>

#include <stdlib.h>
#include <stdio.h>		// char related functions
#include <string.h>		// strcmp()

#include <GL/glu.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Aircraft/aircraft.hxx>
#include <Autopilot/xmlauto.hxx>
#include <GUI/gui.h>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Scenery/scenery.hxx>

#if defined ( __sun__ ) || defined ( __sgi )
extern "C" {
    extern void *memmove(void *, const void *, size_t);
}
#endif

#include "hud.hxx"

static char units[5];

// The following routines obtain information concerning the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

deque< instr_item * > HUD_deque;

fgTextList         HUD_TextList;
fgLineList         HUD_LineList;
fgLineList         HUD_StippleLineList;

fntRenderer *HUDtext = 0;
float  HUD_TextSize = 0;
int HUD_style = 0;

float HUD_matrix[16];
static float hud_trans_alpha = 0.67f;


//$$$ begin - added, Neetha, 28 Nov 2k

static string	name;
static int		x;
static int		y;
static UINT    width;
static UINT    height;
static float	factor;
static float   span_units;
static float   division_units;
static float   minor_division = 0;
static UINT    screen_hole;
static UINT    lbl_pos;
static bool    working;
static string  loadfn;
static UINT	options;
static float   maxValue;
static float   minValue;
static float   scaling;
static UINT    major_divs;
static UINT    minor_divs;
static UINT    modulator;
static int     dp_showing = 0;
static string  label_format;
static string  prelabel;
static string  postlabel;
static int     justi;
static int		blinking;
static float   maxBankAngle;
static float   maxSlipAngle;
static UINT    gap_width;
static bool	latitude;
static bool	longitude;
static bool	tick_bottom;
static bool	tick_top;
static bool	tick_right;
static bool	tick_left;
static bool	cap_bottom;
static bool	cap_top;
static bool	cap_right;
static bool	cap_left;
static float   marker_off;
static string  type;
static bool    enable_pointer;
static string  type_pointer;
static bool    frl_spot;
static bool	target;
static bool    vel_vector;
static bool    drift;
static bool    alpha;
static bool	energy;
static bool	climb_dive;
static bool	glide;
static float	glide_slope_val;
static bool	worm_energy;
static bool	waypoint;
static string type_tick;//hud
static string length_tick;//hud
static bool label_box;//hud
static int digits; //suma
static float radius; //suma
static int divisions; //suma
static int zoom; //suma
static int zenith; //suma
static int nadir ; //suma
static int hat; //suma
static bool tsi; //suma
static float rad; //suma


static FLTFNPTR load_fn;    
static fgLabelJust justification;
static const char *pre_label_string  = 0;
static const char *post_label_string = 0;

int readHud( istream &input );
int readInstrument ( const SGPropertyNode * node);
static instr_item * readLadder ( const SGPropertyNode * node);
static instr_item * readCard ( const SGPropertyNode * node);
static instr_item * readLabel( const SGPropertyNode * node);
static instr_item * readTBI( const SGPropertyNode * node);
//$$$ end   - added, Neetha, 28 Nov 2k

static void drawHUD();
static void fgUpdateHUDVirtual();

void fgHUDalphaInit( void );

class locRECT {
public:
    RECT rect;

    locRECT( UINT left, UINT top, UINT right, UINT bottom);
    RECT get_rect(void) { return rect;}
};

locRECT :: locRECT( UINT left, UINT top, UINT right, UINT bottom)
{
    rect.left   =  left;
    rect.top    =  top;
    rect.right  =  right;
    rect.bottom =  bottom;

}
// #define DEBUG

//========================= End of Class Implementations===================
// fgHUDInit
//
// Constructs a HUD object and then adds in instruments. At the present
// the instruments are hard coded into the routine. Ultimately these need
// to be defined by the aircraft's instrumentation records so that the
// display for a Piper Cub doesn't show the speed range of a North American
// mustange and the engine readouts of a B36!
//

#define INSTRDEFS 21

//$$$ begin - added, Neetha, 28 Nov 2k
static instr_item * 
readLadder(const SGPropertyNode * node)
{

    instr_item *p;

    name			= node->getStringValue("name");
    x				= node->getIntValue("x");
    y				= node->getIntValue("y");
    width			= node->getIntValue("width");
    height			= node->getIntValue("height");
    factor			= node->getFloatValue("compression_factor");
    span_units		= node->getFloatValue("span_units");
    division_units	= node->getFloatValue("division_units");
    screen_hole		= node->getIntValue("screen_hole");
    lbl_pos			= node->getIntValue("lbl_pos");
    frl_spot		= node->getBoolValue("enable_frl",false);
    target			= node->getBoolValue("enable_target_spot",false);
    vel_vector		= node->getBoolValue("enable_velocity_vector",false);
    drift			= node->getBoolValue("enable_drift_marker",false);
    alpha			= node->getBoolValue("enable_alpha_bracket",false);
    energy			= node->getBoolValue("enable_energy_marker",false);
    climb_dive		= node->getBoolValue("enable_climb_dive_marker",false);
    glide			= node->getBoolValue("enable_glide_slope_marker",false);
    glide_slope_val	= node->getFloatValue("glide_slope",-4.0);
    worm_energy		= node->getBoolValue("enable_energy_marker",false);
    waypoint		= node->getBoolValue("enable_waypoint_marker",false);
    working			= node->getBoolValue("working");
    zenith			= node->getIntValue("zenith");  //suma
    nadir			= node->getIntValue("nadir");  //suma
    hat				= node->getIntValue("hat");
    // The factor assumes a base of 55 degrees per 640 pixels.
    // Invert to convert the "compression" factor to a
    // pixels-per-degree number.
    if(fgGetBool("/sim/hud/enable3d", true))
    {
        if (HUD_style == 1)
        {
            factor = 1;
            factor = (640./55.) / factor;
        }
    }

    SG_LOG(SG_INPUT, SG_INFO, "Done reading instrument " << name);
	
    p = (instr_item *) new HudLadder( name, x, y,
                                      width, height, factor,
                                      get_roll, get_pitch,
                                      span_units, division_units, minor_division,
                                      screen_hole, lbl_pos, frl_spot, target, vel_vector, 
                                      drift, alpha, energy, climb_dive, 
                                      glide, glide_slope_val, worm_energy, 
                                      waypoint, working, zenith, nadir, hat);
				
    return p;
		
} //end readLadder

static instr_item * 
readCard(const SGPropertyNode * node)
{

    instr_item *p;

    name			= node->getStringValue("name");
    x				= node->getIntValue("x");
    y				= node->getIntValue("y");
    width			= node->getIntValue("width");
    height			= node->getIntValue("height");
    loadfn			= node->getStringValue("loadfn");
    options			= node->getIntValue("options");
    maxValue		= node->getFloatValue("maxValue");
    minValue		= node->getFloatValue("minValue");
    scaling			= node->getFloatValue("disp_scaling");
    major_divs		= node->getIntValue("major_divs");
    minor_divs		= node->getIntValue("minor_divs");
    modulator		= node->getIntValue("modulator");
    span_units		= node->getFloatValue("value_span");
    type			= node->getStringValue("type");
    tick_bottom	    = node->getBoolValue("tick_bottom",false);
    tick_top		= node->getBoolValue("tick_top",false);
    tick_right		= node->getBoolValue("tick_right",false);
    tick_left		= node->getBoolValue("tick_left",false);
    cap_bottom		= node->getBoolValue("cap_bottom",false);
    cap_top			= node->getBoolValue("cap_top",false);
    cap_right		= node->getBoolValue("cap_right",false);
    cap_left		= node->getBoolValue("cap_left",false);
    marker_off		= node->getFloatValue("marker_offset",0.0);
    enable_pointer	= node->getBoolValue("enable_pointer",true);
    type_pointer	= node->getStringValue("pointer_type");
    type_tick		= node->getStringValue("tick_type");//hud Can be 'circle' or 'line'
    length_tick		= node->getStringValue("tick_length");//hud For variable length
    working			= node->getBoolValue("working");
    radius			= node->getFloatValue("radius"); //suma
    divisions		= node->getIntValue("divisions"); //suma
    zoom			= node->getIntValue("zoom"); //suma

    SG_LOG(SG_INPUT, SG_INFO, "Done reading instrument " << name);


    if(type=="gauge") {
        span_units = maxValue - minValue;
    }

    if (loadfn=="anzg") {
        load_fn = get_anzg;
    } else if (loadfn=="heading") {
        load_fn = get_heading;
    } else if (loadfn=="aoa") {
        load_fn = get_aoa;
    } else if (loadfn=="climb") {
        load_fn = get_climb_rate;
    } else if (loadfn=="altitude") {
        load_fn = get_altitude;
    } else if (loadfn=="agl") {
        load_fn = get_agl;
    } else if (loadfn=="speed") {
        load_fn = get_speed;
    } else if (loadfn=="view_direction") {
        load_fn = get_view_direction;
    } else if (loadfn=="aileronval") {
        load_fn = get_aileronval;
    } else if (loadfn=="elevatorval") {
        load_fn = get_elevatorval;
    } else if (loadfn=="elevatortrimval") {
        load_fn = get_elev_trimval;
    } else if (loadfn=="rudderval") {
        load_fn = get_rudderval;
    } else if (loadfn=="throttleval") {
        load_fn = get_throttleval;
    }


    if ( (type == "dial") | (type == "tape") ) {
        p = (instr_item *) new hud_card( x,
                                         y,  
                                         width,
                                         height,
                                         load_fn,
                                         options,
                                         maxValue, minValue,
                                         scaling,
                                         major_divs, minor_divs,
                                         modulator,
                                         dp_showing,
                                         span_units,
                                         type,
                                         tick_bottom,
                                         tick_top,
                                         tick_right,
                                         tick_left,
                                         cap_bottom,
                                         cap_top,
                                         cap_right,
                                         cap_left,
                                         marker_off,
                                         enable_pointer,
                                         type_pointer,
                                         type_tick,//hud
                                         length_tick,//hud
                                         working,
                                         radius, //suma
                                         divisions, //suma
                                         zoom  //suma
                                         );
    } else {
        p = (instr_item *) new  gauge_instr( x,            // x
                                             y,  // y
                                             width,            // width
                                             height,            // height
                                             load_fn, // data source
                                             options,
                                             scaling,
                                             maxValue,minValue,
                                             major_divs, minor_divs,
                                             dp_showing,
                                             modulator,
                                             working);
    }

    return p;
}// end readCard

static instr_item *
readLabel(const SGPropertyNode * node)
{
    instr_item *p;

    int font_size = (fgGetInt("/sim/startup/xsize") > 1000) ? HUD_FONT_LARGE : HUD_FONT_SMALL;

    name		= node->getStringValue("name");
    x                   = node->getIntValue("x");
    y                   = node->getIntValue("y");
    width               = node->getIntValue("width");
    height		= node->getIntValue("height");
    loadfn		= node->getStringValue("data_source");
    label_format	= node->getStringValue("label_format");
    prelabel		= node->getStringValue("pre_label_string");
    postlabel		= node->getStringValue("post_label_string");
    scaling		= node->getFloatValue("scale_data");
    options		= node->getIntValue("options");
    justi		= node->getIntValue("justification");
    blinking            = node->getIntValue("blinking");
    latitude		= node->getBoolValue("latitude",false);
    longitude		= node->getBoolValue("longitude",false);
    label_box		= node->getBoolValue("label_box",false);//hud
    working             = node->getBoolValue("working");
    digits		= node->getIntValue("digits"); //suma


    SG_LOG(SG_INPUT, SG_INFO, "Done reading instrument " << name);


    if ( justi == 0 ) {
        justification = LEFT_JUST;
    } else {
        if ( justi == 1 ) {
            justification = CENTER_JUST;
        } else {
            if ( justi == 2 ) {
                justification = RIGHT_JUST;
            }
        }
    }

    if ( prelabel == "NULL" ) {
        pre_label_string = NULL;
    } else {
        if ( prelabel == "blank" ) {
            pre_label_string = " ";
        } else {
            pre_label_string = prelabel.c_str();
        }
    }

    if ( postlabel == "blank" ) {
        post_label_string = " ";
    } else {
        if ( postlabel == "NULL" ) {
            post_label_string = NULL;
        } else {
            if ( postlabel == "units" ) {
                post_label_string = units;
            } else {
                post_label_string = postlabel.c_str();
            }
        }
    }

    if ( loadfn== "aux1" ) {
        load_fn = get_aux1;
    } else if ( loadfn == "aux2" ) {
        load_fn = get_aux2;
    } else if ( loadfn == "aux3" ) {
        load_fn = get_aux3;
    } else if ( loadfn == "aux4" ) {
        load_fn = get_aux4;
    } else if ( loadfn == "aux5" ) {
        load_fn = get_aux5;
    } else if ( loadfn == "aux6" ) {
        load_fn = get_aux6;
    } else if ( loadfn == "aux7" ) {
        load_fn = get_aux7;
    } else if ( loadfn == "aux8" ) {
        load_fn = get_aux8;
    } else if ( loadfn == "aux9" ) {
        load_fn = get_aux9;
    } else if ( loadfn == "aux10" ) {
        load_fn = get_aux10;
    } else if ( loadfn == "aux11" ) {
        load_fn = get_aux11;
    } else if ( loadfn == "aux12" ) {
        load_fn = get_aux12;
    } else if ( loadfn == "aux13" ) {
        load_fn = get_aux13;
    } else if ( loadfn == "aux14" ) {
        load_fn = get_aux14;
    } else if ( loadfn == "aux15" ) {
        load_fn = get_aux15;
    } else if ( loadfn == "aux16" ) {
        load_fn = get_aux16;
    } else if ( loadfn == "aux17" ) {
        load_fn = get_aux17;
    } else if ( loadfn == "aux18" ) {
        load_fn = get_aux18;
    } else if ( loadfn == "ax" ) {
        load_fn = get_Ax;
    } else if ( loadfn == "speed" ) {
        load_fn = get_speed;
    } else if ( loadfn == "mach" ) {
        load_fn = get_mach;
    } else if ( loadfn == "altitude" ) {
        load_fn = get_altitude;
    } else if ( loadfn == "agl" ) {
        load_fn = get_agl;
    } else if ( loadfn == "framerate" ) {
        load_fn = get_frame_rate;
    } else if ( loadfn == "heading" ) {
        load_fn = get_heading;
    } else if ( loadfn == "fov" ) {
        load_fn = get_fov;
    } else if ( loadfn == "vfc_tris_culled" ) {
        load_fn = get_vfc_tris_culled;
    } else if ( loadfn == "vfc_tris_drawn" ) {
        load_fn = get_vfc_tris_drawn;
    } else if ( loadfn == "aoa" ) {
        load_fn = get_aoa;
    } else if ( loadfn == "latitude" ) {
        load_fn  = get_latitude;
    } else if ( loadfn == "anzg" ) {
        load_fn = get_anzg;
    } else if ( loadfn == "longitude" ) {
        load_fn   = get_longitude;
    } else if (loadfn=="throttleval") {
        load_fn = get_throttleval;
    }

    p = (instr_item *) new instr_label ( x,
                                         y,
                                         width,
                                         height,
                                         load_fn,
                                         label_format.c_str(),
                                         pre_label_string,
                                         post_label_string,
                                         scaling,
                                         options,
                                         justification,
                                         font_size,
                                         blinking,
                                         latitude,
                                         longitude,
                                         label_box, //hud
                                         working,
                                         digits); //suma

    return p;
} // end readLabel

static instr_item * 
readTBI(const SGPropertyNode * node)
{

    instr_item *p;

    name           = node->getStringValue("name");
    x              = node->getIntValue("x");
    y              = node->getIntValue("y");
    width          = node->getIntValue("width");
    height         = node->getIntValue("height");
    maxBankAngle   = node->getFloatValue("maxBankAngle");
    maxSlipAngle   = node->getFloatValue("maxSlipAngle");
    gap_width      = node->getIntValue("gap_width");
    working        = node->getBoolValue("working");
    tsi			   = node->getBoolValue("tsi"); //suma
    rad			   = node->getFloatValue("rad"); //suma

    SG_LOG(SG_INPUT, SG_INFO, "Done reading instrument " << name);


    p = (instr_item *) new fgTBI_instr(	x,
                                        y,  
                                        width,
                                        height,
                                        get_roll,
                                        get_sideslip,
                                        maxBankAngle, 
                                        maxSlipAngle,
                                        gap_width,
                                        working,
                                        tsi, //suma
                                        rad); //suma

    return p;
} //end readTBI


int readInstrument(const SGPropertyNode * node)
{
    static const SGPropertyNode *startup_units_node
        = fgGetNode("/sim/startup/units");

    instr_item *HIptr;
    
    if ( !strcmp(startup_units_node->getStringValue(), "feet") ) {
        strcpy(units, " ft");
    } else {
        strcpy(units, " m");
    }

    const SGPropertyNode * ladder_group = node->getNode("ladders");

    if (ladder_group != 0) {
        int nLadders = ladder_group->nChildren();
        for (int j = 0; j < nLadders; j++) {
            
            HIptr = readLadder(ladder_group->getChild(j));
            HUD_deque.insert( HUD_deque.begin(), HIptr);
					
        }// for - ladders
    }

    const SGPropertyNode * card_group = node->getNode("cards");
    if (card_group != 0) {
        int nCards = card_group->nChildren();
        for (int j = 0; j < nCards; j++) {
            
            HIptr = readCard(card_group->getChild(j));
            HUD_deque.insert( HUD_deque.begin(), HIptr);

        }//for - cards
    }

    const SGPropertyNode * label_group = node->getNode("labels");
    if (label_group != 0) {
        int nLabels = label_group->nChildren();
        for (int j = 0; j < nLabels; j++) {

            HIptr = readLabel(label_group->getChild(j));
            HUD_deque.insert( HUD_deque.begin(), HIptr);

        }//for - labels
    }

    const SGPropertyNode * tbi_group = node->getNode("tbis");
    if (tbi_group != 0) {
        int nTbis = tbi_group->nChildren();
        for (int j = 0; j < nTbis; j++) {

            HIptr = readTBI(tbi_group->getChild(j));
            HUD_deque.insert( HUD_deque.begin(), HIptr);

        }//for - tbis
    }
    return 0;
}//end readinstrument


int readHud( istream &input ) 
{

    SGPropertyNode root;

    try {
        readProperties(input, &root);
    } catch (const sg_exception &e) {
        guiErrorMessage("Error reading HUD: ", e);
        return 0;
    }
  
	
    SG_LOG(SG_INPUT, SG_INFO, "Read properties for  " <<
           root.getStringValue("name"));


    HUD_deque.erase( HUD_deque.begin(), HUD_deque.end());  // empty the HUD deque


    SG_LOG(SG_INPUT, SG_INFO, "Reading Hud instruments");

    const SGPropertyNode * instrument_group = root.getChild("instruments");
    int nInstruments = instrument_group->nChildren();

    for (int i = 0; i < nInstruments; i++) {
		
        const SGPropertyNode * node = instrument_group->getChild(i);

        SGPath path( globals->get_fg_root() );
        path.append(node->getStringValue("path"));

        SG_LOG(SG_INPUT, SG_INFO, "Reading Instrument "
               << node->getName()
               << " from "
               << path.str());

        SGPropertyNode root2;
        try {
            readProperties(path.str(), &root2);
        } catch (const sg_exception &e) {
            guiErrorMessage("Error reading HUD instrument: ", e);
            continue;
        } 
        readInstrument(&root2);
    }//for loop(i)

    return 0;
}


int fgHUDInit( fgAIRCRAFT * /* current_aircraft */ )
{

    HUD_style = 1;

    SG_LOG( SG_COCKPIT, SG_INFO, "Initializing current aircraft HUD" );

    string hud_path =
	fgGetString("/sim/hud/path", "Huds/Default/default.xml");
    SGPath path(globals->get_fg_root());
    path.append(hud_path);
	
    ifstream input(path.c_str());
    if (!input.good()) {
        SG_LOG(SG_INPUT, SG_ALERT,
               "Cannot read Hud configuration from " << path.str());
    } else {
        readHud(input);
        input.close();
    }

    fgHUDalphaInit();
    fgHUDReshape();

    if ( HUDtext ) {
        // this chunk of code is not necessarily thread safe if the
        // compiler optimizer reorders these statements.  Note that
        // "delete ptr" does not set "ptr = NULL".  We have to do that
        // ourselves.
        fntRenderer *tmp = HUDtext;
        HUDtext = NULL;
        delete tmp;
    }

//    HUD_TextSize = fgGetInt("/sim/startup/xsize") / 60;
    HUD_TextSize = 10;
    HUDtext = new fntRenderer();
    HUDtext -> setFont      ( guiFntHandle ) ;
    HUDtext -> setPointSize ( HUD_TextSize ) ;
    HUD_TextList.setFont( HUDtext );
    
    return 0;  // For now. Later we may use this for an error code.

}

int fgHUDInit2( fgAIRCRAFT * /* current_aircraft */ )
{

    HUD_style = 2;

    SG_LOG( SG_COCKPIT, SG_INFO, "Initializing current aircraft HUD" );

    SGPath path(globals->get_fg_root());
    path.append("Huds/Minimal/default.xml");


    ifstream input(path.c_str());
    if (!input.good()) {
        SG_LOG(SG_INPUT, SG_ALERT,
               "Cannot read Hud configuration from " << path.str());
    } else {
        readHud(input);
        input.close();
    }

    return 0;  // For now. Later we may use this for an error code.

}
//$$$ End - added, Neetha, 28 Nov 2k  

static int global_day_night_switch = HUD_DAY;

void HUD_masterswitch( bool incr )
{
    if ( fgGetBool("/sim/hud/visibility") ) {
	if ( global_day_night_switch == HUD_DAY ) {
	    global_day_night_switch = HUD_NIGHT;
	} else {
	    fgSetBool("/sim/hud/visibility", false);
	}
    } else {
        fgSetBool("/sim/hud/visibility", true);
	global_day_night_switch = HUD_DAY;
    }	
}

void HUD_brightkey( bool incr_bright )
{
    instr_item *pHUDInstr = HUD_deque[0];
    int brightness        = pHUDInstr->get_brightness();

    if( fgGetBool("/sim/hud/visibility") ) {
	if( incr_bright ) {
	    switch (brightness)
		{
		case HUD_BRT_LIGHT:
		    brightness = HUD_BRT_BLACK;
		    break;

		case HUD_BRT_MEDIUM:
		    brightness = HUD_BRT_LIGHT;
		    break;

		case HUD_BRT_DARK:
		    brightness = HUD_BRT_MEDIUM;
		    break;

		case HUD_BRT_BLACK:
		    brightness = HUD_BRT_DARK;
		    break;

		default:
		    brightness = HUD_BRT_BLACK;
		}
	} else {
	    switch (brightness)
		{
		case HUD_BRT_LIGHT:
		    brightness = HUD_BRT_MEDIUM;
		    break;

		case HUD_BRT_MEDIUM:
		    brightness = HUD_BRT_DARK;
		    break;

		case HUD_BRT_DARK:
		    brightness = HUD_BRT_BLACK;
		    break;

		case HUD_BRT_BLACK:
		    brightness = HUD_BRT_LIGHT;
		    break;

		default:
		    fgSetBool("/sim/hud/visibility", false);
		}
	}
    } else {
	fgSetBool("/sim/hud/visibility", true);
    }

    pHUDInstr->SetBrightness( brightness );
}


#define fgAP_CLAMP(val,min,max) ( (val) = (val) > (max) ? (max) : (val) < (min) ? (min) : (val) )

static puDialogBox *HUDalphaDialog;
static puText      *HUDalphaText;
static puSlider    *HUDalphaHS0;
//static puText      *HUDtextText;
//static puSlider    *HUDalphaHS1;
static char         SliderText[2][ 8 ];

static void alpha_adj( puObject *hs ) {
    float val ;

    hs-> getValue ( &val ) ;
    fgAP_CLAMP ( val, 0.1, 1.0 ) ;
    // printf ( "maxroll_adj( %p ) %f %f\n", hs, val, MaxRollAdjust * val ) ;
    hud_trans_alpha = val;
    sprintf( SliderText[ 0 ], "%05.2f", hud_trans_alpha );
    HUDalphaText -> setLabel ( SliderText[ 0 ] ) ;
}

void fgHUDalphaAdjust( puObject * ) {
    fgSetBool("/sim/hud/antialiased", true);
    FG_PUSH_PUI_DIALOG( HUDalphaDialog );
}

static void goAwayHUDalphaAdjust (puObject *)
{
    FG_POP_PUI_DIALOG( HUDalphaDialog );
}

static void cancelHUDalphaAdjust (puObject *)
{
    fgSetBool("/sim/hud/antialiased", false);
    FG_POP_PUI_DIALOG( HUDalphaDialog );
}

// Done once at system initialization
void fgHUDalphaInit( void ) {

    //	printf("fgHUDalphaInit\n");
#define HORIZONTAL  FALSE

    int DialogX = 40;
    int DialogY = 100;
    int DialogWidth = 240;

    char Label[] =  "HUD Adjuster";
    char *s;

    int labelX = (DialogWidth / 2) -
        ( puGetDefaultLabelFont().getStringWidth( Label ) / 2);
	
    int nSliders = 1;
    int slider_x = 10;
    int slider_y = 55;
    int slider_width = 220;
    int slider_title_x = 15;
    int slider_value_x = 160;
    float slider_delta = 0.05f;

    puFont HUDalphaLegendFont;
    puFont HUDalphaLabelFont;
    puGetDefaultFonts ( &HUDalphaLegendFont, &HUDalphaLabelFont );
	
    HUDalphaDialog = new puDialogBox ( DialogX, DialogY ); {
        int horiz_slider_height = HUDalphaLabelFont.getStringHeight() +
            HUDalphaLabelFont.getStringDescender() +
            PUSTR_TGAP + PUSTR_BGAP + 5;

        /* puFrame *
           HUDalphaFrame = new puFrame ( 0, 0, DialogWidth,
           85 + nSliders
           * horiz_slider_height ); */

        puText *
            HUDalphaDialogMessage = new puText ( labelX,
                                                 52 + nSliders
                                                 * horiz_slider_height );
        HUDalphaDialogMessage -> setDefaultValue ( Label );
        HUDalphaDialogMessage -> getDefaultValue ( &s );
        HUDalphaDialogMessage -> setLabel        ( s );

        HUDalphaHS0 = new puSlider ( slider_x, slider_y,
                                     slider_width, HORIZONTAL ) ;
        HUDalphaHS0->     setDelta ( slider_delta ) ;
        HUDalphaHS0->     setValue ( hud_trans_alpha ) ;
        HUDalphaHS0->    setCBMode ( PUSLIDER_DELTA ) ;
        HUDalphaHS0->  setCallback ( alpha_adj ) ;

        puText *
            HUDalphaTitle =      new puText ( slider_title_x, slider_y ) ;
        HUDalphaTitle-> setDefaultValue ( "Alpha" ) ;
        HUDalphaTitle-> getDefaultValue ( &s ) ;
        HUDalphaTitle->        setLabel ( s ) ;
		
        HUDalphaText = new puText ( slider_value_x, slider_y ) ;
        sprintf( SliderText[ 0 ], "%05.2f", hud_trans_alpha );
        HUDalphaText-> setLabel ( SliderText[ 0 ] ) ;

        puOneShot *
            HUDalphaOkButton =     new puOneShot ( 10, 10, 60, 45 );
        HUDalphaOkButton->         setLegend ( gui_msg_OK );
        HUDalphaOkButton-> makeReturnDefault ( TRUE );
        HUDalphaOkButton->       setCallback ( goAwayHUDalphaAdjust );

        puOneShot *
            HUDalphaNoButton = new puOneShot ( 160, 10, 230, 45 );
        HUDalphaNoButton->     setLegend ( gui_msg_CANCEL );
        HUDalphaNoButton->   setCallback ( cancelHUDalphaAdjust );
    }
    FG_FINALIZE_PUI_DIALOG( HUDalphaDialog );

#undef HORIZONTAL
}


void fgHUDReshape(void) {
#if 0
    if ( HUDtext ) {
        // this chunk of code is not necessarily thread safe if the
        // compiler optimizer reorders these statements.  Note that
        // "delete ptr" does not set "ptr = NULL".  We have to do that
        // ourselves.
        fntRenderer *tmp = HUDtext;
        HUDtext = NULL;
        delete tmp;
    }

    HUD_TextSize = fgGetInt("/sim/startup/xsize") / 60;
    HUD_TextSize = 10;
    HUDtext = new fntRenderer();
    HUDtext -> setFont      ( guiFntHandle ) ;
    HUDtext -> setPointSize ( HUD_TextSize ) ;
    HUD_TextList.setFont( HUDtext );
#endif
}


static void set_hud_color(float r, float g, float b) {
    fgGetBool("/sim/hud/antialiased") ?
        glColor4f(r,g,b,hud_trans_alpha) :
        glColor3f(r,g,b);
}


// fgUpdateHUD
//
// Performs a once around the list of calls to instruments installed in
// the HUD object with requests for redraw. Kinda. It will when this is
// all C++.
//
void fgUpdateHUD( void ) {
	
    static const SGPropertyNode *enable3d_node = fgGetNode("/sim/hud/enable3d");
    if( HUD_style == 1 && enable3d_node->getBoolValue() )
    {
        fgUpdateHUDVirtual();
        return;
    }
    
    static const float normal_aspect = float(640) / float(480);
    // note: aspect_ratio is Y/X
    float current_aspect = 1.0f/globals->get_current_view()->get_aspect_ratio();
    if( current_aspect > normal_aspect ) {
        float aspect_adjust = current_aspect / normal_aspect;
        float adjust = 320.0f*aspect_adjust - 320.0f;
        fgUpdateHUD( -adjust, 0.0f, 640.0f+adjust, 480.0f );
    } else {
        float aspect_adjust = normal_aspect / current_aspect;
        float adjust = 240.0f*aspect_adjust - 240.0f;
        fgUpdateHUD( 0.0f, -adjust, 640.0f, 480.0f+adjust );
    }
}

void fgUpdateHUDVirtual()
{
    FGViewer* view = globals->get_current_view();

    // Standard fgfs projection, with essentially meaningless clip
    // planes (we'll map the whole HUD plane to z=-1)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(view->get_v_fov(), 1/view->get_aspect_ratio(), 0.1, 10);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
  
    // Standard fgfs view direction computation
    float lookat[3];
    lookat[0] = -sin(SG_DEGREES_TO_RADIANS * view->getHeadingOffset_deg());
    lookat[1] = tan(SG_DEGREES_TO_RADIANS * view->getPitchOffset_deg());
    lookat[2] = -cos(SG_DEGREES_TO_RADIANS * view->getHeadingOffset_deg());
    if(fabs(lookat[1]) > 9999) lookat[1] = 9999; // FPU sanity
    gluLookAt(0, 0, 0, lookat[0], lookat[1], lookat[2], 0, 1, 0);

    // Map the -1:1 square to a 55.0x41.25 degree wide patch at z=1.
    // This is the default fgfs field of view, which the HUD files are
    // written to assume.
    float dx = 0.52056705; // tan(55/2)
    float dy = dx * 0.75;  // assumes 4:3 aspect ratio
    float m[16];
    m[0] = dx; m[4] =  0; m[ 8] = 0; m[12] = 0;
    m[1] =  0; m[5] = dy; m[ 9] = 0; m[13] = 0;
    m[2] =  0; m[6] =  0; m[10] = 1; m[14] = 0;
    m[3] =  0; m[7] =  0; m[11] = 0; m[15] = 1;
    glMultMatrixf(m);

    // Convert the 640x480 "HUD standard" coordinate space to a square
    // about the origin in the range [-1:1] at depth of -1
    glScalef(1./320, 1./240, 1);
    glTranslatef(-320, -240, -1);

    // Do the deed
    drawHUD();

    // Clean up our mess
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void fgUpdateHUD( GLfloat x_start, GLfloat y_start,
                  GLfloat x_end, GLfloat y_end )
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(x_start, x_end, y_start, y_end);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    drawHUD();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void drawHUD()
{
    if( !HUD_deque.size() ) {  // Trust everyone, but ALWAYS cut the cards!
        return;
    }

    HUD_TextList.erase();
    HUD_LineList.erase();
    // HUD_StippleLineList.erase();
  
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    static const SGPropertyNode *antialiased_node
        = fgGetNode("/sim/hud/antialiased", true);
    static const SGPropertyNode *heading_enabled
        = fgGetNode("/autopilot/locks/heading", true);
    static const SGPropertyNode *altitude_enabled
        = fgGetNode("/autopilot/locks/altitude", true);

    static char hud_hdg_text[256];
    static char hud_wp0_text[256];
    static char hud_wp1_text[256];
    static char hud_wp2_text[256];
    static char hud_alt_text[256];

    if( antialiased_node->getBoolValue() ) {
        glEnable(GL_LINE_SMOOTH);
        //	  glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
        glLineWidth(2.0);
    } else {
        glLineWidth(1.0);
    }

    if( global_day_night_switch == HUD_DAY) {
        switch (HUD_deque[0]->get_brightness())
            {
            case HUD_BRT_LIGHT:
                set_hud_color (0.1f, 0.9f, 0.1f);
                break;

            case HUD_BRT_MEDIUM:
                set_hud_color (0.1f, 0.7f, 0.0f);
                break;

            case HUD_BRT_DARK:
                set_hud_color (0.0f, 0.6f, 0.0f);
                break;

            case HUD_BRT_BLACK:
                set_hud_color( 0.0f, 0.0f, 0.0f);
                break;

            default:
                set_hud_color (0.1f, 0.9f, 0.1f);
            }
    } else {
        if( global_day_night_switch == HUD_NIGHT) {
            switch (HUD_deque[0]->get_brightness())
                {
                case HUD_BRT_LIGHT:
                    set_hud_color (0.9f, 0.1f, 0.1f);
                    break;

                case HUD_BRT_MEDIUM:
                    set_hud_color (0.7f, 0.0f, 0.1f);
                    break;

                case HUD_BRT_DARK:
                    set_hud_color (0.6f, 0.0f, 0.0f);
                    break;

                case HUD_BRT_BLACK:
                    set_hud_color( 0.0f, 0.0f, 0.0f);
                    break;

                default:
                    set_hud_color (0.6f, 0.0f, 0.0f);
                }
        } else {     // Just in case default
            set_hud_color (0.1f, 0.9f, 0.1f);
        }
    }

    for_each(HUD_deque.begin(), HUD_deque.end(), HUDdraw());

    HUD_TextList.add( fgText(40, 10, get_formated_gmt_time(), 0) );


    int apY = 480 - 80;
    
    
    if (strcmp( heading_enabled->getStringValue(), "dg-heading-hold") == 0 ) {
        snprintf( hud_hdg_text, 256, "hdg = %.1f\n",
                  fgGetDouble("/autopilot/settings/heading-bug-deg") );
        HUD_TextList.add( fgText( 40, apY, hud_hdg_text ) );
        apY -= 15;
    } else if ( strcmp(heading_enabled->getStringValue(), "true-heading-hold") == 0 ) {
        snprintf( hud_hdg_text, 256, "hdg = %.1f\n",
                  fgGetDouble("/autopilot/settings/true-heading-deg") );
        HUD_TextList.add( fgText( 40, apY, hud_hdg_text ) );
        apY -= 15;

        string wp0_id = fgGetString( "/autopilot/route-manager/wp[0]/id" );
        if ( wp0_id.length() > 0 ) {
            snprintf( hud_wp0_text, 256, "%5s %6.1fnm %s", wp0_id.c_str(), 
                      fgGetDouble( "/autopilot/route-manager/wp[0]/dist" ),
                      fgGetString( "/autopilot/route-manager/wp[0]/eta" ) );
            HUD_TextList.add( fgText( 40, apY, hud_wp0_text ) );
            apY -= 15;
        }
        string wp1_id = fgGetString( "/autopilot/route-manager/wp[1]/id" );
        if ( wp1_id.length() > 0 ) {
            snprintf( hud_wp1_text, 256, "%5s %6.1fnm %s", wp1_id.c_str(), 
                      fgGetDouble( "/autopilot/route-manager/wp[1]/dist" ),
                      fgGetString( "/autopilot/route-manager/wp[1]/eta" ) );
            HUD_TextList.add( fgText( 40, apY, hud_wp1_text ) );
            apY -= 15;
        }
        string wp2_id = fgGetString( "/autopilot/route-manager/wp-last/id" );
        if ( wp2_id.length() > 0 ) {
            snprintf( hud_wp2_text, 256, "%5s %6.1fnm %s", wp2_id.c_str(), 
                      fgGetDouble( "/autopilot/route-manager/wp-last/dist" ),
                      fgGetString( "/autopilot/route-manager/wp-last/eta" ) );
            HUD_TextList.add( fgText( 40, apY, hud_wp2_text ) );
            apY -= 15;
        }
    }
  
    if ( strcmp( altitude_enabled->getStringValue(), "altitude-hold" ) == 0 ) {
        snprintf( hud_alt_text, 256, "alt = %.0f\n",
                  fgGetDouble("/autopilot/settings/target-altitude-ft") );
        HUD_TextList.add( fgText( 40, apY, hud_alt_text ) );
        apY -= 15;
    } else if ( strcmp( altitude_enabled->getStringValue(), "agl-hold" ) == 0 ){
        snprintf( hud_alt_text, 256, "agl = %.0f\n",
                  fgGetDouble("/autopilot/settings/target-agl-ft") );
        HUD_TextList.add( fgText( 40, apY, hud_alt_text ) );
        apY -= 15;
    }

    HUD_TextList.draw();

    HUD_LineList.draw();

    // glEnable(GL_LINE_STIPPLE);
    // glLineStipple( 1, 0x00FF );
    // HUD_StippleLineList.draw();
    // glDisable(GL_LINE_STIPPLE);

    if( antialiased_node->getBoolValue() ) {
        // glDisable(GL_BLEND);
        glDisable(GL_LINE_SMOOTH);
        glLineWidth(1.0);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

