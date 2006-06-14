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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
#include <stdio.h>              // char related functions
#include <string.h>             // strcmp()

#include SG_GLU_H

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Aircraft/aircraft.hxx>
#include <Autopilot/xmlauto.hxx>
#include <GUI/new_gui.hxx>           // FGFontCache
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Scenery/scenery.hxx>

#include "hud.hxx"


static HUD_Properties *HUDprop = 0;

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
fntTexFont *HUD_Font = 0;
float HUD_TextSize = 0;
int HUD_style = 0;

float HUD_matrix[16];

static string  name;
static string  label_format;
static string  prelabel;
static string  postlabel;
static string  type;
static string  type_pointer;
static string  type_tick;
static string  length_tick;

int readHud( istream &input );
int readInstrument ( const SGPropertyNode * node);

static void drawHUD();
static void fgUpdateHUDVirtual();

class locRECT {
public:
    RECT rect;

    locRECT( UINT left, UINT top, UINT right, UINT bottom);
    RECT get_rect(void) { return rect; }
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
            HIptr = static_cast<instr_item *>(new HudLadder(ladder_group->getChild(j)));
            HUD_deque.insert(HUD_deque.begin(), HIptr);
        }
    }

    const SGPropertyNode * card_group = node->getNode("cards");
    if (card_group != 0) {
        int nCards = card_group->nChildren();
        for (int j = 0; j < nCards; j++) {
            const char *type = card_group->getChild(j)->getStringValue("type", "gauge");
            if (!strcmp(type, "gauge"))
                HIptr = static_cast<instr_item *>(new gauge_instr(card_group->getChild(j)));
            else if (!strcmp(type, "dial") || !strcmp(type, "tape"))
                HIptr = static_cast<instr_item *>(new hud_card(card_group->getChild(j)));
            HUD_deque.insert(HUD_deque.begin(), HIptr);
        }
    }

    const SGPropertyNode * label_group = node->getNode("labels");
    if (label_group != 0) {
        int nLabels = label_group->nChildren();
        for (int j = 0; j < nLabels; j++) {
            HIptr = static_cast<instr_item *>(new instr_label(label_group->getChild(j)));
            HUD_deque.insert(HUD_deque.begin(), HIptr);
        }
    }

    const SGPropertyNode * tbi_group = node->getNode("tbis");
    if (tbi_group != 0) {
        int nTbis = tbi_group->nChildren();
        for (int j = 0; j < nTbis; j++) {
            HIptr = static_cast<instr_item *>(new fgTBI_instr(tbi_group->getChild(j)));
            HUD_deque.insert(HUD_deque.begin(), HIptr);
        }
    }

    const SGPropertyNode * rwy_group = node->getNode("runways");
    if (rwy_group != 0) {
        int nRwy = rwy_group->nChildren();
        for (int j = 0; j < nRwy; j++) {
            HIptr = static_cast<instr_item *>(new runway_instr(rwy_group->getChild(j)));
            HUD_deque.insert(HUD_deque.begin(), HIptr);
        }
    }
    return 0;
} //end readinstrument


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


    HUD_deque.erase( HUD_deque.begin(), HUD_deque.end());


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

    FGFontCache *fc = globals->get_fontcache();
    HUD_Font = fc->getTexFont(fgGetString("/sim/hud/font/name", "Helvetica.txf"));
    if (!HUD_Font)
        throw sg_throwable(string("/sim/hud/font/name is not a texture font"));

    HUD_TextSize = fgGetFloat("/sim/hud/font/size", 10);

    HUDtext = new fntRenderer();
    HUDtext->setFont(HUD_Font);
    HUDtext->setPointSize(HUD_TextSize);
    HUD_TextList.setFont( HUDtext );

    if (!HUDprop)
        HUDprop = new HUD_Properties;
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

    if (!HUDprop)
        HUDprop = new HUD_Properties;
    return 0;  // For now. Later we may use this for an error code.

}
//$$$ End - added, Neetha, 28 Nov 2k


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


// fgUpdateHUD
//
// Performs a once around the list of calls to instruments installed in
// the HUD object with requests for redraw. Kinda. It will when this is
// all C++.
//
void fgUpdateHUD( void ) {

    static const SGPropertyNode *enable3d_node = fgGetNode("/sim/hud/enable3d");
    if ( HUD_style == 1 && enable3d_node->getBoolValue() ) {
        fgUpdateHUDVirtual();
        return;
    }

    static const float normal_aspect = float(640) / float(480);
    // note: aspect_ratio is Y/X
    float current_aspect = 1.0f/globals->get_current_view()->get_aspect_ratio();
    if ( current_aspect > normal_aspect ) {
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
    if (fabs(lookat[1]) > 9999)
        lookat[1] = 9999; // FPU sanity
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
    if ( !HUD_deque.size() ) // Trust everyone, but ALWAYS cut the cards!
        return;

    HUD_TextList.erase();
    HUD_LineList.erase();
    // HUD_StippleLineList.erase();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    static const SGPropertyNode *heading_enabled
        = fgGetNode("/autopilot/locks/heading", true);
    static const SGPropertyNode *altitude_enabled
        = fgGetNode("/autopilot/locks/altitude", true);

    static char hud_hdg_text[256];
    static char hud_wp0_text[256];
    static char hud_wp1_text[256];
    static char hud_wp2_text[256];
    static char hud_alt_text[256];

    glEnable(GL_BLEND);
    if (HUDprop->isTransparent())
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (HUDprop->isAntialiased()) {
        glEnable(GL_LINE_SMOOTH);
        glAlphaFunc(GL_GREATER, HUDprop->alphaClamp());
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        //glLineWidth(1.5);
    } else {
        //glLineWidth(1.0);
    }

    HUDprop->setColor();
    for_each(HUD_deque.begin(), HUD_deque.end(), HUDdraw());

    //HUD_TextList.add( fgText(40, 10, get_formated_gmt_time(), 0) );


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

    if (HUDprop->isAntialiased()) {
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_LINE_SMOOTH);
        //glLineWidth(1.0);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}



void fgTextList::draw()
{
    if (!Font)
        return;

    vector<fgText>::iterator curString = List.begin();
    vector<fgText>::iterator lastString = List.end();

    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    if (HUDprop->isTransparent())
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (HUDprop->isAntialiased()) {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, HUDprop->alphaClamp());
    }

    Font->begin();
    for (; curString != lastString; curString++)
        curString->Draw(Font,curString->digit);
    Font->end();

    glDisable(GL_TEXTURE_2D);
    glPopAttrib();
}



// HUD property listener class
//
HUD_Properties::HUD_Properties() :
    _current(fgGetNode("/sim/hud/current-color", true)),
    _visibility(fgGetNode("/sim/hud/visibility", true)),
    _antialiasing(fgGetNode("/sim/hud/color/antialiased", true)),
    _transparency(fgGetNode("/sim/hud/color/transparent", true)),
    _red(fgGetNode("/sim/hud/color/red", true)),
    _green(fgGetNode("/sim/hud/color/green", true)),
    _blue(fgGetNode("/sim/hud/color/blue", true)),
    _alpha(fgGetNode("/sim/hud/color/alpha", true)),
    _alpha_clamp(fgGetNode("/sim/hud/color/alpha-clamp", true)),
    _brightness(fgGetNode("/sim/hud/color/brightness", true)),
    _visible(false),
    _antialiased(false),
    _transparent(false),
    _a(0.67),
    _cl(0.01)
{
    _visibility->addChangeListener(this);
    _antialiasing->addChangeListener(this);
    _transparency->addChangeListener(this);
    _red->addChangeListener(this);
    _green->addChangeListener(this);
    _blue->addChangeListener(this);
    _alpha->addChangeListener(this);
    _alpha_clamp->addChangeListener(this);
    _brightness->addChangeListener(this);
    _current->addChangeListener(this, true);
}


void HUD_Properties::valueChanged(SGPropertyNode *node)
{
    if (!strcmp(node->getName(), "current-color")) {
        int i = node->getIntValue();
        if (i < 0)
            i = 0;
        SGPropertyNode *n = fgGetNode("/sim/hud/palette", true);
        if ((n = n->getChild("color", i, false))) {
            if (n->hasValue("red"))
                _red->setFloatValue(n->getFloatValue("red", 1.0));
            if (n->hasValue("green"))
                _green->setFloatValue(n->getFloatValue("green", 1.0));
            if (n->hasValue("blue"))
                _blue->setFloatValue(n->getFloatValue("blue", 1.0));
            if (n->hasValue("alpha"))
                _alpha->setFloatValue(n->getFloatValue("alpha", 0.67));
            if (n->hasValue("alpha-clamp"))
                _alpha_clamp->setFloatValue(n->getFloatValue("alpha-clamp", 0.01));
            if (n->hasValue("brightness"))
                _brightness->setFloatValue(n->getFloatValue("brightness", 0.75));
            if (n->hasValue("antialiased"))
                _antialiasing->setBoolValue(n->getBoolValue("antialiased", false));
            if (n->hasValue("transparent"))
                _transparency->setBoolValue(n->getBoolValue("transparent", false));
        }
    }
    _visible = _visibility->getBoolValue();
    _transparent = _transparency->getBoolValue();
    _antialiased = _antialiasing->getBoolValue();
    float brt = _brightness->getFloatValue();
    _r = clamp(brt * _red->getFloatValue());
    _g = clamp(brt * _green->getFloatValue());
    _b = clamp(brt * _blue->getFloatValue());
    _a = clamp(_alpha->getFloatValue());
    _cl = clamp(_alpha_clamp->getFloatValue());
}


void HUD_Properties::setColor() const
{
    if (_antialiased)
        glColor4f(_r, _g, _b, _a);
    else
        glColor3f(_r, _g, _b);
}

