// ATCDialog.cxx - Functions and classes to handle the pop-up ATC dialog
//
// Written by Alexander Kappes and David Luff, started February 2003.
//
// Copyright (C) 2003  Alexander Kappes and David Luff
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

#include <Main/globals.hxx>
#include <Main/fgfs.hxx>
#include <GUI/gui.h>
#include <simgear/misc/commands.hxx>

#include "ATCDialog.hxx"
#include "ATC.hxx"
#include "ATCmgr.hxx"

FGATCDialog *current_atcdialog;

// For the command manager - maybe eventually this should go in the built in command list
static bool do_ATC_dialog(const SGPropertyNode* arg) {
	globals->get_ATC_mgr()->doPopupDialog();
	return(true);
}

ATCMenuEntry::ATCMenuEntry() {
  stationid    = "";
  stationfr    = 0;
  transmission = "";
  menuentry    = "";
}

ATCMenuEntry::~ATCMenuEntry() {
}

static char* t0 = "Request landing clearance";
static char* t1 = "Request departure clearance";
static char* t2 = "Report Runway vacated";
static char** towerOptions = new char*[4];

// ----------------------- DCL ------------------------------------------
// For the ATC dialog - copied from the Autopilot new heading dialog code!
static puDialogBox*		atcDialog;
static puFrame*			atcDialogFrame;
static puText*			atcDialogMessage;
//static puInput*			atcDialogInput;
static puOneShot*		atcDialogOkButton;
static puOneShot*		atcDialogCancelButton;
static puButtonBox*		atcDialogCommunicationOptions;
// ----------------------------------------------------------------------

// ------------------------ AK ------------------------------------------
static puDialogBox  *ATCMenuBox = 0;
static puFrame      *ATCMenuFrame = 0;
static puText       *ATCMenuBoxMessage = 0;
static puButtonBox	*ATCOptionsList = 0;
// ----------------------------------------------------------------------

// AK
static void AKATCDialogOK(puObject *)
{
	switch(ATCOptionsList->getValue()) {
	case 0:
		//cout << "Option 0 chosen\n";
		fgSetBool("/sim/atc/opt0",true);
		break;
	case 1:
		//cout << "Option 1 chosen\n";
		fgSetBool("/sim/atc/opt1",true);
		break;
	case 2:
		//cout << "Option 2 chosen\n";
		fgSetBool("/sim/atc/opt2",true);
		break;
	case 3:
		//cout << "Option 2 chosen\n";
		fgSetBool("/sim/atc/opt3",true);
		break;
	default:
		break;
	}
	FG_POP_PUI_DIALOG( ATCMenuBox );
}

// AK
static void AKATCDialogCancel(puObject *)
{
    FG_POP_PUI_DIALOG( ATCMenuBox );
}

// DCL
static void ATCDialogCancel(puObject *)
{
    //ATCDialogInput->rejectInput();
    FG_POP_PUI_DIALOG( atcDialog );
}

// DCL
static void ATCDialogOK (puObject *me)
{
	// Note that currently the dialog is hardwired to comm1 only here.
	switch(globals->get_ATC_mgr()->GetComm1ATCType()) {
	case INVALID:
		break;
	case ATIS:
		break;
	case TOWER: {
		FGTower* twr = (FGTower*)globals->get_ATC_mgr()->GetComm1ATCPointer();
		switch(atcDialogCommunicationOptions->getValue()) {
		case 0:
			//cout << "Option 0 chosen\n";
			twr->RequestLandingClearance("golf bravo echo");
			break;
		case 1:
			//cout << "Option 1 chosen\n";
			twr->RequestDepartureClearance("golf bravo echo");
			break;
		case 2:
			//cout << "Option 2 chosen\n";
			twr->ReportRunwayVacated("golf bravo echo");
			break;
		default:
			break;
		}
		break;
	}
	case GROUND:
		break;
	case APPROACH:
		break;
	default:
		break;
	}

    ATCDialogCancel(me);
    //if(error) mkDialog(s.c_str());
}

// DCL
static void ATCDialog(puObject *cb)
{
    //ApHeadingDialogInput   ->    setValue ( heading );
    //ApHeadingDialogInput    -> acceptInput();
    FG_PUSH_PUI_DIALOG(atcDialog);
}

// DCL
void ATCDialogInit()
{
	char defaultATCLabel[] = "Enter desired option to communicate with ATC:";
	char *s;

	// Option lists hardwired per ATC type	
	towerOptions[0] = new char[strlen(t0)+1];
	strcpy(towerOptions[0], t0);
	towerOptions[1] = new char[strlen(t1)+1];
	strcpy(towerOptions[1], t1);
	towerOptions[2] = new char[strlen(t2)+1];
	strcpy(towerOptions[2], t2);
	towerOptions[3] = NULL;
	
	atcDialog = new puDialogBox (150, 50);
	{
		atcDialogFrame   = new puFrame (0, 0, 500, 250);
		
		atcDialogMessage = new puText          (250, 220);
		atcDialogMessage    -> setDefaultValue (defaultATCLabel);
		atcDialogMessage    -> getDefaultValue (&s);
		atcDialogMessage    -> setLabel        (s);
		atcDialogMessage    -> setLabelPlace   (PUPLACE_TOP_CENTERED);

		atcDialogCommunicationOptions = new puButtonBox (50, 50, 450, 210, NULL, true);
		
		atcDialogOkButton     =  new puOneShot         (50, 10, 110, 50);
		atcDialogOkButton     ->     setLegend         (gui_msg_OK);
		atcDialogOkButton     ->     makeReturnDefault (TRUE);
		atcDialogOkButton     ->     setCallback       (ATCDialogOK);
		
		atcDialogCancelButton =  new puOneShot         (140, 10, 210, 50);
		atcDialogCancelButton ->     setLegend         (gui_msg_CANCEL);
		atcDialogCancelButton ->     setCallback       (ATCDialogCancel);
		
	}
	FG_FINALIZE_PUI_DIALOG(atcDialog);
	
	// Add ATC-dialog to the command list
	globals->get_commands()->addCommand("ATC-dialog", do_ATC_dialog);
}

///////////////////////////////////////////////////////////////////////
//
// ATCDoDialog is in a state of flux at the moment
// Stations other than approach are handled by DCL's simple code
// Approach is handled by AK's fancy dynamic-list code
// Hopefully all interactive stations should go to AK's code eventually
//
///////////////////////////////////////////////////////////////////////
void ATCDoDialog(atc_type type) {
	switch(type) {
	case INVALID:
		atcDialogCommunicationOptions->newList(NULL);
		atcDialogMessage->setLabel("Not tuned in to any ATC service.");
		break;
	case ATIS:
		atcDialogCommunicationOptions->newList(NULL);
		atcDialogMessage->setLabel("Tuned in to ATIS: no communication possible.");
		break;
	case TOWER: 
		atcDialogCommunicationOptions->newList(towerOptions);
		atcDialogMessage->setLabel("Tuned in to Tower - select communication to transmit:");
		break;
	case GROUND:
		atcDialogCommunicationOptions->newList(NULL);
		atcDialogMessage->setLabel("Tuned in to Ground - select communication to transmit:");
		break;
	case APPROACH:
		current_atcdialog->DoDialog();
		break;
	default:
		atcDialogCommunicationOptions->newList(NULL);
		atcDialogMessage->setLabel("Tuned in to unknown ATC service - enter transmission:");
		break;
	}

	// Third - display the dialog without pausing sim.
	if(type != APPROACH) {	
		ATCDialog(NULL);
	}
}

void FGATCDialog::Init() {
}

// AK
// Add an entry
void FGATCDialog::add_entry(string station, string transmission, string menutext ) {
  
  ATCMenuEntry a;

  a.stationid = station;
  a.transmission = transmission;
  a.menuentry = menutext;

  atcmentrylist_station[station.c_str()].push_back(a);

}

// AK
// query the database whether the transmission is already registered; 
bool FGATCDialog::trans_reg( const string &station, const string &trans ) {

  atcmentry_list_type     atcmlist = atcmentrylist_station[station];
  atcmentry_list_iterator current  = atcmlist.begin();
  atcmentry_list_iterator last     = atcmlist.end();
  
  for ( ; current != last ; ++current ) {
    if ( current->transmission == trans ) return true;
  }
  return false;
}

// AK
// ===================================================
// ===  Update ATC menue and look for keys pressed ===
// ===================================================
void FGATCDialog::DoDialog() {
	
	static string mentry[10];
	static string mtrans[10];
	char   buf[10];
	TransPar TPar;
	FGATC* atcptr = globals->get_ATC_mgr()->GetComm1ATCPointer();	// Hardwired to comm1 at the moment
	
	if(atcptr != NULL) {
		
		atcmentry_list_type     atcmlist = atcmentrylist_station[atcptr->get_ident()];
		//atcmentry_list_type     atcmlist = atcmentrylist_station["EGNX"];
		atcmentry_list_iterator current  = atcmlist.begin();
		atcmentry_list_iterator last     = atcmlist.end();
		
		// Set all opt flags to false before displaying box
		fgSetBool("/sim/atc/opt0",false);
		fgSetBool("/sim/atc/opt1",false);
		fgSetBool("/sim/atc/opt2",false);
		fgSetBool("/sim/atc/opt3",false);
		fgSetBool("/sim/atc/opt4",false);
		fgSetBool("/sim/atc/opt5",false);
		fgSetBool("/sim/atc/opt6",false);
		fgSetBool("/sim/atc/opt7",false);
		fgSetBool("/sim/atc/opt8",false);
		fgSetBool("/sim/atc/opt9",false);
		
		//int yc = 10;
		int yc = 70;
		int xsize = 600;
		
		if ( atcmlist.size() != 0 ){ 
			int k=atcmlist.size();
			//int k = 3;
			//cout << "k = " << k << endl;
			int y = (fgGetInt("/sim/startup/ysize") - 200 - 20 - k*20);
			ATCMenuBox = new puDialogBox (100, y);
			ATCMenuFrame      =  new puFrame (0,0,xsize,yc+40);
			// loop over all entries in atcmentrylist
			ATCOptionsList = new puButtonBox (50, 50, 450, 50+(k*25), NULL, true);
			char** optList = new char*[k+1];
			int kk = 0;
			for ( ; current != last ; ++current ) {
				string dum;
				sprintf( buf, "%i", kk+1 );
				buf[1] = '\0';
				dum = (string)(buf);
				mentry[kk] = dum + ". " + current->menuentry;
				optList[kk] = new char[strlen(mentry[kk].c_str()) + 1];
				strcpy(optList[kk], mentry[kk].c_str());
				//cout << "optList[" << kk << "] = " << optList[kk] << endl; 
				mtrans[kk] =              current->transmission;
				//ATCMenuBoxMessage =  new puText (10, yc);
				//ATCMenuBoxMessage ->     setLabel( mentry[kk].c_str() );
				yc += 20;
				++kk;
			} 
			yc += 2*20;
			optList[k] = NULL;
			ATCOptionsList->newList(optList);
		} else {
			int y = (fgGetInt("/sim/startup/ysize") - 100 - 20 );
			ATCMenuBox = new puDialogBox (10, y);
			ATCMenuFrame      =  new puFrame (0,0,xsize,yc+40);
			ATCMenuBoxMessage =  new puText (10, yc-10);
			ATCMenuBoxMessage ->     setLabel( "No transmission available" );
		}
		
		ATCMenuBoxMessage =  new puText (10, yc+10);
		ATCMenuBoxMessage ->     setLabel( "ATC Menu" );
		atcDialogOkButton     =  new puOneShot         ((xsize/2)-85, 10, (xsize/2)-25, 50);
		atcDialogOkButton     ->     setLegend         (gui_msg_OK);
		atcDialogOkButton     ->     makeReturnDefault (TRUE);
		atcDialogOkButton     ->     setCallback       (AKATCDialogOK);
		
		atcDialogCancelButton =  new puOneShot         ((xsize/2)+25, 10, (xsize/2)+85, 50);
		atcDialogCancelButton ->     setLegend         (gui_msg_CANCEL);
		atcDialogCancelButton ->     setCallback       (AKATCDialogCancel);
		FG_FINALIZE_PUI_DIALOG( ATCMenuBox );
		FG_PUSH_PUI_DIALOG( ATCMenuBox );
		
		
		/*	
		if ( atckey != -1 && TransDisplayed && mtrans[atckey-1].c_str() != "" ) {
			cout << mtrans[atckey-1].c_str() << endl;
			TPar = current_transmissionlist->extract_transpar( mtrans[atckey-1].c_str() );
			current_atcmentrylist->reset = true;
			current_transparlist->add_entry( TPar );
			
			//    transpar_list_type test = current_transparlist;
			// transpar_list_iterator current = test.begin();
			//for ( ; current != test.end(); ++current ) {
				// current->tpar.intention;
			//}
		}
		
		if ( current_atcmentrylist->freq != (int)(comm1_freq*100.0 + 0.5) ) {
			current_atcmentrylist->reset = true;
		}
		
		// reset (delete) ATCmenue list if reset is true
		if ( current_atcmentrylist->reset == true ) {
			delete ( current_atcmentrylist );
			current_atcmentrylist = new FGatcmentryList;
			current_atcmentrylist->init( (int)(comm1_freq*100.0 + 0.5) );
			if ( TransDisplayed ) {
				FG_POP_PUI_DIALOG( ATCMenuBox ); 
				TransDisplayed = false;
			}
		}
		*/	
	}
}

