// preset_dlg.cxx -- Preset dialogs and funcitons
//
// Written by Curtis Olson, started November 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/structure/commands.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/fg_props.hxx>

#include "gui.h"
#include "preset_dlg.hxx"


static puDialogBox *PresetDialog = 0;
static puFrame     *PresetDialogFrame = 0;
static puText      *PresetDialogMessage = 0;
static puInput     *PresetDialogInput = 0;

static const int MAX_VALUE = 16;
static char PresetValue[MAX_VALUE];
static char PresetSavedValue[MAX_VALUE];
static char PresetLabel[] = "Enter New Airport ID"; 
static string PresetProperty = "";

static puOneShot *PresetDialogOkButton = 0;
static puOneShot *PresetDialogCancelButton = 0;
static puOneShot *PresetDialogResetButton = 0;


static void PresetDialog_OK(puObject *)
{
    char *value;
    PresetDialogInput->getValue(&value);
    SG_LOG( SG_GENERAL, SG_DEBUG, "setting " << PresetProperty
            << " = " << value );
    fgSetString( PresetProperty.c_str(), value );
    FG_POP_PUI_DIALOG( PresetDialog );

    // consistancy handling for some specialized cases
    if ( PresetProperty == "/sim/presets/airport-id" ) {
        fgSetDouble("/sim/presets/longitude-deg", -9999.0 );
        fgSetDouble("/sim/presets/latitude-deg", -9999.0 );
    } else if ( PresetProperty == "/sim/presets/runway" ) {
        fgSetDouble("/sim/presets/longitude-deg", -9999.0 );
        fgSetDouble("/sim/presets/latitude-deg", -9999.0 );
    } else if ( PresetProperty == "/sim/presets/offset-distance" ) {
        if ( fabs(fgGetDouble("/sim/presets/altitude-ft")) > 0.000001
             && fabs(fgGetDouble("/sim/presets/glideslope-deg")) > 0.000001 ) {
            fgSetDouble("/sim/presets/altitude-ft", -9999.0);
            SG_LOG( SG_GENERAL, SG_DEBUG, "nuking altitude" );
        }
    } else if ( PresetProperty == "/sim/presets/altitude-ft" ) {
        if ( fabs(fgGetDouble("/sim/presets/offset-distance")) > 0.000001
             && fabs(fgGetDouble("/sim/presets/glideslope-deg")) > 0.000001 ) {
            fgSetDouble("/sim/presets/offset-distance", 0.0);
            SG_LOG( SG_GENERAL, SG_DEBUG, "nuking offset distance" );
        }
    } else if ( PresetProperty == "/sim/presets/glideslope-deg" ) {
        if ( fabs(fgGetDouble("/sim/presets/offset-distance")) > 0.000001
             && fabs(fgGetDouble("/sim/presets/altitude-ft")) > 0.000001 ) {
            fgSetDouble("/sim/presets/altitude-ft", -9999.0);
            SG_LOG( SG_GENERAL, SG_DEBUG, "nuking altitude" );
        }
    }
}


static void PresetDialog_Cancel(puObject *)
{
    FG_POP_PUI_DIALOG( PresetDialog );
}


static void PresetDialog_Reset(puObject *)
{
    PresetDialogInput->setValue( PresetSavedValue );
    PresetDialogInput->setCursor( 0 ) ;
}


// Initialize the preset dialog box
void fgPresetInit()
{
    sprintf( PresetValue, "%s", fgGetString("/sim/presets/airport-id") );
    int len = 150
        - puGetDefaultLabelFont().getStringWidth( PresetLabel ) / 2;

    PresetDialog = new puDialogBox (150, 50);
    {
        PresetDialogFrame = new puFrame           (0,0,350, 150);
        PresetDialogMessage = new puText            (len, 110);
        PresetDialogMessage -> setLabel          ("");

        PresetDialogInput = new puInput           (50, 70, 300, 100);
        PresetDialogInput -> setValue          ("");
        PresetDialogInput -> acceptInput();

        PresetDialogOkButton = new puOneShot   (50, 10, 110, 50);
        PresetDialogOkButton -> setLegend(gui_msg_OK);
        PresetDialogOkButton -> setCallback (PresetDialog_OK);
        PresetDialogOkButton -> makeReturnDefault(TRUE);

        PresetDialogCancelButton =  new puOneShot (140, 10, 210, 50);
        PresetDialogCancelButton -> setLegend (gui_msg_CANCEL);
        PresetDialogCancelButton -> setCallback (PresetDialog_Cancel);

        PresetDialogResetButton =  new puOneShot (240, 10, 300, 50);
        PresetDialogResetButton -> setLegend (gui_msg_RESET);
        PresetDialogResetButton -> setCallback (PresetDialog_Reset);
    }
    SG_LOG( SG_GENERAL, SG_DEBUG, "PresetInit " << PresetValue );
    FG_FINALIZE_PUI_DIALOG( PresetDialog );
}


void fgPresetAirport(puObject *cb)
{
    PresetDialogMessage -> setLabel( "Enter Airport ID:" );
    PresetProperty = "/sim/presets/airport-id";
    snprintf( PresetValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    snprintf( PresetSavedValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    PresetDialogInput->setValue( PresetValue );

    FG_PUSH_PUI_DIALOG( PresetDialog );
}


void fgPresetRunway(puObject *cb)
{
    PresetDialogMessage -> setLabel( "Enter Runway Number:" );
    PresetProperty = "/sim/presets/runway";
    snprintf( PresetValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    snprintf( PresetSavedValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    PresetDialogInput->setValue( PresetValue );

    FG_PUSH_PUI_DIALOG( PresetDialog );
}


void fgPresetOffsetDistance(puObject *cb)
{
    PresetDialogMessage -> setLabel( "Enter Offset Distance (miles):" );
    PresetProperty = "/sim/presets/offset-distance";
    snprintf( PresetValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    snprintf( PresetSavedValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    PresetDialogInput->setValue( PresetValue );

    FG_PUSH_PUI_DIALOG( PresetDialog );
}


void fgPresetAltitude(puObject *cb)
{
    PresetDialogMessage -> setLabel( "Enter Altitude (feet):" );
    PresetProperty = "/sim/presets/altitude-ft";
    snprintf( PresetValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    snprintf( PresetSavedValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    PresetDialogInput->setValue( PresetValue );

    FG_PUSH_PUI_DIALOG( PresetDialog );
}


void fgPresetGlideslope(puObject *cb)
{
    PresetDialogMessage -> setLabel( "Enter Glideslope (deg):" );
    PresetProperty = "/sim/presets/glideslope-deg";
    snprintf( PresetValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    snprintf( PresetSavedValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    PresetDialogInput->setValue( PresetValue );

    FG_PUSH_PUI_DIALOG( PresetDialog );
}


void fgPresetAirspeed(puObject *cb)
{
    PresetDialogMessage -> setLabel( "Enter Airspeed (kts):" );
    PresetProperty = "/sim/presets/airspeed-kt";
    snprintf( PresetValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    snprintf( PresetSavedValue, MAX_VALUE, "%s",
              fgGetString(PresetProperty.c_str()) );
    PresetDialogInput->setValue( PresetValue );

    FG_PUSH_PUI_DIALOG( PresetDialog );
}


void fgPresetCommit(puObject *)
{
    SGPropertyNode args;
    if ( !globals->get_commands()->execute("presets-commit", &args) )
    {
        SG_LOG( SG_GENERAL, SG_ALERT, "Command: presets-commit failed.");
    }
}


