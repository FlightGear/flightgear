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

#include <GL/glut.h>
#include <stdlib.h>
#include STL_STRING
#include STL_FSTREAM

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/props.hxx>
//#include <simgear/math/fg_random.h>
//#include <simgear/math/polar3d.hxx>

#include <Aircraft/aircraft.hxx>
#include <Autopilot/newauto.hxx>
#include <GUI/gui.h>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#ifdef FG_NETWORK_OLK
#include <NetworkOLK/network.h>
#endif
#include <Scenery/scenery.hxx>
//#include <Time/fg_timer.hxx>

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

#ifdef OLD_CODE
void drawOneLine( UINT x1, UINT y1, UINT x2, UINT y2)
{
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
}

void drawOneLine( RECT &rect)
{
  glBegin(GL_LINES);
  glVertex2f(rect.left, rect.top);
  glVertex2f(rect.right, rect.bottom);
  glEnd();
}

//
// The following code deals with painting the "instrument" on the display
//
   /* textString - Bitmap font string */

void textString( int x, int y, char *msg, void *font ){

    if(*msg)
    {
//      puDrawString (  NULL, msg, x, y );
        glRasterPos2f(x, y);
        while (*msg) {
            glutBitmapCharacter(font, *msg);
            msg++;
        }
    }
}


/* strokeString - Stroke font string */
void strokeString(int x, int y, char *msg, void *font, float theta)
{
    int xx;
    int yy;
    int c;
    float sintheta,costheta;
    

    if(*msg)
    {
    glPushMatrix();
    glRotatef(theta * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0);
    sintheta = sin(theta);
    costheta = cos(theta);
    xx = (int)(x * costheta + y * sintheta);
    yy = (int)(y * costheta - x * sintheta);
    glTranslatef( xx, yy, 0);
    glScalef(.1, .1, 0.0);
    while( (c=*msg++) ) {
        glutStrokeCharacter(font, c);
    }
    glPopMatrix();
    }
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
#endif // OLD_CODE

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

				SG_LOG(SG_INPUT, SG_INFO, "Done reading instrument " << name);
	
				
				p = (instr_item *) new HudLadder( name, x, y,
                                        width, height, factor,
										get_roll, get_pitch,
										span_units, division_units, minor_division,
										screen_hole, lbl_pos, frl_spot, target, vel_vector, 
										drift, alpha, energy, climb_dive, 
										glide, glide_slope_val, worm_energy, 
										waypoint, working);
				
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
				working			= node->getBoolValue("working");


				SG_LOG(SG_INPUT, SG_INFO, "Done reading instrument " << name);


				if(type=="guage")
					span_units = maxValue - minValue;

				if(loadfn=="anzg")
					load_fn = get_anzg;
				else				
				if(loadfn=="heading")
					load_fn = get_heading;
				else
				if(loadfn=="aoa")
					load_fn = get_aoa;
				else
				if(loadfn=="climb")
					load_fn = get_climb_rate;
				else
				if(loadfn=="altitude")
					load_fn = get_altitude;
				else
				if(loadfn=="agl")
					load_fn = get_agl;
				else
				if(loadfn=="speed")
					load_fn = get_speed;
				else
				if(loadfn=="view_direction")
					load_fn = get_view_direction;
				else
				if(loadfn=="aileronval")
					load_fn = get_aileronval;
				else
				if(loadfn=="elevatorval")
					load_fn = get_elevatorval;
				else
				if(loadfn=="rudderval")
					load_fn = get_rudderval;
				else
				if(loadfn=="throttleval")
					load_fn = get_throttleval;


				p = (instr_item *) new hud_card(	x,
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
														working);
					return p;
}// end readCard

static instr_item *
readLabel(const SGPropertyNode * node)
{
	instr_item *p;

	int font_size = (fgGetInt("/sim/startup/xsize") > 1000) ? LARGE : SMALL;

				name				= node->getStringValue("name");
				x                   = node->getIntValue("x");
				y                   = node->getIntValue("y");
				width               = node->getIntValue("width");
				height				= node->getIntValue("height");
				loadfn				= node->getStringValue("data_source");
				label_format		= node->getStringValue("label_format");
				prelabel			= node->getStringValue("pre_label_string");
				postlabel			= node->getStringValue("post_label_string");
				scaling				= node->getFloatValue("scale_data");
				options				= node->getIntValue("options");
				justi				= node->getIntValue("justification");
				blinking            = node->getIntValue("blinking");
				latitude			= node->getBoolValue("latitude",false);
				longitude			= node->getBoolValue("longitude",false);
				working             = node->getBoolValue("working");


				SG_LOG(SG_INPUT, SG_INFO, "Done reading instrument " << name);


				if(justi==0)
					justification = LEFT_JUST;
				else
				if(justi==1)
					justification = CENTER_JUST;
				else
				if(justi==2)
					justification = RIGHT_JUST;


				if(prelabel=="NULL")
					pre_label_string = NULL;
				else
				if(prelabel=="blank")
					pre_label_string = " ";
				else
					pre_label_string = prelabel.c_str();


				if(postlabel=="blank")
					post_label_string = " ";
				else
				if(postlabel=="NULL")
					post_label_string = NULL;
				else
				if(postlabel=="units")
					post_label_string = units;
				else
					post_label_string = postlabel.c_str();
 

				if(loadfn=="aux16")
					load_fn = get_aux16;
				else
				if(loadfn=="aux17")
					load_fn = get_aux17;
				else
				if(loadfn=="aux9")
					load_fn = get_aux9;
				else
				if(loadfn=="aux11")
					load_fn = get_aux11;
				else
				if(loadfn=="aux12")
					load_fn = get_aux12;
				else
				if(loadfn=="aux10")
					load_fn = get_aux10;
				else
				if(loadfn=="aux13")
					load_fn = get_aux13;
				else
				if(loadfn=="aux14")
					load_fn = get_aux14;
				else
				if(loadfn=="aux15")
					load_fn = get_aux15;
				else
				if(loadfn=="aux8")
					load_fn = get_aux8;
				else
				if(loadfn=="ax")
					load_fn = get_Ax;
				else
				if(loadfn=="speed")
					load_fn = get_speed;
				else
				if(loadfn=="mach")
					load_fn = get_mach;
				else
				if(loadfn=="altitude")
					load_fn = get_altitude;
				else
				if(loadfn=="agl")
					load_fn = get_agl;
				else
				if(loadfn=="framerate")
					load_fn = get_frame_rate;
				else
				if(loadfn=="heading")
					load_fn = get_heading;
				else
				if(loadfn=="fov")
					load_fn = get_fov;
				else
				if(loadfn=="vfc_tris_culled")
					load_fn = get_vfc_tris_culled;
				else
				if(loadfn=="vfc_tris_drawn")
					load_fn = get_vfc_tris_drawn;
				else
				if(loadfn=="aoa")
					load_fn = get_aoa;
				else
				if(loadfn=="latitude")
					load_fn  = get_latitude;
				else
				if(loadfn=="longitude")
					load_fn   = get_longitude;


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
												 working);

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
														working);

				return p;
} //end readTBI


int readInstrument(const SGPropertyNode * node)
{

	instr_item *HIptr;
    
    if ( fgGetString("/sim/startup/units") == "feet" ) {
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


	if (!readProperties(input, &root)) {
		SG_LOG(SG_INPUT, SG_ALERT, "Malformed property list for hud.");
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
		if (readProperties(path.str(), &root2)) {

			readInstrument(&root2);

		}//if
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
	if (!input.good()) 
		{
			SG_LOG(SG_INPUT, SG_ALERT,
			"Cannot read Hud configuration from " << path.str());
		} 
	else 
		{
			readHud(input);
			input.close();
		}

	fgHUDalphaInit();
	fgHUDReshape();

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
	} 
	else {
		readHud(input);
		input.close();
	}

    return 0;  // For now. Later we may use this for an error code.

}
//$$$ End - added, Neetha, 28 Nov 2k  

static int global_day_night_switch = DAY;

void HUD_masterswitch( bool incr )
{
    if ( fgGetBool("/sim/hud/visibility") ) {
	if ( global_day_night_switch == DAY ) {
	    global_day_night_switch = NIGHT;
	} else {
	    fgSetBool("/sim/hud/visibility", false);
	}
    } else {
        fgSetBool("/sim/hud/visibility", true);
	global_day_night_switch = DAY;
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
		case BRT_LIGHT:
		    brightness = BRT_BLACK;
		    break;

		case BRT_MEDIUM:
		    brightness = BRT_LIGHT;
		    break;

		case BRT_DARK:
		    brightness = BRT_MEDIUM;
		    break;

		case BRT_BLACK:
		    brightness = BRT_DARK;
		    break;

		default:
		    brightness = BRT_BLACK;
		}
	} else {
	    switch (brightness)
		{
		case BRT_LIGHT:
		    brightness = BRT_MEDIUM;
		    break;

		case BRT_MEDIUM:
		    brightness = BRT_DARK;
		    break;

		case BRT_DARK:
		    brightness = BRT_BLACK;
		    break;

		case BRT_BLACK:
		    brightness = BRT_LIGHT;
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
	//    printf ( "maxroll_adj( %p ) %f %f\n", hs, val, MaxRollAdjust * val ) ;
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
				 (puGetStringWidth( puGetDefaultLabelFont(), Label ) / 2);
	
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
		int horiz_slider_height = puGetStringHeight (HUDalphaLabelFont) +
								  puGetStringDescender (HUDalphaLabelFont) +
								  PUSTR_TGAP + PUSTR_BGAP + 5;

		puFrame *
		HUDalphaFrame = new puFrame ( 0, 0,
									  DialogWidth,
									  85 + nSliders * horiz_slider_height );
		
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
	if ( HUDtext )
		delete HUDtext;

	HUD_TextSize = fgGetInt("/sim/startup/xsize") / 60;
        HUD_TextSize = 10;
	HUDtext = new fntRenderer();
	HUDtext -> setFont      ( guiFntHandle ) ;
	HUDtext -> setPointSize ( HUD_TextSize ) ;
	HUD_TextList.setFont( HUDtext );
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
  int brightness;
//  int day_night_sw = current_aircraft.controls->day_night_switch;
  int day_night_sw = global_day_night_switch;
  int hud_displays = HUD_deque.size();
  instr_item *pHUDInstr;
  float line_width;

  if( !hud_displays ) {  // Trust everyone, but ALWAYS cut the cards!
    return;
    }

  HUD_TextList.erase();
  HUD_LineList.erase();
//  HUD_StippleLineList.erase();
  
  pHUDInstr = HUD_deque[0];
  brightness = pHUDInstr->get_brightness();
//  brightness = HUD_deque.at(0)->get_brightness();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glLoadIdentity();
  gluOrtho2D(0, 640, 0, 480);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  if( fgGetBool("/sim/hud/antialiased") ) {
	  glEnable(GL_LINE_SMOOTH);
//	  glEnable(GL_BLEND);
	  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	  glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
	  glLineWidth(1.5);
  } else {
	  glLineWidth(1.0);
  }

  if( day_night_sw == DAY) {
      switch (brightness)
	  {
	  case BRT_LIGHT:
	      set_hud_color (0.1f, 0.9f, 0.1f);
	      break;

          case BRT_MEDIUM:
	      set_hud_color (0.1f, 0.7f, 0.0f);
	      break;

          case BRT_DARK:
	      set_hud_color (0.0f, 0.6f, 0.0f);
	      break;

          case BRT_BLACK:
	      set_hud_color( 0.0f, 0.0f, 0.0f);
	      break;

          default:
	      set_hud_color (0.1f, 0.9f, 0.1f);
	  }
  } else {
      if( day_night_sw == NIGHT) {
	  switch (brightness)
	      {
	      case BRT_LIGHT:
		  set_hud_color (0.9f, 0.1f, 0.1f);
		  break;

              case BRT_MEDIUM:
		  set_hud_color (0.7f, 0.0f, 0.1f);
		  break;

	      case BRT_DARK:
		  set_hud_color (0.6f, 0.0f, 0.0f);
		  break;

	      case BRT_BLACK:
		  set_hud_color( 0.0f, 0.0f, 0.0f);
		  break;

	      default:
		  set_hud_color (0.6f, 0.0f, 0.0f);
	      }
      } else {     // Just in case default
	  set_hud_color (0.1f, 0.9f, 0.1f);
      }
  }

  deque < instr_item * > :: iterator current = HUD_deque.begin();
  deque < instr_item * > :: iterator last = HUD_deque.end();

  for ( ; current != last; ++current ) {
	  pHUDInstr = *current;

	  if( pHUDInstr->enabled()) {
		  //  fgPrintf( SG_COCKPIT, SG_DEBUG, "HUD Code %d  Status %d\n",
		  //            hud->code, hud->status );
		  pHUDInstr->draw();
		  
	  }
  }

  char *gmt_str = get_formated_gmt_time();
  HUD_TextList.add( fgText(40, 10, gmt_str) );

#ifdef FG_NETWORK_OLK
  if ( net_hud_display ) {
      net_hud_update();
  }
#endif


  // temporary
  // extern bool fgAPAltitudeEnabled( void );
  // extern bool fgAPHeadingEnabled( void );
  // extern bool fgAPWayPointEnabled( void );
  // extern char *fgAPget_TargetDistanceStr( void );
  // extern char *fgAPget_TargetHeadingStr( void );
  // extern char *fgAPget_TargetAltitudeStr( void );
  // extern char *fgAPget_TargetLatLonStr( void );

  int apY = 480 - 80;
//  char scratch[128];
//  HUD_TextList.add( fgText( "AUTOPILOT", 20, apY) );
//  apY -= 15;
  if( current_autopilot->get_HeadingEnabled() ) {
      HUD_TextList.add( fgText( 40, apY, 
				current_autopilot->get_TargetHeadingStr()) );
      apY -= 15;
  }
  if( current_autopilot->get_AltitudeEnabled() ) {
      HUD_TextList.add( fgText( 40, apY, 
				current_autopilot->get_TargetAltitudeStr()) );
      apY -= 15;
  }
  if( current_autopilot->get_HeadingMode() == 
      FGAutopilot::FG_HEADING_WAYPOINT )
  {
      char *wpstr;
      wpstr = current_autopilot->get_TargetWP1Str();
      if ( strlen( wpstr ) ) {
	  HUD_TextList.add( fgText( 40, apY, wpstr ) );
	  apY -= 15;
      }
      wpstr = current_autopilot->get_TargetWP2Str();
      if ( strlen( wpstr ) ) {
	  HUD_TextList.add( fgText( 40, apY, wpstr ) );
	  apY -= 15;
      }
      wpstr = current_autopilot->get_TargetWP3Str();
      if ( strlen( wpstr ) ) {
	  HUD_TextList.add( fgText( 40, apY, wpstr ) );
	  apY -= 15;
      }
  }
  
  HUD_TextList.draw();

  HUD_LineList.draw();

//  glEnable(GL_LINE_STIPPLE);
//  glLineStipple( 1, 0x00FF );
//  HUD_StippleLineList.draw();
//  glDisable(GL_LINE_STIPPLE);

  if( fgGetBool("/sim/hud/antialiased") ) {
//	  glDisable(GL_BLEND);
	  glDisable(GL_LINE_SMOOTH);
	  glLineWidth(1.0);
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

