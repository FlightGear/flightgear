#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdlib.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgpath.hxx>

#include <Include/general.hxx>

#include <GL/glut.h>
#include <simgear/xgl/xgl.h>

#include <Airports/simple.hxx>
#include <FDM/flight.hxx>
#include <Main/fg_init.hxx>
#include <Main/globals.hxx>
#include <Main/options.hxx>
//#include <Time/fg_time.hxx>

#include "gui.h"
#include "gui_local.hxx"

/// The beginnings of teleportation :-)
//  Needs cleaning up but works
//  These statics should disapear when this is a class
static puDialogBox     *AptDialog = 0;
static puFrame         *AptDialogFrame = 0;
static puText          *AptDialogMessage = 0;
static puInput         *AptDialogInput = 0;

static char NewAirportId[16];
static char NewAirportLabel[] = "Enter New Airport ID"; 

static puOneShot       *AptDialogOkButton = 0;
static puOneShot       *AptDialogCancelButton = 0;
static puOneShot       *AptDialogResetButton = 0;

void AptDialog_Cancel(puObject *)
{
	FG_POP_PUI_DIALOG( AptDialog );
}

void AptDialog_OK (puObject *)
{
	FGPath path( globals->get_options()->get_fg_root() );
	path.append( "Airports" );
	path.append( "simple.mk4" );
	FGAirports airports( path.c_str() );

	FGAirport a;

	int freeze = globals->get_freeze();
	if(!freeze)
		globals->set_freeze( true );

	char *s;
	AptDialogInput->getValue(&s);
	string AptId(s);

	cout << "AptDialog_OK " << AptId << " " << AptId.length() << endl;

	AptDialog_Cancel( NULL );

	if ( AptId.length() ) {
		// set initial position from airport id
		FG_LOG( FG_GENERAL, FG_INFO,
				"Attempting to set starting position from airport code "
				<< AptId );

		if ( airports.search( AptId, &a ) )
		{
			globals->get_options()->set_airport_id( AptId.c_str() );
			globals->get_options()->set_altitude( -9999.0 );
		// fgSetPosFromAirportID( AptId );
			fgSetPosFromAirportIDandHdg( AptId, 
										 cur_fdm_state->get_Psi() * RAD_TO_DEG);
			BusyCursor(0);
			fgReInitSubsystems();
			BusyCursor(1);
		} else {
			AptId  += " not in database.";
			mkDialog(AptId.c_str());
		}
	}
	if(!freeze)
		globals->set_freeze( false );
}

void AptDialog_Reset(puObject *)
{
	//  strncpy( NewAirportId, globals->get_options()->get_airport_id().c_str(), 16 );
	sprintf( NewAirportId, "%s", globals->get_options()->get_airport_id().c_str() );
	AptDialogInput->setValue ( NewAirportId );
	AptDialogInput->setCursor( 0 ) ;
}

void NewAirport(puObject *cb)
{
	//  strncpy( NewAirportId, globals->get_options()->get_airport_id().c_str(), 16 );
	sprintf( NewAirportId, "%s", globals->get_options()->get_airport_id().c_str() );
//	cout << "NewAirport " << NewAirportId << endl;
	AptDialogInput->setValue( NewAirportId );

	FG_PUSH_PUI_DIALOG( AptDialog );
}

void NewAirportInit(void)
{
	sprintf( NewAirportId, "%s", globals->get_options()->get_airport_id().c_str() );
	int len = 150 - puGetStringWidth( puGetDefaultLabelFont(),
									  NewAirportLabel ) / 2;

	AptDialog = new puDialogBox (150, 50);
	{
		AptDialogFrame   = new puFrame           (0,0,350, 150);
		AptDialogMessage = new puText            (len, 110);
		AptDialogMessage ->    setLabel          (NewAirportLabel);

		AptDialogInput   = new puInput           (50, 70, 300, 100);
		AptDialogInput   ->    setValue          (NewAirportId);
		AptDialogInput   ->    acceptInput();

		AptDialogOkButton     =  new puOneShot   (50, 10, 110, 50);
		AptDialogOkButton     ->     setLegend   (gui_msg_OK);
		AptDialogOkButton     ->     setCallback (AptDialog_OK);
		AptDialogOkButton     ->     makeReturnDefault(TRUE);

		AptDialogCancelButton =  new puOneShot   (140, 10, 210, 50);
		AptDialogCancelButton ->     setLegend   (gui_msg_CANCEL);
		AptDialogCancelButton ->     setCallback (AptDialog_Cancel);

		AptDialogResetButton  =  new puOneShot   (240, 10, 300, 50);
		AptDialogResetButton  ->     setLegend   (gui_msg_RESET);
		AptDialogResetButton  ->     setCallback (AptDialog_Reset);
	}
	cout << "NewAirportInit " << NewAirportId << endl;
	FG_FINALIZE_PUI_DIALOG( AptDialog );
}
