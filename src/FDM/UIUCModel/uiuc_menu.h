
#ifndef _MENU_H_
#define _MENU_H_

#include "uiuc_aircraft.h"
#include "uiuc_convert.h"
#include "uiuc_parsefile.h"
#include "uiuc_warnings_errors.h"
#include "uiuc_initializemaps.h"
#include "uiuc_1DdataFileReader.h"
#include "uiuc_2DdataFileReader.h"
#include "../FDM/LaRCsim/ls_generic.h"
#include "../FDM/LaRCsim/ls_cockpit.h"    /* Long_trim defined */
#include "../FDM/LaRCsim/ls_constants.h"  /* INVG defined */

bool check_float(string  &token); // To check whether the token is a float or not
void uiuc_menu (string aircraft);

#endif //_MENU_H_
