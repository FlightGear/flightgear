// autopilot.cxx -- autopilot subsystem
//
// Started by Jeff Goeke-Smith, started April 1998.
//
// Copyright (C) 1998  Jeff Goeke-Smith, jgoeke@voyager.net
//
// Heavy modifications and additions by Norman Vine and few by Curtis
// Olson
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

#include <assert.h>
#include <stdlib.h>

#include <Scenery/scenery.hxx>

#include "autopilot.hxx"

#include <Include/fg_constants.h>
#include <Debug/logstream.hxx>
#include <Airports/simple.hxx>
#include <GUI/gui.h>
#include <Main/fg_init.hxx>
#include <Main/options.hxx>
#include <Time/fg_time.hxx>


#define mySlider puSlider

/// These statics will eventually go into the class
/// they are just here while I am experimenting -- NHV :-)
// AutoPilot Gain Adjuster members
static double MaxRollAdjust;        // MaxRollAdjust       = 2 * APData->MaxRoll;
static double RollOutAdjust;        // RollOutAdjust       = 2 * APData->RollOut;
static double MaxAileronAdjust;     // MaxAileronAdjust    = 2 * APData->MaxAileron;
static double RollOutSmoothAdjust;  // RollOutSmoothAdjust = 2 * APData->RollOutSmooth;

static float MaxRollValue;          // 0.1 -> 1.0
static float RollOutValue;
static float MaxAileronValue;
static float RollOutSmoothValue;

static float TmpMaxRollValue;       // for cancel operation
static float TmpRollOutValue;
static float TmpMaxAileronValue;
static float TmpRollOutSmoothValue;

static puDialogBox *APAdjustDialog;
static puFrame     *APAdjustFrame;
static puText      *APAdjustDialogMessage;
static puFont      APAdjustLegendFont;
static puFont      APAdjustLabelFont;

static puOneShot *APAdjustOkButton;
static puOneShot *APAdjustResetButton;
static puOneShot *APAdjustCancelButton;

//static puButton        *APAdjustDragButton;

static puText *APAdjustMaxRollTitle;
static puText *APAdjustRollOutTitle;
static puText *APAdjustMaxAileronTitle;
static puText *APAdjustRollOutSmoothTitle;

static puText *APAdjustMaxAileronText;
static puText *APAdjustMaxRollText;
static puText *APAdjustRollOutText;
static puText *APAdjustRollOutSmoothText;

static mySlider *APAdjustHS0;
static mySlider *APAdjustHS1;
static mySlider *APAdjustHS2;
static mySlider *APAdjustHS3;

static char SliderText[ 4 ][ 8 ];

///////// AutoPilot New Heading Dialog

static puDialogBox     *ApHeadingDialog;
static puFrame         *ApHeadingDialogFrame;
static puText          *ApHeadingDialogMessage;
static puInput         *ApHeadingDialogInput;
static puOneShot       *ApHeadingDialogOkButton;
static puOneShot       *ApHeadingDialogCancelButton;


///////// AutoPilot New Altitude Dialog

static puDialogBox     *ApAltitudeDialog = 0;
static puFrame         *ApAltitudeDialogFrame = 0;
static puText          *ApAltitudeDialogMessage = 0;
static puInput         *ApAltitudeDialogInput = 0;

static puOneShot       *ApAltitudeDialogOkButton = 0;
static puOneShot       *ApAltitudeDialogCancelButton = 0;


/// The beginnings of Lock AutoPilot to target location :-)
//  Needs cleaning up but works
//  These statics should disapear when this is a class
static puDialogBox     *TgtAptDialog = 0;
static puFrame         *TgtAptDialogFrame = 0;
static puText          *TgtAptDialogMessage = 0;
static puInput         *TgtAptDialogInput = 0;

static char NewTgtAirportId[16];
static char NewTgtAirportLabel[] = "Enter New TgtAirport ID"; 

static puOneShot       *TgtAptDialogOkButton = 0;
static puOneShot       *TgtAptDialogCancelButton = 0;
static puOneShot       *TgtAptDialogResetButton = 0;


// global variable holding the AP info
// I want this gone.  Data should be in aircraft structure
fgAPDataPtr APDataGlobal;

// Local Prototype section
static double LinearExtrapolate( double x, double x1, double y1, double x2, double y2 );
static double NormalizeDegrees( double Input );
// End Local ProtoTypes

extern char *coord_format_lat(float);
extern char *coord_format_lon(float);
			

static inline double get_ground_speed( void ) {
    // starts in ft/s so we convert to kts
    double ft_s = cur_fdm_state->get_V_ground_speed() 
	* current_options.get_speed_up();;
    double kts = ft_s * FEET_TO_METER * 3600 * METER_TO_NM;
    return kts;
}

// The below routines were copied right from hud.c ( I hate reinventing
// the wheel more than necessary)

// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//
static void get_control_values( void ) {
    fgAPDataPtr APData;
    APData = APDataGlobal;
    APData->old_aileron = controls.get_aileron();
    APData->old_elevator = controls.get_elevator();
    APData->old_elevator_trim = controls.get_elevator_trim();
    APData->old_rudder = controls.get_rudder();
}

static void MakeTargetHeadingStr( fgAPDataPtr APData, double bearing ) {
    if( bearing < 0. )
	bearing += 360.;
    else if(bearing > 360. )
	bearing -= 360.;
    sprintf(APData->TargetHeadingStr, "APHeading  %6.1f", bearing);
}

static inline void MakeTargetDistanceStr( fgAPDataPtr APData, double distance ) {
    double eta = distance*METER_TO_NM / get_ground_speed();
    if ( eta >= 100.0 ) { eta = 99.999; }
    int major, minor;
    if ( eta < (1.0/6.0) ) {
	// within 10 minutes, bump up to min/secs
	eta *= 60.0;
    }
    major = (int)eta;
    minor = (int)((eta - (int)eta) * 60.0);
    sprintf(APData->TargetDistanceStr, "APDistance %.2f NM  ETA %d:%02d",
	    distance*METER_TO_NM, major, minor);
    // cout << "distance = " << distance*METER_TO_NM
    //      << "  gndsp = " << get_ground_speed() 
    //      << "  time = " << eta
    //      << "  major = " << major
    //      << "  minor = " << minor
    //      << endl;
}

static inline void MakeTargetAltitudeStr( fgAPDataPtr APData, double altitude ) {
    sprintf(APData->TargetAltitudeStr, "APAltitude  %6.0f", altitude);
}

static inline void MakeTargetLatLonStr( fgAPDataPtr APData, double lat, double lon ) {
    float tmp;
    tmp = APData->TargetLatitude;
    sprintf( APData->TargetLatitudeStr , "%s", coord_format_lat(tmp) );
    tmp = APData->TargetLongitude;
    sprintf( APData->TargetLongitudeStr, "%s", coord_format_lon(tmp) );
		
    sprintf(APData->TargetLatLonStr, "%s  %s",
	    APData->TargetLatitudeStr,
	    APData->TargetLongitudeStr );
}

static inline double get_speed( void ) {
    return( cur_fdm_state->get_V_equiv_kts() );
}

static inline double get_aoa( void ) {
    return( cur_fdm_state->get_Gamma_vert_rad() * RAD_TO_DEG );
}

static inline double fgAPget_latitude( void ) {
    return( cur_fdm_state->get_Latitude() * RAD_TO_DEG );
}

static inline double fgAPget_longitude( void ) {
    return( cur_fdm_state->get_Longitude() * RAD_TO_DEG );
}

static inline double fgAPget_roll( void ) {
    return( cur_fdm_state->get_Phi() * RAD_TO_DEG );
}

static inline double get_pitch( void ) {
    return( cur_fdm_state->get_Theta() );
}

double fgAPget_heading( void ) {
    return( cur_fdm_state->get_Psi() * RAD_TO_DEG );
}

static inline double fgAPget_altitude( void ) {
    return( cur_fdm_state->get_Altitude() * FEET_TO_METER );
}

static inline double fgAPget_climb( void ) {
    // return in meters per minute
    return( cur_fdm_state->get_Climb_Rate() * FEET_TO_METER * 60 );
}

static inline double get_sideslip( void ) {
    return( cur_fdm_state->get_Beta() );
}

static inline double fgAPget_agl( void ) {
    double agl;

    agl = cur_fdm_state->get_Altitude() * FEET_TO_METER
	- scenery.cur_elev;

    return( agl );
}

// End of copied section.  ( thanks for the wheel :-)
double fgAPget_TargetLatitude( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetLatitude;
}

double fgAPget_TargetLongitude( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetLongitude;
}

double fgAPget_TargetHeading( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetHeading;
}

double fgAPget_TargetDistance( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetDistance;
}

double fgAPget_TargetAltitude( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetAltitude;
}

char *fgAPget_TargetLatitudeStr( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetLatitudeStr;
}

char *fgAPget_TargetLongitudeStr( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetLongitudeStr;
}

char *fgAPget_TargetDistanceStr( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetDistanceStr;
}

char *fgAPget_TargetHeadingStr( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetHeadingStr;
}

char *fgAPget_TargetAltitudeStr( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetAltitudeStr;
}

char *fgAPget_TargetLatLonStr( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->TargetLatLonStr;
}

bool fgAPWayPointEnabled( void )
{
    fgAPDataPtr APData;

    APData = APDataGlobal;

    // heading hold enabled?
    return APData->waypoint_hold;
}


bool fgAPHeadingEnabled( void )
{
    fgAPDataPtr APData;

    APData = APDataGlobal;

    // heading hold enabled?
    return APData->heading_hold;
}

bool fgAPAltitudeEnabled( void )
{
    fgAPDataPtr APData;

    APData = APDataGlobal;

    // altitude hold or terrain follow enabled?
    return APData->altitude_hold;
}

bool fgAPTerrainFollowEnabled( void )
{
    fgAPDataPtr APData;

    APData = APDataGlobal;

    // altitude hold or terrain follow enabled?
    return APData->terrain_follow ;
}

bool fgAPAutoThrottleEnabled( void )
{
    fgAPDataPtr APData;

    APData = APDataGlobal;

    // autothrottle enabled?
    return APData->auto_throttle;
}

void fgAPAltitudeAdjust( double inc )
{
    // Remove at a later date
    fgAPDataPtr APData = APDataGlobal;
    // end section

    double target_alt, target_agl;

    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
	target_alt = APData->TargetAltitude * METER_TO_FEET;
	target_agl = APData->TargetAGL * METER_TO_FEET;
    } else {
	target_alt = APData->TargetAltitude;
	target_agl = APData->TargetAGL;
    }

    target_alt = ( int ) ( target_alt / inc ) * inc + inc;
    target_agl = ( int ) ( target_agl / inc ) * inc + inc;

    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
	target_alt *= FEET_TO_METER;
	target_agl *= FEET_TO_METER;
    }

    APData->TargetAltitude = target_alt;
    APData->TargetAGL = target_agl;
	
    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET )
	target_alt *= METER_TO_FEET;
    ApAltitudeDialogInput->setValue((float)target_alt);
    MakeTargetAltitudeStr( APData, target_alt);

    get_control_values();
}

void fgAPAltitudeSet( double new_altitude ) {
    // Remove at a later date
    fgAPDataPtr APData = APDataGlobal;
    // end section
    double target_alt = new_altitude;

    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET )
	target_alt = new_altitude * FEET_TO_METER;

    if( target_alt < scenery.cur_elev )
	target_alt = scenery.cur_elev;

    APData->TargetAltitude = target_alt;
	
    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET )
	target_alt *= METER_TO_FEET;
    ApAltitudeDialogInput->setValue((float)target_alt);
    MakeTargetAltitudeStr( APData, target_alt);
	
    get_control_values();
}

void fgAPHeadingAdjust( double inc ) {
    fgAPDataPtr APData = APDataGlobal;

    if ( APData->waypoint_hold )
	APData->waypoint_hold = false;
	
    double target = ( int ) ( APData->TargetHeading / inc ) * inc + inc;

    APData->TargetHeading = NormalizeDegrees( target );
    // following cast needed ambiguous plib
    ApHeadingDialogInput -> setValue ((float)APData->TargetHeading );
    MakeTargetHeadingStr( APData, APData->TargetHeading );			
    get_control_values();
}

void fgAPHeadingSet( double new_heading ) {
    fgAPDataPtr APData = APDataGlobal;
	
    if ( APData->waypoint_hold )
	APData->waypoint_hold = false;
	
    new_heading = NormalizeDegrees( new_heading );
    APData->TargetHeading = new_heading;
    // following cast needed ambiguous plib
    ApHeadingDialogInput -> setValue ((float)APData->TargetHeading );
    MakeTargetHeadingStr( APData, APData->TargetHeading );			
    get_control_values();
}

void fgAPAutoThrottleAdjust( double inc ) {
    fgAPDataPtr APData = APDataGlobal;

    double target = ( int ) ( APData->TargetSpeed / inc ) * inc + inc;

    APData->TargetSpeed = target;
}

// THIS NEEDS IMPROVEMENT !!!!!!!!!!!!!
static int scan_number(char *s, double *new_value)
{
    int ret = 0;
    char WordBuf[64];
    char *cptr = s;
    char *WordBufPtr = WordBuf;

    if (*cptr == '+')
	cptr++;
    if (*cptr == '-') {
	*WordBufPtr++ = *cptr++;
    }
    while (isdigit(*cptr) ) {
	*WordBufPtr++ = *cptr++;
	ret = 1;
    }
    if (*cptr == '.') 
	*WordBufPtr++ = *cptr++;  // put the '.' into the string
    while (isdigit(*cptr)) {
	*WordBufPtr++ = *cptr++;
	ret = 1;
    }
    if( ret == 1 ) {
	*WordBufPtr = '\0';
	sscanf(WordBuf, "%lf", new_value);
    }

    return(ret);
} // scan_number


void ApHeadingDialog_Cancel(puObject *)
{
    ApHeadingDialogInput->rejectInput();
    FG_POP_PUI_DIALOG( ApHeadingDialog );
}

void ApHeadingDialog_OK (puObject *me)
{
    int error = 0;
    char *c;
    string s;
    ApHeadingDialogInput -> getValue( &c );

    if( strlen(c) ) {
	double NewHeading;
	if( scan_number( c, &NewHeading ) )
	    {
		if(!fgAPHeadingEnabled())
		    fgAPToggleHeading();
		fgAPHeadingSet( NewHeading );
	    } else {
		error = 1;
		s = c;
		s += " is not a valid number.";
	    }
    }
    ApHeadingDialog_Cancel(me);
    if( error )  mkDialog(s.c_str());
}

void NewHeading(puObject *cb)
{
    //	string ApHeadingLabel( "Enter New Heading" );
    //	ApHeadingDialogMessage  -> setLabel(ApHeadingLabel.c_str());
    ApHeadingDialogInput    -> acceptInput();
    FG_PUSH_PUI_DIALOG( ApHeadingDialog );
}

void NewHeadingInit(void)
{
    //	printf("NewHeadingInit\n");
    char NewHeadingLabel[] = "Enter New Heading";
    char *s;

    float heading = fgAPget_heading();
    int len = 260/2 -
	(puGetStringWidth( puGetDefaultLabelFont(), NewHeadingLabel ) /2 );

    ApHeadingDialog = new puDialogBox (150, 50);
    {
	ApHeadingDialogFrame   = new puFrame (0, 0, 260, 150);

	ApHeadingDialogMessage = new puText   (len, 110);
	ApHeadingDialogMessage    -> setDefaultValue (NewHeadingLabel);
	ApHeadingDialogMessage    -> getDefaultValue (&s);
	ApHeadingDialogMessage    -> setLabel        (s);

	ApHeadingDialogInput   = new puInput  ( 50, 70, 210, 100 );
	ApHeadingDialogInput   ->    setValue ( heading );

	ApHeadingDialogOkButton     =  new puOneShot         (50, 10, 110, 50);
	ApHeadingDialogOkButton     ->     setLegend         (gui_msg_OK);
	ApHeadingDialogOkButton     ->     makeReturnDefault (TRUE);
	ApHeadingDialogOkButton     ->     setCallback       (ApHeadingDialog_OK);

	ApHeadingDialogCancelButton =  new puOneShot         (140, 10, 210, 50);
	ApHeadingDialogCancelButton ->     setLegend         (gui_msg_CANCEL);
	ApHeadingDialogCancelButton ->     setCallback       (ApHeadingDialog_Cancel);

    }
    FG_FINALIZE_PUI_DIALOG( ApHeadingDialog );
}

void ApAltitudeDialog_Cancel(puObject *)
{
    ApAltitudeDialogInput -> rejectInput();
    FG_POP_PUI_DIALOG( ApAltitudeDialog );
}

void ApAltitudeDialog_OK (puObject *me)
{
    int error = 0;
    string s;
    char *c;
    ApAltitudeDialogInput->getValue( &c );

    if( strlen( c ) ) {
	double NewAltitude;
	if( scan_number( c, &NewAltitude) )
	    {
		if(!(fgAPAltitudeEnabled()))
		    fgAPToggleAltitude();
		fgAPAltitudeSet( NewAltitude );
	    } else {
		error = 1;
		s = c;
		s += " is not a valid number.";
	    }
    }
    ApAltitudeDialog_Cancel(me);
    if( error )  mkDialog(s.c_str());
    get_control_values();
}

void NewAltitude(puObject *cb)
{
    ApAltitudeDialogInput -> acceptInput();
    FG_PUSH_PUI_DIALOG( ApAltitudeDialog );
}

void NewAltitudeInit(void)
{
    //	printf("NewAltitudeInit\n");
    char NewAltitudeLabel[] = "Enter New Altitude";
    char *s;

    float alt = cur_fdm_state->get_Altitude();

    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_METERS) {
	alt *= FEET_TO_METER;
    }

    int len = 260/2 -
	(puGetStringWidth( puGetDefaultLabelFont(), NewAltitudeLabel )/2);

    //	ApAltitudeDialog = new puDialogBox (150, 50);
    ApAltitudeDialog = new puDialogBox (150, 200);
    {
	ApAltitudeDialogFrame   = new puFrame  (0, 0, 260, 150);
	ApAltitudeDialogMessage = new puText   (len, 110);
	ApAltitudeDialogMessage    -> setDefaultValue (NewAltitudeLabel);
	ApAltitudeDialogMessage    -> getDefaultValue (&s);
	ApAltitudeDialogMessage    -> setLabel (s);

	ApAltitudeDialogInput   = new puInput  ( 50, 70, 210, 100 );
	ApAltitudeDialogInput      -> setValue ( alt );
	// Uncomment the next line to have input active on startup
	// ApAltitudeDialogInput   ->    acceptInput       ( );
	// cursor at begining or end of line ?
	//len = strlen(s);
	//		len = 0;
	//		ApAltitudeDialogInput   ->    setCursor         ( len );
	//		ApAltitudeDialogInput   ->    setSelectRegion   ( 5, 9 );

	ApAltitudeDialogOkButton     =  new puOneShot         (50, 10, 110, 50);
	ApAltitudeDialogOkButton     ->     setLegend         (gui_msg_OK);
	ApAltitudeDialogOkButton     ->     makeReturnDefault (TRUE);
	ApAltitudeDialogOkButton     ->     setCallback       (ApAltitudeDialog_OK);

	ApAltitudeDialogCancelButton =  new puOneShot         (140, 10, 210, 50);
	ApAltitudeDialogCancelButton ->     setLegend         (gui_msg_CANCEL);
	ApAltitudeDialogCancelButton ->     setCallback       (ApAltitudeDialog_Cancel);

    }
    FG_FINALIZE_PUI_DIALOG( ApAltitudeDialog );
}

/////// simple AutoPilot GAIN / LIMITS ADJUSTER

#define fgAP_CLAMP(val,min,max) \
( (val) = (val) > (max) ? (max) : (val) < (min) ? (min) : (val) )

    static void maxroll_adj( puObject *hs ) {
	float val ;
	fgAPDataPtr APData = APDataGlobal;

	hs-> getValue ( &val ) ;
	fgAP_CLAMP ( val, 0.1, 1.0 ) ;
	//    printf ( "maxroll_adj( %p ) %f %f\n", hs, val, MaxRollAdjust * val ) ;
	APData-> MaxRoll = MaxRollAdjust * val;
	sprintf( SliderText[ 0 ], "%05.2f", APData->MaxRoll );
	APAdjustMaxRollText -> setLabel ( SliderText[ 0 ] ) ;
}

static void rollout_adj( puObject *hs ) {
    float val ;
    fgAPDataPtr APData = APDataGlobal;

    hs-> getValue ( &val ) ;
    fgAP_CLAMP ( val, 0.1, 1.0 ) ;
    //    printf ( "rollout_adj( %p ) %f %f\n", hs, val, RollOutAdjust * val ) ;
    APData-> RollOut = RollOutAdjust * val;
    sprintf( SliderText[ 1 ], "%05.2f", APData->RollOut );
    APAdjustRollOutText -> setLabel ( SliderText[ 1 ] );
}

static void maxaileron_adj( puObject *hs ) {
    float val ;
    fgAPDataPtr APData;
    APData = APDataGlobal;

    hs-> getValue ( &val ) ;
    fgAP_CLAMP ( val, 0.1, 1.0 ) ;
    //    printf ( "maxaileron_adj( %p ) %f %f\n", hs, val, MaxAileronAdjust * val ) ;
    APData-> MaxAileron = MaxAileronAdjust * val;
    sprintf( SliderText[ 3 ], "%05.2f", APData->MaxAileron );
    APAdjustMaxAileronText -> setLabel ( SliderText[ 3 ] );
}

static void rolloutsmooth_adj( puObject *hs ) {
    float val ;
    fgAPDataPtr APData = APDataGlobal;

    hs -> getValue ( &val ) ;
    fgAP_CLAMP ( val, 0.1, 1.0 ) ;
    //    printf ( "rolloutsmooth_adj( %p ) %f %f\n", hs, val, RollOutSmoothAdjust * val ) ;
    APData->RollOutSmooth = RollOutSmoothAdjust * val;
    sprintf( SliderText[ 2 ], "%5.2f", APData-> RollOutSmooth );
    APAdjustRollOutSmoothText-> setLabel ( SliderText[ 2 ] );

}

static void goAwayAPAdjust (puObject *)
{
    FG_POP_PUI_DIALOG( APAdjustDialog );
}

void cancelAPAdjust( puObject *self ) {
    fgAPDataPtr APData = APDataGlobal;

    APData-> MaxRoll       = TmpMaxRollValue;
    APData-> RollOut       = TmpRollOutValue;
    APData-> MaxAileron    = TmpMaxAileronValue;
    APData-> RollOutSmooth = TmpRollOutSmoothValue;

    goAwayAPAdjust(self);
}

void resetAPAdjust( puObject *self ) {
    fgAPDataPtr APData = APDataGlobal;

    APData-> MaxRoll       = MaxRollAdjust / 2;
    APData-> RollOut       = RollOutAdjust / 2;
    APData-> MaxAileron    = MaxAileronAdjust / 2;
    APData-> RollOutSmooth = RollOutSmoothAdjust / 2;

    FG_POP_PUI_DIALOG( APAdjustDialog );

    fgAPAdjust( self );
}

void fgAPAdjust( puObject * ) {
    fgAPDataPtr APData = APDataGlobal;

    TmpMaxRollValue       = APData-> MaxRoll;
    TmpRollOutValue       = APData-> RollOut;
    TmpMaxAileronValue    = APData-> MaxAileron;
    TmpRollOutSmoothValue = APData-> RollOutSmooth;

    MaxRollValue       = APData-> MaxRoll / MaxRollAdjust;
    RollOutValue       = APData-> RollOut / RollOutAdjust;
    MaxAileronValue    = APData-> MaxAileron / MaxAileronAdjust;
    RollOutSmoothValue = APData-> RollOutSmooth / RollOutSmoothAdjust;

    APAdjustHS0-> setValue ( MaxRollValue ) ;
    APAdjustHS1-> setValue ( RollOutValue ) ;
    APAdjustHS2-> setValue ( RollOutSmoothValue ) ;
    APAdjustHS3-> setValue ( MaxAileronValue ) ;

    FG_PUSH_PUI_DIALOG( APAdjustDialog );
}

// Done once at system initialization
void fgAPAdjustInit( void ) {

    //	printf("fgAPAdjustInit\n");
#define HORIZONTAL  FALSE

    int DialogX = 40;
    int DialogY = 100;
    int DialogWidth = 230;

    char Label[] =  "AutoPilot Adjust";
    char *s;

    fgAPDataPtr APData = APDataGlobal;

    int labelX = (DialogWidth / 2) -
	(puGetStringWidth( puGetDefaultLabelFont(), Label ) / 2);
    labelX -= 30;  // KLUDGEY

    int nSliders = 4;
    int slider_x = 10;
    int slider_y = 55;
    int slider_width = 210;
    int slider_title_x = 15;
    int slider_value_x = 160;
    float slider_delta = 0.1f;

    TmpMaxRollValue       = APData-> MaxRoll;
    TmpRollOutValue       = APData-> RollOut;
    TmpMaxAileronValue    = APData-> MaxAileron;
    TmpRollOutSmoothValue = APData-> RollOutSmooth;

    MaxRollValue       = APData-> MaxRoll / MaxRollAdjust;
    RollOutValue       = APData-> RollOut / RollOutAdjust;
    MaxAileronValue    = APData-> MaxAileron / MaxAileronAdjust;
    RollOutSmoothValue = APData-> RollOutSmooth / RollOutSmoothAdjust;

    puGetDefaultFonts (  &APAdjustLegendFont,  &APAdjustLabelFont );
    APAdjustDialog = new puDialogBox ( DialogX, DialogY ); {
	int horiz_slider_height = puGetStringHeight (APAdjustLabelFont) +
	    puGetStringDescender (APAdjustLabelFont) +
	    PUSTR_TGAP + PUSTR_BGAP + 5;

	APAdjustFrame = new puFrame ( 0, 0,
				      DialogWidth, 85 + nSliders * horiz_slider_height );

	APAdjustDialogMessage = new puText ( labelX, 52 + nSliders * horiz_slider_height );
	APAdjustDialogMessage -> setDefaultValue ( Label );
	APAdjustDialogMessage -> getDefaultValue ( &s );
	APAdjustDialogMessage -> setLabel        ( s );

	APAdjustHS0 = new mySlider ( slider_x, slider_y, slider_width, HORIZONTAL ) ;
	APAdjustHS0-> setDelta ( slider_delta ) ;
	APAdjustHS0-> setValue ( MaxRollValue ) ;
	APAdjustHS0-> setCBMode ( PUSLIDER_DELTA ) ;
	APAdjustHS0-> setCallback ( maxroll_adj ) ;

	sprintf( SliderText[ 0 ], "%05.2f", APData->MaxRoll );
	APAdjustMaxRollTitle = new puText ( slider_title_x, slider_y ) ;
	APAdjustMaxRollTitle-> setDefaultValue ( "MaxRoll" ) ;
	APAdjustMaxRollTitle-> getDefaultValue ( &s ) ;
	APAdjustMaxRollTitle-> setLabel ( s ) ;
	APAdjustMaxRollText = new puText ( slider_value_x, slider_y ) ;
	APAdjustMaxRollText-> setLabel ( SliderText[ 0 ] ) ;

	slider_y += horiz_slider_height;

	APAdjustHS1 = new mySlider ( slider_x, slider_y, slider_width, HORIZONTAL ) ;
	APAdjustHS1-> setDelta ( slider_delta ) ;
	APAdjustHS1-> setValue ( RollOutValue ) ;
	APAdjustHS1-> setCBMode ( PUSLIDER_DELTA ) ;
	APAdjustHS1-> setCallback ( rollout_adj ) ;

	sprintf( SliderText[ 1 ], "%05.2f", APData->RollOut );
	APAdjustRollOutTitle = new puText ( slider_title_x, slider_y ) ;
	APAdjustRollOutTitle-> setDefaultValue ( "AdjustRollOut" ) ;
	APAdjustRollOutTitle-> getDefaultValue ( &s ) ;
	APAdjustRollOutTitle-> setLabel ( s ) ;
	APAdjustRollOutText = new puText ( slider_value_x, slider_y ) ;
	APAdjustRollOutText-> setLabel ( SliderText[ 1 ] );

	slider_y += horiz_slider_height;

	APAdjustHS2 = new mySlider ( slider_x, slider_y, slider_width, HORIZONTAL ) ;
	APAdjustHS2-> setDelta ( slider_delta ) ;
	APAdjustHS2-> setValue ( RollOutSmoothValue ) ;
	APAdjustHS2-> setCBMode ( PUSLIDER_DELTA ) ;
	APAdjustHS2-> setCallback ( rolloutsmooth_adj ) ;

	sprintf( SliderText[ 2 ], "%5.2f", APData->RollOutSmooth );
	APAdjustRollOutSmoothTitle = new puText ( slider_title_x, slider_y ) ;
	APAdjustRollOutSmoothTitle-> setDefaultValue ( "RollOutSmooth" ) ;
	APAdjustRollOutSmoothTitle-> getDefaultValue ( &s ) ;
	APAdjustRollOutSmoothTitle-> setLabel ( s ) ;
	APAdjustRollOutSmoothText = new puText ( slider_value_x, slider_y ) ;
	APAdjustRollOutSmoothText-> setLabel ( SliderText[ 2 ] );

	slider_y += horiz_slider_height;

	APAdjustHS3 = new mySlider ( slider_x, slider_y, slider_width, HORIZONTAL ) ;
	APAdjustHS3-> setDelta ( slider_delta ) ;
	APAdjustHS3-> setValue ( MaxAileronValue ) ;
	APAdjustHS3-> setCBMode ( PUSLIDER_DELTA ) ;
	APAdjustHS3-> setCallback ( maxaileron_adj ) ;

	sprintf( SliderText[ 3 ], "%05.2f", APData->MaxAileron );
	APAdjustMaxAileronTitle = new puText ( slider_title_x, slider_y ) ;
	APAdjustMaxAileronTitle-> setDefaultValue ( "MaxAileron" ) ;
	APAdjustMaxAileronTitle-> getDefaultValue ( &s ) ;
	APAdjustMaxAileronTitle-> setLabel ( s ) ;
	APAdjustMaxAileronText = new puText ( slider_value_x, slider_y ) ;
	APAdjustMaxAileronText-> setLabel ( SliderText[ 3 ] );

	APAdjustOkButton = new puOneShot ( 10, 10, 60, 50 );
	APAdjustOkButton-> setLegend ( gui_msg_OK );
	APAdjustOkButton-> makeReturnDefault ( TRUE );
	APAdjustOkButton-> setCallback ( goAwayAPAdjust );

	APAdjustCancelButton = new puOneShot ( 70, 10, 150, 50 );
	APAdjustCancelButton-> setLegend ( gui_msg_CANCEL );
	APAdjustCancelButton-> setCallback ( cancelAPAdjust );

	APAdjustResetButton = new puOneShot ( 160, 10, 220, 50 );
	APAdjustResetButton-> setLegend ( gui_msg_RESET );
	APAdjustResetButton-> setCallback ( resetAPAdjust );
    }
    FG_FINALIZE_PUI_DIALOG( APAdjustDialog );

#undef HORIZONTAL
}

// Simple Dialog to input Target Airport
void TgtAptDialog_Cancel(puObject *)
{
    FG_POP_PUI_DIALOG( TgtAptDialog );
}

void TgtAptDialog_OK (puObject *)
{
    fgAPDataPtr APData;
    APData = APDataGlobal;
    string TgtAptId;
    
    //    FGTime *t = FGTime::cur_time_params;
    //    int PauseMode = t->getPause();
    //    if(!PauseMode)
    //        t->togglePauseMode();
    
    char *s;
    TgtAptDialogInput->getValue(&s);
    TgtAptId = s;
    
    TgtAptDialog_Cancel( NULL );
    
    if ( TgtAptId.length() ) {
        // set initial position from TgtAirport id
        
        fgAIRPORTS airports;
        fgAIRPORT a;
        
        FG_LOG( FG_GENERAL, FG_INFO,
                "Attempting to set starting position from airport code "
                << s );
        
        airports.load("apt_simple");
        if ( airports.search( TgtAptId, &a ) )
	    {
		double course, reverse, distance;
		//            fgAPset_tgt_airport_id( TgtAptId.c_str() );
		current_options.set_airport_id( TgtAptId.c_str() );
		sprintf( NewTgtAirportId, "%s", TgtAptId.c_str() );
			
		APData->TargetLatitude = a.latitude; // * DEG_TO_RAD; 
		APData->TargetLongitude = a.longitude; // * DEG_TO_RAD;
		MakeTargetLatLonStr( APData,
				     APData->TargetLatitude,
				     APData->TargetLongitude);
			
		APData->old_lat = fgAPget_latitude();
		APData->old_lon = fgAPget_longitude();
			
		// need to test for iter
		if( ! geo_inverse_wgs_84( fgAPget_altitude(),
					  fgAPget_latitude(),
					  fgAPget_longitude(),
					  APData->TargetLatitude,
					  APData->TargetLongitude,
					  &course,
					  &reverse,
					  &distance ) ) {
		    APData->TargetHeading = course;
		    MakeTargetHeadingStr( APData, APData->TargetHeading );			
		    APData->TargetDistance = distance;
		    MakeTargetDistanceStr( APData, distance );
		    // This changes the AutoPilot Heading
		    // following cast needed
		    ApHeadingDialogInput->setValue ((float)APData->TargetHeading );
		    // Force this !
		    APData->waypoint_hold = true ;
		    APData->heading_hold = true;
		}
	    } else {
		TgtAptId  += " not in database.";
		mkDialog(TgtAptId.c_str());
	    }
    }
    get_control_values();
    //    if( PauseMode != t->getPause() )
    //        t->togglePauseMode();
}

void TgtAptDialog_Reset(puObject *)
{
    //  strncpy( NewAirportId, current_options.get_airport_id().c_str(), 16 );
    sprintf( NewTgtAirportId, "%s", current_options.get_airport_id().c_str() );
    TgtAptDialogInput->setValue ( NewTgtAirportId );
    TgtAptDialogInput->setCursor( 0 ) ;
}

void NewTgtAirport(puObject *cb)
{
    //  strncpy( NewAirportId, current_options.get_airport_id().c_str(), 16 );
    sprintf( NewTgtAirportId, "%s", current_options.get_airport_id().c_str() );
    TgtAptDialogInput->setValue( NewTgtAirportId );
    
    FG_PUSH_PUI_DIALOG( TgtAptDialog );
}

void NewTgtAirportInit(void)
{
    FG_LOG( FG_AUTOPILOT, FG_INFO, " enter NewTgtAirportInit()" );
    //	fgAPset_tgt_airport_id( current_options.get_airport_id() );	
    sprintf( NewTgtAirportId, "%s", current_options.get_airport_id().c_str() );
    FG_LOG( FG_AUTOPILOT, FG_INFO, " NewTgtAirportId " << NewTgtAirportId );
    //	printf(" NewTgtAirportId %s\n", NewTgtAirportId);
    int len = 150 - puGetStringWidth( puGetDefaultLabelFont(),
                                      NewTgtAirportLabel ) / 2;
    
    TgtAptDialog = new puDialogBox (150, 50);
    {
        TgtAptDialogFrame   = new puFrame           (0,0,350, 150);
        TgtAptDialogMessage = new puText            (len, 110);
        TgtAptDialogMessage ->    setLabel          (NewTgtAirportLabel);
        
        TgtAptDialogInput   = new puInput           (50, 70, 300, 100);
        TgtAptDialogInput   ->    setValue          (NewTgtAirportId);
        TgtAptDialogInput   ->    acceptInput();
        
        TgtAptDialogOkButton     =  new puOneShot   (50, 10, 110, 50);
        TgtAptDialogOkButton     ->     setLegend   (gui_msg_OK);
        TgtAptDialogOkButton     ->     setCallback (TgtAptDialog_OK);
        TgtAptDialogOkButton     ->     makeReturnDefault(TRUE);
        
        TgtAptDialogCancelButton =  new puOneShot   (140, 10, 210, 50);
        TgtAptDialogCancelButton ->     setLegend   (gui_msg_CANCEL);
        TgtAptDialogCancelButton ->     setCallback (TgtAptDialog_Cancel);
        
        TgtAptDialogResetButton  =  new puOneShot   (240, 10, 300, 50);
        TgtAptDialogResetButton  ->     setLegend   (gui_msg_RESET);
        TgtAptDialogResetButton  ->     setCallback (TgtAptDialog_Reset);
    }
    FG_FINALIZE_PUI_DIALOG( TgtAptDialog );
    printf("leave NewTgtAirportInit()");
}


// Finally actual guts of AutoPilot
void fgAPInit( fgAIRCRAFT *current_aircraft ) {
    fgAPDataPtr APData;

    FG_LOG( FG_AUTOPILOT, FG_INFO, "Init AutoPilot Subsystem" );

    APData = ( fgAPDataPtr ) calloc( sizeof( fgAPData ), 1 );

    if ( APData == NULL ) {
	// I couldn't get the mem.  Dying
	FG_LOG( FG_AUTOPILOT, FG_ALERT, "No ram for Autopilot. Dying." );
	exit( -1 );
    }

    FG_LOG( FG_AUTOPILOT, FG_INFO, " Autopilot allocated " );
	
    APData->waypoint_hold = false ;     // turn the target hold off
    APData->heading_hold = false ;      // turn the heading hold off
    APData->altitude_hold = false ;     // turn the altitude hold off

    // Initialize target location to startup location
    //	FG_LOG( FG_AUTOPILOT, FG_INFO, " Autopilot setting startup location" );
    APData->old_lat = 
	APData->TargetLatitude = fgAPget_latitude();
    APData->old_lon =
	APData->TargetLongitude = fgAPget_longitude();

    //	FG_LOG( FG_AUTOPILOT, FG_INFO, " Autopilot setting TargetLatitudeStr" );
    MakeTargetLatLonStr( APData, APData->TargetLatitude, APData->TargetLongitude);
	
    APData->TargetHeading = 0.0;     // default direction, due north
    APData->TargetAltitude = 3000;   // default altitude in meters
    APData->alt_error_accum = 0.0;

    MakeTargetAltitudeStr( APData, 3000.0);
    MakeTargetHeadingStr( APData, 0.0 );
	
    // These eventually need to be read from current_aircaft somehow.

#if 0
    // Original values
    // the maximum roll, in Deg
    APData->MaxRoll = 7;
    // the deg from heading to start rolling out at, in Deg
    APData->RollOut = 30;
    // how far can I move the aleron from center.
    APData->MaxAileron = .1;
    // Smoothing distance for alerion control
    APData->RollOutSmooth = 10;
#endif

    // the maximum roll, in Deg
    APData->MaxRoll = 20;

    // the deg from heading to start rolling out at, in Deg
    APData->RollOut = 20;

    // how far can I move the aleron from center.
    APData->MaxAileron = .2;

    // Smoothing distance for alerion control
    APData->RollOutSmooth = 10;

    //Remove at a later date
    APDataGlobal = APData;

    // Hardwired for now should be in options
    // 25% max control variablilty  0.5 / 2.0
    APData->disengage_threshold = 1.0;

#if !defined( USING_SLIDER_CLASS )
    MaxRollAdjust = 2 * APData->MaxRoll;
    RollOutAdjust = 2 * APData->RollOut;
    MaxAileronAdjust = 2 * APData->MaxAileron;
    RollOutSmoothAdjust = 2 * APData->RollOutSmooth;
#endif  // !defined( USING_SLIDER_CLASS )

    get_control_values();
	
    //	FG_LOG( FG_AUTOPILOT, FG_INFO, " calling NewTgtAirportInit" );
    NewTgtAirportInit();
    fgAPAdjustInit() ;
    NewHeadingInit();
    NewAltitudeInit();
};


void fgAPReset( void ) {
    fgAPDataPtr APData = APDataGlobal;

    if ( fgAPTerrainFollowEnabled() )
	fgAPToggleTerrainFollow( );

    if ( fgAPAltitudeEnabled() )
	fgAPToggleAltitude();

    if ( fgAPHeadingEnabled() )
	fgAPToggleHeading();

    if ( fgAPAutoThrottleEnabled() )
	fgAPToggleAutoThrottle();

    APData->TargetHeading = 0.0;     // default direction, due north
    MakeTargetHeadingStr( APData, APData->TargetHeading );			
	
    APData->TargetAltitude = 3000;   // default altitude in meters
    MakeTargetAltitudeStr( APData, 3000);
	
    APData->alt_error_accum = 0.0;
	
	
    get_control_values();

    sprintf( NewTgtAirportId, "%s", current_options.get_airport_id().c_str() );
	
    APData->TargetLatitude = fgAPget_latitude();
    APData->TargetLongitude = fgAPget_longitude();
    MakeTargetLatLonStr( APData,
			 APData->TargetLatitude,
			 APData->TargetLongitude);
}


int fgAPRun( void ) {
    // Remove the following lines when the calling funcitons start
    // passing in the data pointer

    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    // get control settings 
    double aileron = controls.get_aileron();
    double elevator = controls.get_elevator();
    double elevator_trim = controls.get_elevator_trim();
    double rudder = controls.get_rudder();
	
    double lat = fgAPget_latitude();
    double lon = fgAPget_longitude();

#ifdef FG_FORCE_AUTO_DISENGAGE
    // see if somebody else has changed them
    if( fabs(aileron - APData->old_aileron) > APData->disengage_threshold ||
	fabs(elevator - APData->old_elevator) > APData->disengage_threshold ||
	fabs(elevator_trim - APData->old_elevator_trim) > 
	APData->disengage_threshold ||		
	fabs(rudder - APData->old_rudder) > APData->disengage_threshold )
    {
	// if controls changed externally turn autopilot off
	APData->waypoint_hold = false ;     // turn the target hold off
	APData->heading_hold = false ;      // turn the heading hold off
	APData->altitude_hold = false ;     // turn the altitude hold off
	APData->terrain_follow = false;     // turn the terrain_follow hold off
	// APData->auto_throttle = false;      // turn the auto_throttle off

	// stash this runs control settings
	APData->old_aileron = aileron;
	APData->old_elevator = elevator;
	APData->old_elevator_trim = elevator_trim;
	APData->old_rudder = rudder;
	
	return 0;
    }
#endif
	
    // waypoint hold enabled?
    if ( APData->waypoint_hold == true )
	{
	    double wp_course, wp_reverse, wp_distance;

#ifdef DO_fgAP_CORRECTED_COURSE
	    // compute course made good
	    // this needs lots of special casing before use
	    double course, reverse, distance, corrected_course;
	    // need to test for iter
	    geo_inverse_wgs_84( 0, //fgAPget_altitude(),
				APData->old_lat,
				APData->old_lon,
				lat,
				lon,
				&course,
				&reverse,
				&distance );
#endif // DO_fgAP_CORRECTED_COURSE

	    // compute course to way_point
	    // need to test for iter
	    if( ! geo_inverse_wgs_84( 0, //fgAPget_altitude(),
				      lat,
				      lon,
				      APData->TargetLatitude,
				      APData->TargetLongitude,
				      &wp_course,
				      &wp_reverse,
				      &wp_distance ) ) {
		
#ifdef DO_fgAP_CORRECTED_COURSE
		corrected_course = course - wp_course;
		if( fabs(corrected_course) > 0.1 )
		    printf("fgAP: course %f  wp_course %f  %f  %f\n",
			   course, wp_course, fabs(corrected_course), distance );
#endif // DO_fgAP_CORRECTED_COURSE
		
		if ( wp_distance > 100 ) {
		    // corrected_course = course - wp_course;
		    APData->TargetHeading = NormalizeDegrees(wp_course);
		} else {
		    printf("APData->distance(%f) to close\n", wp_distance);
		    // Real Close -- set heading hold to current heading
		    // and Ring the arival bell !!
		    NewTgtAirport(NULL);
		    APData->waypoint_hold = false;
		    // use previous
		    APData->TargetHeading = fgAPget_heading();
		}
		MakeTargetHeadingStr( APData, APData->TargetHeading );
		// Force this just in case
		APData->TargetDistance = wp_distance;
		MakeTargetDistanceStr( APData, wp_distance );
		// This changes the AutoPilot Heading Read Out
		// following cast needed
		ApHeadingDialogInput   ->    setValue ((float)APData->TargetHeading );
	    }
	    APData->heading_hold = true;
	}

    // heading hold enabled?
    if ( APData->heading_hold == true ) {
	double RelHeading;
	double TargetRoll;
	double RelRoll;
	double AileronSet;

	RelHeading =
	    NormalizeDegrees( APData->TargetHeading - fgAPget_heading() );
	// figure out how far off we are from desired heading

	// Now it is time to deterime how far we should be rolled.
	FG_LOG( FG_AUTOPILOT, FG_DEBUG, "RelHeading: " << RelHeading );


	// Check if we are further from heading than the roll out point
	if ( fabs( RelHeading ) > APData->RollOut ) {
	    // set Target Roll to Max in desired direction
	    if ( RelHeading < 0 ) {
		TargetRoll = 0 - APData->MaxRoll;
	    } else {
		TargetRoll = APData->MaxRoll;
	    }
	} else {
	    // We have to calculate the Target roll

	    // This calculation engine thinks that the Target roll
	    // should be a line from (RollOut,MaxRoll) to (-RollOut,
	    // -MaxRoll) I hope this works well.  If I get ambitious
	    // some day this might become a fancier curve or
	    // something.

	    TargetRoll = LinearExtrapolate( RelHeading, -APData->RollOut,
					    -APData->MaxRoll, APData->RollOut,
					    APData->MaxRoll );
	}

	// Target Roll has now been Found.

	// Compare Target roll to Current Roll, Generate Rel Roll

	FG_LOG( FG_COCKPIT, FG_BULK, "TargetRoll: " << TargetRoll );

	RelRoll = NormalizeDegrees( TargetRoll - fgAPget_roll() );

	// Check if we are further from heading than the roll out smooth point
	if ( fabs( RelRoll ) > APData->RollOutSmooth ) {
	    // set Target Roll to Max in desired direction
	    if ( RelRoll < 0 ) {
		AileronSet = 0 - APData->MaxAileron;
	    } else {
		AileronSet = APData->MaxAileron;
	    }
	} else {
	    AileronSet = LinearExtrapolate( RelRoll, -APData->RollOutSmooth,
					    -APData->MaxAileron,
					    APData->RollOutSmooth,
					    APData->MaxAileron );
	}

	controls.set_aileron( AileronSet );
	controls.set_rudder( AileronSet / 2.0 );
	// controls.set_rudder( 0.0 );
    }

    // altitude hold or terrain follow enabled?
    if ( APData->altitude_hold || APData->terrain_follow ) {
	double speed, max_climb, error;
	double prop_error, int_error;
	double prop_adj, int_adj, total_adj;

	if ( APData->altitude_hold ) {
	    // normal altitude hold
	    APData->TargetClimbRate =
		( APData->TargetAltitude - fgAPget_altitude() ) * 8.0;
	} else if ( APData->terrain_follow ) {
	    // brain dead ground hugging with no look ahead
	    APData->TargetClimbRate =
		( APData->TargetAGL - fgAPget_agl() ) * 16.0;
	} else {
	    // just try to zero out rate of climb ...
	    APData->TargetClimbRate = 0.0;
	}

	speed = get_speed();

	if ( speed < 90.0 ) {
	    max_climb = 0.0;
	} else if ( speed < 100.0 ) {
	    max_climb = ( speed - 90.0 ) * 20;
	    //        } else if ( speed < 150.0 ) {
	} else {			
	    max_climb = ( speed - 100.0 ) * 4.0 + 200.0;
	} //else { // this is NHV hack
	//            max_climb = ( speed - 150.0 ) * 6.0 + 300.0;
	//        }

	if ( APData->TargetClimbRate > max_climb ) {
	    APData->TargetClimbRate = max_climb;
	}

	else if ( APData->TargetClimbRate < -400.0 ) {
	    APData->TargetClimbRate = -400.0;
	}

	error = fgAPget_climb() - APData->TargetClimbRate;

	// accumulate the error under the curve ... this really should
	// be *= delta t
	APData->alt_error_accum += error;

	// calculate integral error, and adjustment amount
	int_error = APData->alt_error_accum;
	// printf("error = %.2f  int_error = %.2f\n", error, int_error);
	int_adj = int_error / 8000.0;

	// caclulate proportional error
	prop_error = error;
	prop_adj = prop_error / 2000.0;

	total_adj = 0.9 * prop_adj + 0.1 * int_adj;
	if ( total_adj > 0.6 ) {
	    total_adj = 0.6;
	}
	else if ( total_adj < -0.2 ) {
	    total_adj = -0.2;
	}

	controls.set_elevator( total_adj );
    }

    // auto throttle enabled?
    if ( APData->auto_throttle ) {
	double error;
	double prop_error, int_error;
	double prop_adj, int_adj, total_adj;

	error = APData->TargetSpeed - get_speed();

	// accumulate the error under the curve ... this really should
	// be *= delta t
	APData->speed_error_accum += error;
	if ( APData->speed_error_accum > 2000.0 ) {
	    APData->speed_error_accum = 2000.0;
	}
	else if ( APData->speed_error_accum < -2000.0 ) {
	    APData->speed_error_accum = -2000.0;
	}

	// calculate integral error, and adjustment amount
	int_error = APData->speed_error_accum;

	// printf("error = %.2f  int_error = %.2f\n", error, int_error);
	int_adj = int_error / 200.0;

	// caclulate proportional error
	prop_error = error;
	prop_adj = 0.5 + prop_error / 50.0;

	total_adj = 0.9 * prop_adj + 0.1 * int_adj;
	if ( total_adj > 1.0 ) {
	    total_adj = 1.0;
	}
	else if ( total_adj < 0.0 ) {
	    total_adj = 0.0;
	}

	controls.set_throttle( FGControls::ALL_ENGINES, total_adj );
    }

#ifdef THIS_CODE_IS_NOT_USED
    if (APData->Mode == 2) // Glide slope hold
	{
	    double RelSlope;
	    double RelElevator;

	    // First, calculate Relative slope and normalize it
	    RelSlope = NormalizeDegrees( APData->TargetSlope - get_pitch());

	    // Now calculate the elevator offset from current angle
	    if ( abs(RelSlope) > APData->SlopeSmooth )
		{
		    if ( RelSlope < 0 )     //  set RelElevator to max in the correct direction
			RelElevator = -APData->MaxElevator;
		    else
			RelElevator = APData->MaxElevator;
		}

	    else
		RelElevator = LinearExtrapolate(RelSlope,-APData->SlopeSmooth,-APData->MaxElevator,APData->SlopeSmooth,APData->MaxElevator);

	    // set the elevator
	    fgElevMove(RelElevator);

	}
#endif // THIS_CODE_IS_NOT_USED

    // stash this runs control settings
    //	get_control_values();
    APData->old_aileron = controls.get_aileron();
    APData->old_elevator = controls.get_elevator();
    APData->old_elevator_trim = controls.get_elevator_trim();
    APData->old_rudder = controls.get_rudder();

    // for cross track error
    APData->old_lat = lat;
    APData->old_lon = lon;
	
	// Ok, we are done
    return 0;
}

/*
void fgAPSetMode( int mode)
{
//Remove the following line when the calling funcitons start passing in the data pointer
fgAPDataPtr APData;

APData = APDataGlobal;
// end section

fgPrintf( FG_COCKPIT, FG_INFO, "APSetMode : %d\n", mode );

APData->Mode = mode;  // set the new mode
}
*/
#if 0
void fgAPset_tgt_airport_id( const string id ) {
    FG_LOG( FG_AUTOPILOT, FG_INFO, "entering  fgAPset_tgt_airport_id " << id );
    fgAPDataPtr APData;
    APData = APDataGlobal;
    APData->tgt_airport_id = id;
    FG_LOG( FG_AUTOPILOT, FG_INFO, "leaving  fgAPset_tgt_airport_id "
	    << APData->tgt_airport_id );
};

string fgAPget_tgt_airport_id( void ) {
    fgAPDataPtr APData = APDataGlobal;
    return APData->tgt_airport_id;
};
#endif

void fgAPToggleHeading( void ) {
    // Remove at a later date
    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    if ( APData->heading_hold || APData->waypoint_hold ) {
	// turn off heading hold
	APData->heading_hold = false;
	APData->waypoint_hold = false;
    } else {
	// turn on heading hold, lock at current heading
	APData->heading_hold = true;
	APData->TargetHeading = fgAPget_heading();
	MakeTargetHeadingStr( APData, APData->TargetHeading );			
	ApHeadingDialogInput -> setValue ((float)APData->TargetHeading );
    }

    get_control_values();
    FG_LOG( FG_COCKPIT, FG_INFO, " fgAPSetHeading: ("
	    << APData->heading_hold << ") " << APData->TargetHeading );
}

void fgAPToggleWayPoint( void ) {
    // Remove at a later date
    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    if ( APData->waypoint_hold ) {
	// turn off location hold
	APData->waypoint_hold = false;
	// set heading hold to current heading
	//		APData->heading_hold = true;
	APData->TargetHeading = fgAPget_heading();
    } else {
	double course, reverse, distance;
	// turn on location hold
	// turn on heading hold
	APData->old_lat = fgAPget_latitude();
	APData->old_lon = fgAPget_longitude();
			
			// need to test for iter
	if(!geo_inverse_wgs_84( fgAPget_altitude(),
				fgAPget_latitude(),
				fgAPget_longitude(),
				APData->TargetLatitude,
				APData->TargetLongitude,
				&course,
				&reverse,
				&distance ) ) {
	    APData->TargetHeading = course;
	    APData->TargetDistance = distance;
	    MakeTargetDistanceStr( APData, distance );
	}
	// Force this !
	APData->waypoint_hold = true;
	APData->heading_hold = true;
    }

    // This changes the AutoPilot Heading
    // following cast needed
    ApHeadingDialogInput->setValue ((float)APData->TargetHeading );
    MakeTargetHeadingStr( APData, APData->TargetHeading );
	
    get_control_values();
	
    FG_LOG( FG_COCKPIT, FG_INFO, " fgAPSetLocation: ( "
	    << APData->waypoint_hold   << " "
	    << APData->TargetLatitude  << " "
	    << APData->TargetLongitude << " ) "
	    );
}


void fgAPToggleAltitude( void ) {
    // Remove at a later date
    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    if ( APData->altitude_hold ) {
	// turn off altitude hold
	APData->altitude_hold = false;
    } else {
	// turn on altitude hold, lock at current altitude
	APData->altitude_hold = true;
	APData->terrain_follow = false;
	APData->TargetAltitude = fgAPget_altitude();
	APData->alt_error_accum = 0.0;
	// alt_error_queue.erase( alt_error_queue.begin(),
	//                        alt_error_queue.end() );
	float target_alt = APData->TargetAltitude;
	if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET )
	    target_alt *= METER_TO_FEET;
		
	ApAltitudeDialogInput->setValue(target_alt);
	MakeTargetAltitudeStr( APData, target_alt);
    }

    get_control_values();
    FG_LOG( FG_COCKPIT, FG_INFO, " fgAPSetAltitude: ("
	    << APData->altitude_hold << ") " << APData->TargetAltitude );
}


void fgAPToggleAutoThrottle ( void ) {
    // Remove at a later date
    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    if ( APData->auto_throttle ) {
	// turn off altitude hold
	APData->auto_throttle = false;
    } else {
	// turn on terrain follow, lock at current agl
	APData->auto_throttle = true;
	APData->TargetSpeed = get_speed();
	APData->speed_error_accum = 0.0;
    }

    get_control_values();
    FG_LOG( FG_COCKPIT, FG_INFO, " fgAPSetAutoThrottle: ("
	    << APData->auto_throttle << ") " << APData->TargetSpeed );
}

void fgAPToggleTerrainFollow( void ) {
    // Remove at a later date
    fgAPDataPtr APData;

    APData = APDataGlobal;
    // end section

    if ( APData->terrain_follow ) {
	// turn off altitude hold
	APData->terrain_follow = false;
    } else {
	// turn on terrain follow, lock at current agl
	APData->terrain_follow = true;
	APData->altitude_hold = false;
	APData->TargetAGL = fgAPget_agl();
	APData->alt_error_accum = 0.0;
    }
    get_control_values();
	
    FG_LOG( FG_COCKPIT, FG_INFO, " fgAPSetTerrainFollow: ("
	    << APData->terrain_follow << ") " << APData->TargetAGL );
}

static double NormalizeDegrees( double Input ) {
    // normalize the input to the range (-180,180]
    // Input should not be greater than -360 to 360.
    // Current rules send the output to an undefined state.
    if ( Input > 180 )
	while(Input > 180 )
	    Input -= 360;
    else if ( Input <= -180 )
	while ( Input <= -180 )
	    Input += 360;
    return ( Input );
};

static double LinearExtrapolate( double x, double x1, double y1, double x2, double y2 ) {
    // This procedure extrapolates the y value for the x posistion on a line defined by x1,y1; x2,y2
    //assert(x1 != x2); // Divide by zero error.  Cold abort for now

	// Could be
	// static double y = 0.0;
	// double dx = x2 -x1;
	// if( (dx < -FG_EPSILON ) || ( dx > FG_EPSILON ) )
	// {

    double m, b, y;          // the constants to find in y=mx+b
    // double m, b;

    m = ( y2 - y1 ) / ( x2 - x1 );   // calculate the m

    b = y1 - m * x1;       // calculate the b

    y = m * x + b;       // the final calculation

    // }

    return ( y );

};


/* Direct and inverse distance functions */
/** Proceedings of the 7th International Symposium on Geodetic
  Computations, 1985
  "The Nested Coefficient Method for Accurate Solutions of Direct
  and
  Inverse Geodetic Problems With Any Length"
  Zhang Xue-Lian
  pp 747-763
  */
/* modified for FlightGear to use WGS84 only  Norman Vine */

//#include "dstazfns.h"
#include <math.h>
#define GEOD_INV_PI      (3.14159265358979323846)

/* s == distance */
/* az = azimuth */

/* for WGS_84 a = 6378137.000, rf = 298.257223563; */

static double M0( double e2 )
{	//double e4 = e2*e2;
    return GEOD_INV_PI*(1.0 - e2*( 1.0/4.0 + e2*( 3.0/64.0 + e2*(5.0/256.0) )))/2.0;
}
/* s == distance */
int geo_direct_wgs_84 ( double alt, double lat1, double lon1, double az1, double s, 
			double *lat2, double *lon2,  double *az2 )
{
    double a = 6378137.000, rf = 298.257223563;
    double RADDEG = (GEOD_INV_PI)/180.0, testv = 1.0E-10;
    double f = ( rf > 0.0 ? 1.0/rf : 0.0 );
    double b = a*(1.0-f), e2 = f*(2.0-f);
    double phi1 = lat1*RADDEG, lam1 = lon1*RADDEG;
    double sinphi1 = sin(phi1), cosphi1 = cos(phi1);
    double azm1 = az1*RADDEG;
    double sinaz1 = sin(azm1), cosaz1 = cos(azm1);
	
	
    if( fabs(s) < 0.01 )  /* distance < centimeter => congruency */
	{	*lat2 = lat1;
	*lon2 = lon1;
	*az2 = 180.0 + az1;
	if( *az2 > 360.0 ) *az2 -= 360.0;
	return 0;
	}
    else
	if( cosphi1 )	 /* non-polar origin */
	    {	/* u1 is reduced latitude */
		double tanu1 = sqrt(1.0-e2)*sinphi1/cosphi1;
   		double sig1 = atan2(tanu1,cosaz1);
		double cosu1 = 1.0/sqrt( 1.0 + tanu1*tanu1 ), sinu1 = tanu1*cosu1;
		double sinaz =  cosu1*sinaz1, cos2saz = 1.0-sinaz*sinaz;
		double us = cos2saz*e2/(1.0-e2);
		/*	Terms */
		double	ta = 1.0+us*(4096.0+us*(-768.0+us*(320.0-175.0*us)))/16384.0,
		    tb = us*(256.0+us*(-128.0+us*(74.0-47.0*us)))/1024.0,
		    tc = 0;
		/*	FIRST ESTIMATE OF SIGMA (SIG) */		
		double first = s/(b*ta); /* !!*/
		double sig = first;
		double c2sigm, sinsig,cossig, temp,denom,rnumer, dlams, dlam;
		do
		    {	c2sigm = cos(2.0*sig1+sig);
		    sinsig = sin(sig); cossig = cos(sig);
		    temp = sig;
		    sig = first + 
			tb*sinsig*(c2sigm+tb*(cossig*(-1.0+2.0*c2sigm*c2sigm) - 
					      tb*c2sigm*(-3.0+4.0*sinsig*sinsig)
					      *(-3.0+4.0*c2sigm*c2sigm)/6.0)
				   /4.0);
		    }
		while( fabs(sig-temp) > testv);
		/* 	LATITUDE OF POINT 2 */
		/*	DENOMINATOR IN 2 PARTS (TEMP ALSO USED LATER) */
		temp = sinu1*sinsig-cosu1*cossig*cosaz1;
		denom = (1.0-f)*sqrt(sinaz*sinaz+temp*temp);
		/* NUMERATOR */
		rnumer = sinu1*cossig+cosu1*sinsig*cosaz1;
		*lat2 = atan2(rnumer,denom)/RADDEG;
		/* DIFFERENCE IN LONGITUDE ON AUXILARY SPHERE (DLAMS ) */
		rnumer = sinsig*sinaz1;
		denom = cosu1*cossig-sinu1*sinsig*cosaz1;
		dlams = atan2(rnumer,denom);
		/* TERM C */
		tc = f*cos2saz*(4.0+f*(4.0-3.0*cos2saz))/16.0;
		/* DIFFERENCE IN LONGITUDE */
		dlam = dlams-(1.0-tc)*f*sinaz*(sig+tc*sinsig*
					       (c2sigm+
						tc*cossig*(-1.0+2.0*
							   c2sigm*c2sigm)));
		*lon2 = (lam1+dlam)/RADDEG;
		if(*lon2 > 180.0  ) *lon2 -= 360.0;
		if(*lon2 < -180.0 ) *lon2 += 360.0;
		/* AZIMUTH - FROM NORTH */
		*az2 = atan2(-sinaz,temp)/RADDEG;
		if( fabs(*az2) < testv ) *az2 = 0.0;
		if( *az2 < 0.0) *az2 += 360.0;
		return 0;
	    }
	else /* phi1 == 90 degrees, polar origin  */
	    {	double dM = a*M0(e2) - s;
	    double paz = ( phi1 < 0.0 ? 180.0 : 0.0 );
	    return geo_direct_wgs_84( alt, 0.0, lon1, paz, dM,lat2,lon2,az2 );
	    } 
}

int geo_inverse_wgs_84( double alt, double lat1, double lon1, double lat2,
			double lon2, double *az1, double *az2, double *s )
{
    double a = 6378137.000, rf = 298.257223563;
    int iter=0;
    double RADDEG = (GEOD_INV_PI)/180.0, testv = 1.0E-10;
    double f = ( rf > 0.0 ? 1.0/rf : 0.0 );
    double b = a*(1.0-f), e2 = f*(2.0-f);
    double phi1 = lat1*RADDEG, lam1 = lon1*RADDEG;
    double sinphi1 = sin(phi1), cosphi1 = cos(phi1);
    double phi2 = lat2*RADDEG, lam2 = lon2*RADDEG;
    double sinphi2 = sin(phi2), cosphi2 = cos(phi2);
	
    if( (fabs(lat1-lat2) < testv && 
	 ( fabs(lon1-lon2) < testv) || fabs(lat1-90.0) < testv ) )
    {	/* TWO STATIONS ARE IDENTICAL : SET DISTANCE & AZIMUTHS TO ZERO */
	*az1 = 0.0; *az2 = 0.0; *s = 0.0;
	return 0;
    } else
	if(  fabs(cosphi1) < testv ) /* initial point is polar */
	{
	    int k = geo_inverse_wgs_84( alt, lat2,lon2,lat1,lon1, az1,az2,s );
	    b = *az1; *az1 = *az2; *az2 = b;
	    return 0;
	} else
	    if( fabs(cosphi2) < testv ) /* terminal point is polar */
	    {
		int k = geo_inverse_wgs_84( alt, lat1,lon1,lat1,lon1+180.0, 
					    az1,az2,s );
		*s /= 2.0;
		*az2 = *az1 + 180.0;
		if( *az2 > 360.0 ) *az2 -= 360.0; 
		return 0;
	    } else  	/* Geodesic passes through the pole (antipodal) */
		if( (fabs( fabs(lon1-lon2) - 180 ) < testv) && 
		    (fabs(lat1+lat2) < testv) ) 
		{
		    double s1,s2;
		    geo_inverse_wgs_84( alt, lat1,lon1, lat1,lon2, az1,az2, &s1 );
		    geo_inverse_wgs_84( alt, lat2,lon2, lat1,lon2, az1,az2, &s2 );
		    *az2 = *az1;
		    *s = s1 + s2;
		    return 0;
		} else  /* antipodal and polar points don't get here */
		{
		    double dlam = lam2 - lam1, dlams = dlam;
		    double sdlams,cdlams, sig,sinsig,cossig, sinaz,
			cos2saz, c2sigm;
		    double tc,temp, us,rnumer,denom, ta,tb;
		    double cosu1,sinu1, sinu2,cosu2;
		    /* Reduced latitudes */
		    temp = (1.0-f)*sinphi1/cosphi1;
		    cosu1 = 1.0/sqrt(1.0+temp*temp);
		    sinu1 = temp*cosu1;
		    temp = (1.0-f)*sinphi2/cosphi2;
		    cosu2 = 1.0/sqrt(1.0+temp*temp);
		    sinu2 = temp*cosu2;
    
		    do {
			sdlams = sin(dlams), cdlams = cos(dlams);
			sinsig = sqrt(cosu2*cosu2*sdlams*sdlams+
				      (cosu1*sinu2-sinu1*cosu2*cdlams)*
				      (cosu1*sinu2-sinu1*cosu2*cdlams));
			cossig = sinu1*sinu2+cosu1*cosu2*cdlams;
			
			sig = atan2(sinsig,cossig);
			sinaz = cosu1*cosu2*sdlams/sinsig;
			cos2saz = 1.0-sinaz*sinaz;
			c2sigm = (sinu1 == 0.0 || sinu2 == 0.0 ? cossig : 
				  cossig-2.0*sinu1*sinu2/cos2saz);
			tc = f*cos2saz*(4.0+f*(4.0-3.0*cos2saz))/16.0;
			temp = dlams;
			dlams = dlam+(1.0-tc)*f*sinaz*
			    (sig+tc*sinsig*
			     (c2sigm+tc*cossig*(-1.0+2.0*c2sigm*c2sigm)));
			if (fabs(dlams) > GEOD_INV_PI && iter++ > 50) 
			    return iter;
		    } while ( fabs(temp-dlams) > testv);

		    us = cos2saz*(a*a-b*b)/(b*b);	/* !! */
		    /* BACK AZIMUTH FROM NORTH */
		    rnumer = -(cosu1*sdlams);
		    denom = sinu1*cosu2-cosu1*sinu2*cdlams;
		    *az2 = atan2(rnumer,denom)/RADDEG;
		    if( fabs(*az2) < testv ) *az2 = 0.0;
		    if(*az2 < 0.0) *az2 += 360.0;
		    /* FORWARD AZIMUTH FROM NORTH */
		    rnumer = cosu2*sdlams;
		    denom = cosu1*sinu2-sinu1*cosu2*cdlams;
		    *az1 = atan2(rnumer,denom)/RADDEG;
		    if( fabs(*az1) < testv ) *az1 = 0.0;
		    if(*az1 < 0.0) *az1 += 360.0;
		    /* Terms a & b */
		    ta = 1.0+us*(4096.0+us*(-768.0+us*(320.0-175.0*us)))/
			16384.0;
		    tb = us*(256.0+us*(-128.0+us*(74.0-47.0*us)))/1024.0;
		    /* GEODETIC DISTANCE */
		    *s = b*ta*(sig-tb*sinsig*
			       (c2sigm+tb*(cossig*(-1.0+2.0*c2sigm*c2sigm)-tb*
					   c2sigm*(-3.0+4.0*sinsig*sinsig)*
					   (-3.0+4.0*c2sigm*c2sigm)/6.0)/
				4.0));
		    return 0;
		}
}
