
#ifndef _MENU_CROLL_H_
#define _MENU_CROLL_H_

#include "uiuc_aircraft.h"
#include "uiuc_convert.h"
#include "uiuc_1DdataFileReader.h"
#include "uiuc_2DdataFileReader.h"
#include "uiuc_menu_functions.h"
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_cockpit.h>    /* Long_trim defined */
#include <FDM/LaRCsim/ls_constants.h>  /* INVG defined */

void parse_Cl( const string& linetoken2, const string& linetoken3,
	       const string& linetoken4, const string& linetoken5, 
	       const string& linetoken6, const string& linetoken7,
	       const string& linetoken8, const string& linetoken9,
	       const string& linetoken10, const string& aircraft_directory,
	       LIST command_line );

#endif //_MENU_CROLL_H_
