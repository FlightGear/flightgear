
#ifndef _MENU_H_
#define _MENU_H_

#include "uiuc_aircraft.h"
#include "uiuc_convert.h"
#include "uiuc_parsefile.h"
#include "uiuc_warnings_errors.h"
#include "uiuc_initializemaps.h"
#include "uiuc_1DdataFileReader.h"
#include "uiuc_2DdataFileReader.h"
#include <FDM/LaRCsim/ls_generic.h>
#include <FDM/LaRCsim/ls_cockpit.h>    /* Long_trim defined */
#include <FDM/LaRCsim/ls_constants.h>  /* INVG defined */
#include "uiuc_flapdata.h"
#include "uiuc_menu_init.h"
#include "uiuc_menu_geometry.h"
#include "uiuc_menu_controlSurface.h"
#include "uiuc_menu_mass.h"
#include "uiuc_menu_engine.h"
#include "uiuc_menu_CD.h"
#include "uiuc_menu_CL.h"
#include "uiuc_menu_Cm.h"
#include "uiuc_menu_CY.h"
#include "uiuc_menu_Croll.h"
#include "uiuc_menu_Cn.h"
#include "uiuc_menu_gear.h"
#include "uiuc_menu_ice.h"
#include "uiuc_menu_fog.h"
#include "uiuc_menu_record.h"
#include "uiuc_menu_misc.h"

//bool check_float(const string &token); // To check whether the token is a float or not
void uiuc_menu (string aircraft);

#endif //_MENU_H_
