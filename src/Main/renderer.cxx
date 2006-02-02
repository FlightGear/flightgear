// renderer.cxx -- top level sim routines
//
// Written by Curtis Olson, started May 1997.
// This file contains parts of main.cxx prior to october 2004
//
// Copyright (C) 1997 - 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <plib/ssg.h>
#include <plib/netSocket.h>

#include <simgear/screen/extensions.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/scene/model/animation.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/model/model.hxx>
#ifdef FG_JPEG_SERVER
#include <simgear/screen/jpgfactory.hxx>
#endif

#include <simgear/environment/visual_enviro.hxx>

#include <simgear/scene/model/shadowvolume.hxx>

#include <Scenery/tileentry.hxx>
#include <Time/light.hxx>
#include <Time/light.hxx>
#include <Aircraft/aircraft.hxx>
// #include <Aircraft/replay.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/cockpit.hxx>
#include <Cockpit/hud.hxx>
#include <Model/panelnode.hxx>
#include <Model/modelmgr.hxx>
#include <Model/acmodel.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Scripting/NasalDisplay.hxx>
#include <ATC/ATCdisplay.hxx>
#include <GUI/new_gui.hxx>

#include "splash.hxx"
#include "renderer.hxx"
#include "main.hxx"


extern void sgShaderFrameInit(double delta_time_sec);

float default_attenuation[3] = {1.0, 0.0, 0.0};

ssgSelector *lightpoints_brightness = new ssgSelector;
ssgTransform *lightpoints_transform = new ssgTransform;
FGTileEntry *dummy_tile;
sgVec3 rway_ols;

// Clip plane settings...
float scene_nearplane = 0.5f;
float scene_farplane = 120000.0f;

glPointParameterfProc glPointParameterfPtr = 0;
glPointParameterfvProc glPointParameterfvPtr = 0;
bool glPointParameterIsSupported = false;
bool glPointSpriteIsSupported = false;


// fog constants.  I'm a little nervous about putting actual code out
// here but it seems to work (?)
static const double m_log01 = -log( 0.01 );
static const double sqrt_m_log01 = sqrt( m_log01 );
static GLfloat fog_exp_density;
static GLfloat fog_exp2_density;
static GLfloat rwy_exp2_punch_through;
static GLfloat taxi_exp2_punch_through;
static GLfloat ground_exp2_punch_through;

// Sky structures
SGSky *thesky;

// hack
sgMat4 copy_of_ssgOpenGLAxisSwapMatrix =
{
  {  1.0f,  0.0f,  0.0f,  0.0f },
  {  0.0f,  0.0f, -1.0f,  0.0f },
  {  0.0f,  1.0f,  0.0f,  0.0f },
  {  0.0f,  0.0f,  0.0f,  1.0f }
};

ssgSimpleState *cloud3d_imposter_state;
ssgSimpleState *default_state;
ssgSimpleState *hud_and_panel;
ssgSimpleState *menus;

SGShadowVolume *shadows;

FGRenderer::FGRenderer()
{
#ifdef FG_JPEG_SERVER
   jpgRenderFrame = FGRenderer::update;
#endif
}

FGRenderer::~FGRenderer()
{
#ifdef FG_JPEG_SERVER
   jpgRenderFrame = NULL;
#endif
}


void
FGRenderer::build_states( void ) {
    default_state = new ssgSimpleState;
    default_state->ref();
    default_state->disable( GL_TEXTURE_2D );
    default_state->enable( GL_CULL_FACE );
    default_state->enable( GL_COLOR_MATERIAL );
    default_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    default_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    default_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    default_state->disable( GL_BLEND );
    default_state->disable( GL_ALPHA_TEST );
    default_state->disable( GL_LIGHTING );

    cloud3d_imposter_state = new ssgSimpleState;
    cloud3d_imposter_state->ref();
    cloud3d_imposter_state->enable( GL_TEXTURE_2D );
    cloud3d_imposter_state->enable( GL_CULL_FACE );
    cloud3d_imposter_state->enable( GL_COLOR_MATERIAL );
    cloud3d_imposter_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    cloud3d_imposter_state->setMaterial( GL_DIFFUSE, 1, 1, 1, 1 );
    cloud3d_imposter_state->setMaterial( GL_AMBIENT, 1, 1, 1, 1 );
    cloud3d_imposter_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    cloud3d_imposter_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    cloud3d_imposter_state->enable( GL_BLEND );
    cloud3d_imposter_state->enable( GL_ALPHA_TEST );
    cloud3d_imposter_state->disable( GL_LIGHTING );

    hud_and_panel = new ssgSimpleState;
    hud_and_panel->ref();
    hud_and_panel->disable( GL_CULL_FACE );
    hud_and_panel->disable( GL_TEXTURE_2D );
    hud_and_panel->disable( GL_LIGHTING );
    hud_and_panel->enable( GL_BLEND );

    menus = new ssgSimpleState;
    menus->ref();
    menus->disable( GL_CULL_FACE );
    menus->disable( GL_TEXTURE_2D );
    menus->enable( GL_BLEND );

    shadows = new SGShadowVolume( globals->get_scenery()->get_scene_graph() );
    shadows->init( fgGetNode("/sim/rendering", true) );
    shadows->addOccluder( globals->get_scenery()->get_aircraft_branch(), SGShadowVolume::occluderTypeAircraft );

}


// Initialize various GL/view parameters
void
FGRenderer::init( void ) {

    FGLight *l = (FGLight *)(globals->get_subsystem("lighting"));

    // Go full screen if requested ...
    if ( fgGetBool("/sim/startup/fullscreen") ) {
        fgOSFullScreen();
    }

    // If enabled, normal vectors specified with glNormal are scaled
    // to unit length after transformation.  Enabling this has
    // performance implications.  See the docs for glNormal.
    // glEnable( GL_NORMALIZE );

    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );
    // glLightfv( GL_LIGHT0, GL_POSITION, l->sun_vec );  // done later with ssg

    sgVec3 sunpos;
    sgSetVec3( sunpos, l->sun_vec()[0], l->sun_vec()[1], l->sun_vec()[2] );
    ssgGetLight( 0 ) -> setPosition( sunpos );

    glFogi (GL_FOG_MODE, GL_EXP2);
    if ( (!strcmp(fgGetString("/sim/rendering/fog"), "disabled")) || 
         (!fgGetBool("/sim/rendering/shading"))) {
        // if fastest fog requested, or if flat shading force fastest
        glHint ( GL_FOG_HINT, GL_FASTEST );
    } else if ( !strcmp(fgGetString("/sim/rendering/fog"), "nicest") ) {
        glHint ( GL_FOG_HINT, GL_DONT_CARE );
    }
    if ( fgGetBool("/sim/rendering/wireframe") ) {
        // draw wire frame
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }

    // This is the default anyways, but it can't hurt
    glFrontFace ( GL_CCW );

    // Just testing ...
    if ( SGIsOpenGLExtensionSupported("GL_ARB_point_sprite") ||
         SGIsOpenGLExtensionSupported("GL_NV_point_sprite") )
    {
        GLuint handle = thesky->get_sun_texture_id();
        glBindTexture ( GL_TEXTURE_2D, handle ) ;
        glTexEnvf(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
        glEnable(GL_POINT_SPRITE);
        // glEnable(GL_POINT_SMOOTH);
        glPointSpriteIsSupported = true;
    }
    glEnable(GL_LINE_SMOOTH);
    // glEnable(GL_POLYGON_SMOOTH);      
    glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glHint(GL_POINT_SMOOTH_HINT, GL_DONT_CARE);
}



// Update all Visuals (redraws anything graphics related)
void
FGRenderer::update( bool refresh_camera_settings ) {
    bool scenery_loaded = fgGetBool("sim/sceneryloaded") \
                          || fgGetBool("sim/sceneryloaded-override");

    if ( idle_state < 1000 || !scenery_loaded ) {
        // still initializing, draw the splash screen
        fgSplashUpdate(1.0);

        // Keep resetting sim time while the sim is initializing
        globals->set_sim_time_sec( 0.0 );
        SGAnimation::set_sim_time_sec( 0.0 );
        return;
    }

    bool draw_otw = fgGetBool("/sim/rendering/draw-otw");
    bool skyblend = fgGetBool("/sim/rendering/skyblend");
    bool enhanced_lighting = fgGetBool("/sim/rendering/enhanced-lighting");
    bool distance_attenuation = fgGetBool("/sim/rendering/distance-attenuation");
    bool volumetric_clouds = sgEnviro.get_clouds_enable_state();
#ifdef FG_ENABLE_MULTIPASS_CLOUDS
    bool multi_pass_clouds = fgGetBool("/sim/rendering/multi-pass-clouds") && 
                                !volumetric_clouds &&
                                !SGCloudLayer::enable_bump_mapping;  // ugly artefact now
#else
    bool multi_pass_clouds = false;
#endif
    bool draw_clouds = fgGetBool("/environment/clouds/status");

    GLfloat black[4] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };

    // static const SGPropertyNode *longitude
    //     = fgGetNode("/position/longitude-deg");
    // static const SGPropertyNode *latitude
    //     = fgGetNode("/position/latitude-deg");
    // static const SGPropertyNode *altitude
    //     = fgGetNode("/position/altitude-ft");
    static const SGPropertyNode *groundlevel_nearplane
        = fgGetNode("/sim/current-view/ground-level-nearplane-m");

    FGLight *l = (FGLight *)(globals->get_subsystem("lighting"));
    static double last_visibility = -9999;

    // update fog params
    double actual_visibility;
    if (fgGetBool("/environment/clouds/status")) {
        actual_visibility = thesky->get_visibility();
    } else {
        actual_visibility = fgGetDouble("/environment/visibility-m");
    }

        // TODO:TEST only, don't commit that !!
//      sgFXperFrameInit();

    sgShaderFrameInit(delta_time_sec);

    if ( actual_visibility != last_visibility ) {
        last_visibility = actual_visibility;

        fog_exp_density = m_log01 / actual_visibility;
        fog_exp2_density = sqrt_m_log01 / actual_visibility;
        ground_exp2_punch_through = sqrt_m_log01 / (actual_visibility * 1.5);
        if ( actual_visibility < 8000 ) {
            rwy_exp2_punch_through = sqrt_m_log01 / (actual_visibility * 2.5);
            taxi_exp2_punch_through = sqrt_m_log01 / (actual_visibility * 1.5);
        } else {
            rwy_exp2_punch_through = sqrt_m_log01 / ( 8000 * 2.5 );
            taxi_exp2_punch_through = sqrt_m_log01 / ( 8000 * 1.5 );
        }
    }

    // double angle;
    // GLfloat black[4] = { 0.0, 0.0, 0.0, 1.0 };
    // GLfloat white[4] = { 1.0, 1.0, 1.0, 1.0 };
    // GLfloat terrain_color[4] = { 0.54, 0.44, 0.29, 1.0 };
    // GLfloat mat_shininess[] = { 10.0 };
    GLbitfield clear_mask;

    // idle_state is now 1000 meaning we've finished all our
    // initializations and are running the main loop, so this will
    // now work without seg faulting the system.

    FGViewer *current__view = globals->get_current_view();

    // calculate our current position in cartesian space
    Point3D cntr = globals->get_scenery()->get_next_center();
    globals->get_scenery()->set_center(cntr);
    // Force update of center dependent values ...
    current__view->set_dirty();

    if ( refresh_camera_settings ) {
        // update view port
        resize( fgGetInt("/sim/startup/xsize"),
                fgGetInt("/sim/startup/ysize") );

        // Tell GL we are switching to model view parameters
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        ssgSetCamera( (sgVec4 *)current__view->get_VIEW() );
    }

    clear_mask = GL_DEPTH_BUFFER_BIT;
    if ( fgGetBool("/sim/rendering/wireframe") ) {
        clear_mask |= GL_COLOR_BUFFER_BIT;
    }

    if ( skyblend ) {
        if ( fgGetBool("/sim/rendering/textures") ) {
        // glClearColor(black[0], black[1], black[2], black[3]);
        glClearColor(l->adj_fog_color()[0], l->adj_fog_color()[1],
                     l->adj_fog_color()[2], l->adj_fog_color()[3]);
        clear_mask |= GL_COLOR_BUFFER_BIT;
        }
    } else {
        glClearColor(l->sky_color()[0], l->sky_color()[1],
                     l->sky_color()[2], l->sky_color()[3]);
        clear_mask |= GL_COLOR_BUFFER_BIT;
    }
    if ( multi_pass_clouds && draw_clouds ) {
        glClearStencil( 0 );
        clear_mask |= GL_STENCIL_BUFFER_BIT;
    }
    glClear( clear_mask );

    // set the opengl state to known default values
    default_state->force();

    // update fog params if visibility has changed
    double visibility_meters = fgGetDouble("/environment/visibility-m");
    thesky->set_visibility(visibility_meters);

    thesky->modify_vis( cur_fdm_state->get_Altitude() * SG_FEET_TO_METER,
                        ( global_multi_loop * fgGetInt("/sim/speed-up") )
                        / (double)fgGetInt("/sim/model-hz") );

    // Set correct opengl fog density
    glFogf (GL_FOG_DENSITY, fog_exp2_density);

    // update the sky dome
    if ( skyblend ) {
        /*
         SG_LOG( SG_GENERAL, SG_BULK, "thesky->repaint() sky_color = "
         << l->sky_color()[0] << " "
         << l->sky_color()[1] << " "
         << l->sky_color()[2] << " "
         << l->sky_color()[3] );
        SG_LOG( SG_GENERAL, SG_BULK, "    fog = "
         << l->fog_color()[0] << " "
         << l->fog_color()[1] << " "
         << l->fog_color()[2] << " "
         << l->fog_color()[3] ); 
        SG_LOG( SG_GENERAL, SG_BULK,
                "    sun_angle = " << l->sun_angle
         << "    moon_angle = " << l->moon_angle );
        */

        static SGSkyColor scolor;
//        FGLight *l = (FGLight *)(globals->get_subsystem("lighting"));

        scolor.sky_color   = l->sky_color();
        scolor.fog_color   = l->adj_fog_color();
        scolor.cloud_color = l->cloud_color();
        scolor.sun_angle   = l->get_sun_angle();
        scolor.moon_angle  = l->get_moon_angle();
        scolor.nplanets    = globals->get_ephem()->getNumPlanets();
        scolor.nstars      = globals->get_ephem()->getNumStars();
        scolor.planet_data = globals->get_ephem()->getPlanets();
        scolor.star_data   = globals->get_ephem()->getStars();

        thesky->repaint( scolor );

        /*
        SG_LOG( SG_GENERAL, SG_BULK,
                "thesky->reposition( view_pos = " << view_pos[0] << " "
         << view_pos[1] << " " << view_pos[2] );
        SG_LOG( SG_GENERAL, SG_BULK,
                "    zero_elev = " << zero_elev[0] << " "
         << zero_elev[1] << " " << zero_elev[2]
         << " lon = " << cur_fdm_state->get_Longitude()
         << " lat = " << cur_fdm_state->get_Latitude() );
        SG_LOG( SG_GENERAL, SG_BULK,
                "    sun_rot = " << l->get_sun_rotation
         << " gst = " << SGTime::cur_time_params->getGst() );
        SG_LOG( SG_GENERAL, SG_BULK,
             "    sun ra = " << globals->get_ephem()->getSunRightAscension()
          << " sun dec = " << globals->get_ephem()->getSunDeclination()
          << " moon ra = " << globals->get_ephem()->getMoonRightAscension()
          << " moon dec = " << globals->get_ephem()->getMoonDeclination() );
        */

        // The sun and moon distances are scaled down versions
        // of the actual distance to get both the moon and the sun
        // within the range of the far clip plane.
        // Moon distance:    384,467 kilometers
        // Sun distance: 150,000,000 kilometers
        double sun_horiz_eff, moon_horiz_eff;
        if (fgGetBool("/sim/rendering/horizon-effect")) {
        sun_horiz_eff = 0.67+pow(0.5+cos(l->get_sun_angle())*2/2, 0.33)/3;
        moon_horiz_eff = 0.67+pow(0.5+cos(l->get_moon_angle())*2/2, 0.33)/3;
        } else {
           sun_horiz_eff = moon_horiz_eff = 1.0;
        }

        static SGSkyState sstate;

        sstate.view_pos  = current__view->get_view_pos();
        sstate.zero_elev = current__view->get_zero_elev();
        sstate.view_up   = current__view->get_world_up();
        sstate.lon       = current__view->getLongitude_deg()
                            * SGD_DEGREES_TO_RADIANS;
        sstate.lat       = current__view->getLatitude_deg()
                            * SGD_DEGREES_TO_RADIANS;
        sstate.alt       = current__view->getAltitudeASL_ft()
                            * SG_FEET_TO_METER;
        sstate.spin      = l->get_sun_rotation();
        sstate.gst       = globals->get_time_params()->getGst();
        sstate.sun_ra    = globals->get_ephem()->getSunRightAscension();
        sstate.sun_dec   = globals->get_ephem()->getSunDeclination();
        sstate.sun_dist  = 50000.0 * sun_horiz_eff;
        sstate.moon_ra   = globals->get_ephem()->getMoonRightAscension();
        sstate.moon_dec  = globals->get_ephem()->getMoonDeclination();
        sstate.moon_dist = 40000.0 * moon_horiz_eff;

        thesky->reposition( sstate, delta_time_sec );

        shadows->setupShadows( 
          current__view->getLongitude_deg(),
          current__view->getLatitude_deg(),
          globals->get_time_params()->getGst(),
          globals->get_ephem()->getSunRightAscension(),
          globals->get_ephem()->getSunDeclination(),
          l->get_sun_angle());
    }

    glEnable( GL_DEPTH_TEST );
    if ( strcmp(fgGetString("/sim/rendering/fog"), "disabled") ) {
        glEnable( GL_FOG );
        glFogi( GL_FOG_MODE, GL_EXP2 );
        glFogfv( GL_FOG_COLOR, l->adj_fog_color() );
    } else
        glDisable( GL_FOG ); 

    // set sun/lighting parameters
    ssgGetLight( 0 ) -> setPosition( l->sun_vec() );

    // GL_LIGHT_MODEL_AMBIENT has a default non-zero value so if
    // we only update GL_AMBIENT for our lights we will never get
    // a completely dark scene.  So, we set GL_LIGHT_MODEL_AMBIENT
    // explicitely to black.
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, black );

    ssgGetLight( 0 ) -> setColour( GL_AMBIENT, l->scene_ambient() );
    ssgGetLight( 0 ) -> setColour( GL_DIFFUSE, l->scene_diffuse() );
    ssgGetLight( 0 ) -> setColour( GL_SPECULAR, l->scene_specular() );

    sgEnviro.setLight(l->adj_fog_color());

    // texture parameters
    // glEnable( GL_TEXTURE_2D );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ) ;
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ) ;

    double agl = current__view->getAltitudeASL_ft()*SG_FEET_TO_METER
      - current__view->getSGLocation()->get_cur_elev_m();

    if ( agl > 10.0 ) {
        scene_nearplane = 10.0f;
        scene_farplane = 120000.0f;
    } else {
        scene_nearplane = groundlevel_nearplane->getDoubleValue();
        scene_farplane = 120000.0f;
    }

    setNearFar( scene_nearplane, scene_farplane );

    sgEnviro.startOfFrame(current__view->get_view_pos(), 
        current__view->get_world_up(),
        current__view->getLongitude_deg(),
        current__view->getLatitude_deg(),
        current__view->getAltitudeASL_ft() * SG_FEET_TO_METER,
        delta_time_sec);

    if ( draw_otw && skyblend ) {
        // draw the sky backdrop

        // we need a white diffuse light for the phase of the moon
        ssgGetLight( 0 ) -> setColour( GL_DIFFUSE, white );
        thesky->preDraw( cur_fdm_state->get_Altitude() * SG_FEET_TO_METER,
                         fog_exp2_density );
        // return to the desired diffuse color
        ssgGetLight( 0 ) -> setColour( GL_DIFFUSE, l->scene_diffuse() );
    }

    // draw the ssg scene
    glEnable( GL_DEPTH_TEST );

    if ( fgGetBool("/sim/rendering/wireframe") ) {
        // draw wire frame
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }
    if ( draw_otw ) {
        if ( draw_clouds ) {

            // Draw the terrain
            FGTileMgr::set_tile_filter( true );
            sgSetModelFilter( false );
            globals->get_aircraft_model()->select( false );
            ssgCullAndDraw( globals->get_scenery()->get_scene_graph() );

            // Disable depth buffer update, draw the clouds
            glDepthMask( GL_FALSE );
            if( !volumetric_clouds )
                thesky->drawUpperClouds();
            if ( multi_pass_clouds ) {
                thesky->drawLowerClouds();
            }
            glDepthMask( GL_TRUE );

            if ( multi_pass_clouds ) {
                // Draw the objects except the aircraft
                //  and update the stencil buffer with 1
                glEnable( GL_STENCIL_TEST );
                glStencilFunc( GL_ALWAYS, 1, 1 );
                glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );
            }
            FGTileMgr::set_tile_filter( false );
            sgSetModelFilter( true );
            ssgCullAndDraw( globals->get_scenery()->get_scene_graph() );
        } else {
            FGTileMgr::set_tile_filter( true );
            sgSetModelFilter( true );
            globals->get_aircraft_model()->select( false );
            ssgCullAndDraw( globals->get_scenery()->get_scene_graph() );
        }
    }

    // This is a bit kludgy.  Every 200 frames, do an extra
    // traversal of the scene graph without drawing anything, but
    // with the field-of-view set to 360x360 degrees.  This
    // ensures that out-of-range random objects that are not in
    // the current view frustum will still be freed properly.
    static int counter = 0;
    counter++;
    if (counter >= 200) {
        sgFrustum f;
        f.setFOV(360, 360);
                // No need to put the near plane too close;
                // this way, at least the aircraft can be
                // culled.
        f.setNearFar(1000, 1000000);
        sgMat4 m;
        ssgGetModelviewMatrix(m);
        FGTileMgr::set_tile_filter( true );
        sgSetModelFilter( true );
        globals->get_scenery()->get_scene_graph()->cull(&f, m, true);
        counter = 0;
    }

    // change state for lighting here

    // draw runway lighting
    glFogf (GL_FOG_DENSITY, rwy_exp2_punch_through);

    // CLO - 02/25/2005 - DO WE NEED THIS extra fgSetNearFar()?
    // fgSetNearFar( scene_nearplane, scene_farplane );

    if ( enhanced_lighting ) {

        // Enable states for drawing points with GL_extension
        glEnable(GL_POINT_SMOOTH);

        if ( distance_attenuation && glPointParameterIsSupported )
        {
            // Enable states for drawing points with GL_extension
            glEnable(GL_POINT_SMOOTH);

            float quadratic[3] = {1.0, 0.001, 0.0000001};
            // makes the points fade as they move away
            glPointParameterfvPtr(GL_DISTANCE_ATTENUATION_EXT, quadratic);
            glPointParameterfPtr(GL_POINT_SIZE_MIN_EXT, 1.0); 
        }

        glPointSize(4.0);

        // blending function for runway lights
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) ;
    }

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glPolygonMode(GL_FRONT, GL_POINT);

    // draw runway lighting
    if ( draw_otw ) {
        ssgCullAndDraw( globals->get_scenery()->get_vasi_lights_root() );
        ssgCullAndDraw( globals->get_scenery()->get_rwy_lights_root() );
    }

    // change punch through and then draw taxi lighting
    glFogf ( GL_FOG_DENSITY, fog_exp2_density );
    // sgVec3 taxi_fog;
    // sgSetVec3( taxi_fog, 0.0, 0.0, 0.0 );
    // glFogfv ( GL_FOG_COLOR, taxi_fog );
    if ( draw_otw ) {
        ssgCullAndDraw( globals->get_scenery()->get_taxi_lights_root() );
    }

    // clean up lighting
    glPolygonMode(GL_FRONT, GL_FILL);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    //static int _frame_count = 0;
    //if (_frame_count % 30 == 0) {
    //  printf("SSG: %s\n", ssgShowStats());
    //}
    //else {
    //  ssgShowStats();
    //}
    //_frame_count++;


    if ( enhanced_lighting ) {
        if ( distance_attenuation && glPointParameterIsSupported ) {
            glPointParameterfvPtr(GL_DISTANCE_ATTENUATION_EXT,
                                  default_attenuation);
        }

        glPointSize(1.0);
        glDisable(GL_POINT_SMOOTH);
    }

    // draw ground lighting
    glFogf (GL_FOG_DENSITY, ground_exp2_punch_through);
    if ( draw_otw ) {
        ssgCullAndDraw( globals->get_scenery()->get_gnd_lights_root() );
    }

    sgEnviro.drawLightning();

    if ( draw_otw && draw_clouds ) {
        if ( multi_pass_clouds ) {
            // Disable depth buffer update, draw the clouds where the
            //  objects overwrite the already drawn clouds, by testing
            //  the stencil buffer against 1
            glDepthMask( GL_FALSE );
            glStencilFunc( GL_EQUAL, 1, 1 );
            glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
            thesky->drawUpperClouds();
            thesky->drawLowerClouds();
            glDepthMask( GL_TRUE );
            glDisable( GL_STENCIL_TEST );
        } else {
            glDepthMask( GL_FALSE );
            if( volumetric_clouds )
                thesky->drawUpperClouds();
            thesky->drawLowerClouds();
            glDepthMask( GL_TRUE );
        }
    }
       double current_view_origin_airspeed_horiz_kt =
        fgGetDouble("/velocities/airspeed-kt", 0.0)
                       * cos( fgGetDouble("/orientation/pitch-deg", 0.0)
                               * SGD_DEGREES_TO_RADIANS);
       // TODO:find the real view speed, not the AC one
    sgEnviro.drawPrecipitation(
        fgGetDouble("/environment/metar/rain-norm", 0.0),
        fgGetDouble("/environment/metar/snow-norm", 0.0),
        fgGetDouble("/environment/metar/hail-norm", 0.0),
        current__view->getPitch_deg() + current__view->getPitchOffset_deg(),
        current__view->getRoll_deg() + current__view->getRollOffset_deg(),
        - current__view->getHeadingOffset_deg(),
               current_view_origin_airspeed_horiz_kt
               );

    // compute shadows and project them on screen
    bool is_internal = globals->get_current_view()->getInternal();
    // draw before ac because ac internal rendering clear the depth buffer

	globals->get_aircraft_model()->select( true );
    if( is_internal )
        shadows->endOfFrame();

    if ( draw_otw ) {
        FGTileMgr::set_tile_filter( false );
        sgSetModelFilter( false );
        globals->get_aircraft_model()->select( true );
        globals->get_model_mgr()->draw();
        globals->get_aircraft_model()->draw();

        FGTileMgr::set_tile_filter( true );
        sgSetModelFilter( true );
        globals->get_aircraft_model()->select( true );
    }
	// in 'external' view the ac can be culled, so shadows have not been draw in the
	// posttrav callback, this would be a rare case if the getInternal was acting
	// as expected (ie in internal view, getExternal returns false)
	if( !is_internal )
		shadows->endOfFrame();

    // display HUD && Panel
    glDisable( GL_FOG );
    glDisable( GL_DEPTH_TEST );
    // glDisable( GL_CULL_FACE );
    // glDisable( GL_TEXTURE_2D );

    // update the controls subsystem
    globals->get_controls()->update(delta_time_sec);

    hud_and_panel->apply();
    fgCockpitUpdate();

    // Use the hud_and_panel ssgSimpleState for rendering the ATC output
    // This only works properly if called before the panel call
    if((fgGetBool("/sim/atc/enabled")) || (fgGetBool("/sim/ai-traffic/enabled")))
        globals->get_ATC_display()->update(delta_time_sec);

    // Update any messages from the Nasal System
    globals->get_Nasal_display()->update(delta_time_sec);

    // update the panel subsystem
    if ( globals->get_current_panel() != NULL ) {
        globals->get_current_panel()->update(delta_time_sec);
    }
    fgUpdate3DPanels();

    // We can do translucent menus, so why not. :-)
    menus->apply();
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
    puDisplay();
    // glDisable ( GL_BLEND ) ;

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_FOG );

    // Fade out the splash screen over the first three seconds.
    double t = globals->get_sim_time_sec();
    if (t <= 2.5)
        fgSplashUpdate((2.5 - t) / 2.5);
}



// options.cxx needs to see this for toggle_panel()
// Handle new window size or exposure
void
FGRenderer::resize( int width, int height ) {
    int view_h;

    if ( (!fgGetBool("/sim/virtual-cockpit"))
         && fgPanelVisible() && idle_state == 1000 ) {
        view_h = (int)(height * (globals->get_current_panel()->getViewHeight() -
                             globals->get_current_panel()->getYOffset()) / 768.0);
    } else {
        view_h = height;
    }

    glViewport( 0, (GLint)(height - view_h), (GLint)(width), (GLint)(view_h) );

    static int lastwidth = width;
    static int lastheight = height;
    if (width != lastwidth)
        fgSetInt("/sim/startup/xsize", lastwidth = width);
    if (height != lastheight)
        fgSetInt("/sim/startup/ysize", lastheight = height);

    guiInitMouse(width, height);

    // for all views
    FGViewMgr *viewmgr = globals->get_viewmgr();
    if (viewmgr) {
      for ( int i = 0; i < viewmgr->size(); ++i ) {
        viewmgr->get_view(i)->
          set_aspect_ratio((float)view_h / (float)width);
      }

      setFOV( viewmgr->get_current_view()->get_h_fov(),
              viewmgr->get_current_view()->get_v_fov() );
      // cout << "setFOV(" << viewmgr->get_current_view()->get_h_fov()
      //      << ", " << viewmgr->get_current_view()->get_v_fov() << ")"
      //      << endl;

    }

    fgHUDReshape();

}


// These are wrapper functions around ssgSetNearFar() and ssgSetFOV()
// which will post process and rewrite the resulting frustum if we
// want to do asymmetric view frustums.

static void fgHackFrustum() {

    // specify a percent of the configured view frustum to actually
    // display.  This is a bit of a hack to achieve asymmetric view
    // frustums.  For instance, if you want to display two monitors
    // side by side, you could specify each with a double fov, a 0.5
    // aspect ratio multiplier, and then the left side monitor would
    // have a left_pct = 0.0, a right_pct = 0.5, a bottom_pct = 0.0,
    // and a top_pct = 1.0.  The right side monitor would have a
    // left_pct = 0.5 and a right_pct = 1.0.

    static SGPropertyNode *left_pct
        = fgGetNode("/sim/current-view/frustum-left-pct");
    static SGPropertyNode *right_pct
        = fgGetNode("/sim/current-view/frustum-right-pct");
    static SGPropertyNode *bottom_pct
        = fgGetNode("/sim/current-view/frustum-bottom-pct");
    static SGPropertyNode *top_pct
        = fgGetNode("/sim/current-view/frustum-top-pct");

    sgFrustum *f = ssgGetFrustum();

    // cout << " l = " << f->getLeft()
    //      << " r = " << f->getRight()
    //      << " b = " << f->getBot()
    //      << " t = " << f->getTop()
    //      << " n = " << f->getNear()
    //      << " f = " << f->getFar()
    //      << endl;

    double width = f->getRight() - f->getLeft();
    double height = f->getTop() - f->getBot();

    double l, r, t, b;

    if ( left_pct != NULL ) {
        l = f->getLeft() + width * left_pct->getDoubleValue();
    } else {
        l = f->getLeft();
    }

    if ( right_pct != NULL ) {
        r = f->getLeft() + width * right_pct->getDoubleValue();
    } else {
        r = f->getRight();
    }

    if ( bottom_pct != NULL ) {
        b = f->getBot() + height * bottom_pct->getDoubleValue();
    } else {
        b = f->getBot();
    }

    if ( top_pct != NULL ) {
        t = f->getBot() + height * top_pct->getDoubleValue();
    } else {
        t = f->getTop();
    }

    ssgSetFrustum(l, r, b, t, f->getNear(), f->getFar());
}


// we need some static storage space for these values.  However, we
// can't store it in a renderer class object because the functions
// that manipulate these are static.  They are static so they can
// interface to the display callback system.  There's probably a
// better way, there has to be a better way, but I'm not seeing it
// right now.
static float fov_width = 55.0;
static float fov_height = 42.0;
static float fov_near = 1.0;
static float fov_far = 1000.0;


/** FlightGear code should use this routine to set the FOV rather than
 *  calling the ssg routine directly
 */
void FGRenderer::setFOV( float w, float h ) {
    fov_width = w;
    fov_height = h;

    // fully specify the view frustum before hacking it (so we don't
    // accumulate hacked effects
    ssgSetFOV( w, h );
    ssgSetNearFar( fov_near, fov_far );
    fgHackFrustum();
    sgEnviro.setFOV( w, h );
}


/** FlightGear code should use this routine to set the Near/Far clip
 *  planes rather than calling the ssg routine directly
 */
void FGRenderer::setNearFar( float n, float f ) {
    fov_near = n;
    fov_far = f;

    // fully specify the view frustum before hacking it (so we don't
    // accumulate hacked effects
    ssgSetNearFar( n, f );
    ssgSetFOV( fov_width, fov_height );

    fgHackFrustum();
}

bool FGRenderer::getPickInfo( sgdVec3 pt, sgdVec3 dir, unsigned x, unsigned y )
{
  // Get the matrices involved in the transform from global to screen
  // coordinates.
  sgMat4 pm;
  ssgGetProjectionMatrix(pm);
  sgMat4 mv;
  ssgGetModelviewMatrix(mv);
  
  // Compose ...
  sgMat4 m;
  sgMultMat4(m, pm, mv);
  // ... and invert
  sgInvertMat4(m);
  
  // Get the width and height of the display to be able to normalize the
  // mouse coordinate
  float width = fgGetInt("/sim/startup/xsize");
  float height = fgGetInt("/sim/startup/ysize");
  
  // Compute some coordinates of in the line from the eyepoint to the
  // mouse click coodinates.
  // First build the normalized projection coordinates
  sgVec4 normPt;
  sgSetVec4(normPt, (2*x - width)/width, -(2*y - height)/height, 1, 1);
  // Transform them into the real world
  sgVec4 worldPt;
  sgXformPnt4(worldPt, normPt, m);
  if (worldPt[3] == 0)
    return false;
  sgScaleVec3(worldPt, 1/worldPt[3]);

  // Now build a direction from the point
  FGViewer* view = globals->get_current_view();
  sgVec4 fDir;
  sgSubVec3(fDir, worldPt, view->get_view_pos());
  sgdSetVec3(dir, fDir);
  sgdNormalizeVec3(dir);

  // Copy the start point
  sgdCopyVec3(pt, view->get_absolute_view_pos());

  return true;
}

// end of renderer.cxx
    
