// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>		// for strcmp()

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/model/shadowvolume.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/renderer.hxx>
#include <Main/viewmgr.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>

#include "model_panel.hxx"

#include "acmodel.hxx"


class fgLoaderOptions : ssgLoaderOptions {

public:
    virtual void makeTexturePath ( char* path, const char *fname ) const ;
    string livery_path;

};

void fgLoaderOptions::makeTexturePath ( char *path, const char *fname ) const
{
  /* Remove all leading path information. */
  const char* seps = "\\/" ;
  const char* fn = & fname [ strlen ( fname ) - 1 ] ;
  for ( ; fn != fname && strchr(seps,*fn) == NULL ; fn-- )
    /* Search back for a seperator */ ;
  if ( strchr(seps,*fn) != NULL )
    fn++ ;
  fname = fn ;
  // if we have a livery path and the texture is found there then we use that
  // path in priority, if the texture was not found or we add no additional 
  // livery path then we use the current model path or model/texture-path
  if( livery_path.size() ) {
      make_path( path, livery_path.c_str(), fname );
      if( ulFileExists( path ) )
          return;
  }
  make_path ( path, texture_dir, fname ) ;
}

static fgLoaderOptions _fgLoaderOptions;


////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel
////////////////////////////////////////////////////////////////////////

FGAircraftModel::FGAircraftModel ()
  : _aircraft(0),
    _selector(new ssgSelector),
    _scene(new ssgRoot),
    _nearplane(0.10f),
    _farplane(1000.0f)
{
}

FGAircraftModel::~FGAircraftModel ()
{
  // Unregister that one at the scenery manager
  if (_aircraft)
    globals->get_scenery()->unregister_placement_transform(_aircraft->getTransform());

  delete _aircraft;
				// SSG will delete it
  globals->get_scenery()->get_aircraft_branch()->removeKid(_selector);
}

void 
FGAircraftModel::init ()
{
  ssgLoaderOptions *currLoaderOptions = ssgGetCurrentOptions();
  ssgSetCurrentOptions( (ssgLoaderOptions*)&_fgLoaderOptions );
  _aircraft = new SGModelPlacement;
  string path = fgGetString("/sim/model/path", "Models/Geometry/glider.ac");
  string texture_path = fgGetString("/sim/model/texture-path");
  if( texture_path.size() ) {
      SGPath temp_path;
      if ( !ulIsAbsolutePathName( texture_path.c_str() ) ) {
          temp_path = globals->get_fg_root();
          temp_path.append( SGPath( path ).dir() );
          temp_path.append( texture_path );
          _fgLoaderOptions.livery_path = temp_path.str();
      } else
          _fgLoaderOptions.livery_path = texture_path;
  }
  try {
    ssgBranch *model = fgLoad3DModelPanel( globals->get_fg_root(),
                                           path,
                                           globals->get_props(),
                                           globals->get_sim_time_sec() );
    _aircraft->init( model );
  } catch (const sg_exception &ex) {
    SG_LOG(SG_GENERAL, SG_ALERT, "Failed to load aircraft from " << path);
    SG_LOG(SG_GENERAL, SG_ALERT, "(Falling back to glider.ac.)");
    ssgBranch *model = fgLoad3DModelPanel( globals->get_fg_root(),
                                           "Models/Geometry/glider.ac",
                                           globals->get_props(),
                                           globals->get_sim_time_sec() );
    _aircraft->init( model );
  }
  _scene->addKid(_aircraft->getSceneGraph());
  _selector->addKid(_aircraft->getSceneGraph());
  globals->get_scenery()->get_aircraft_branch()->addKid(_selector);

  // Register that one at the scenery manager
  globals->get_scenery()->register_placement_transform(_aircraft->getTransform());
  ssgSetCurrentOptions( currLoaderOptions );
}

void 
FGAircraftModel::bind ()
{
  // No-op
}

void 
FGAircraftModel::unbind ()
{
  // No-op
}

void
FGAircraftModel::update (double dt)
{
  int view_number = globals->get_viewmgr()->get_current();
  int is_internal = fgGetBool("/sim/current-view/internal");

  if (view_number == 0 && !is_internal) {
    _aircraft->setVisible(false);
  } else {
    _aircraft->setVisible(true);
  }

  _aircraft->setPosition(fgGetDouble("/position/longitude-deg"),
			 fgGetDouble("/position/latitude-deg"),
			 fgGetDouble("/position/altitude-ft"));
  _aircraft->setOrientation(fgGetDouble("/orientation/roll-deg"),
			    fgGetDouble("/orientation/pitch-deg"),
			    fgGetDouble("/orientation/heading-deg"));
  _aircraft->update();
}

void
FGAircraftModel::draw ()
{
				// OK, now adjust the clip planes and draw
				// FIXME: view number shouldn't be 
				// hard-coded.
  bool is_internal = globals->get_current_view()->getInternal();
  if (_aircraft->getVisible() && is_internal) {
    glClearDepth(1);
    glClear(GL_DEPTH_BUFFER_BIT);
    FGRenderer::setNearFar(_nearplane, _farplane);
    ssgCullAndDraw(_scene);
    _selector->select(0);
  } else {
    _selector->select(1);
    // in external view the shadows are drawn before the transparent parts of the ac
    _scene->setTravCallback( SSG_CALLBACK_POSTTRAV, SGShadowVolume::ACpostTravCB);
    ssgCullAndDraw(_scene);
    _scene->setTravCallback( SSG_CALLBACK_POSTTRAV, 0);
  }

}

// end of model.cxx
