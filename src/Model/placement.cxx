// placement.cxx - manage the placment of a 3D model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#include <simgear/compiler.h>

#include <string.h>             // for strcmp()

#include <vector>

#include <plib/sg.h>
#include <plib/ssg.h>
#include <plib/ul.h>

#include <simgear/scene/model/location.hxx>

#include "model.hxx"

#include "placement.hxx"

SG_USING_STD(vector);



////////////////////////////////////////////////////////////////////////
// Implementation of FGModelPlacement.
////////////////////////////////////////////////////////////////////////

FGModelPlacement::FGModelPlacement ()
  : _lon_deg(0),
    _lat_deg(0),
    _elev_ft(0),
    _roll_deg(0),
    _pitch_deg(0),
    _heading_deg(0),
    _selector(new ssgSelector),
    _position(new ssgTransform),
    _location(new FGLocation)
{
}

FGModelPlacement::~FGModelPlacement ()
{
}

#if 0
void
FGModelPlacement::init( const string &fg_root,
                        const string &path,
                        SGPropertyNode *prop_root,
                        double sim_time_sec, int dummy )
{
  ssgBranch * model = fgLoad3DModel( fg_root, path, prop_root, sim_time_sec );
  if (model != 0)
      _position->addKid(model);
  _selector->addKid(_position);
  _selector->clrTraversalMaskBits(SSGTRAV_HOT);
}
#endif

void
FGModelPlacement::init( ssgBranch * model )
{
  if (model != 0) {
      _position->addKid(model);
  }
  _selector->addKid(_position);
  _selector->clrTraversalMaskBits(SSGTRAV_HOT);
}

void
FGModelPlacement::update( const Point3D scenery_center )
{
  _location->setPosition( _lon_deg, _lat_deg, _elev_ft );
  _location->setOrientation( _roll_deg, _pitch_deg, _heading_deg );

  sgCopyMat4( POS, _location->getTransformMatrix(scenery_center) );

  sgVec3 trans;
  sgCopyVec3(trans, _location->get_view_pos());

  for(int i = 0; i < 4; i++) {
    float tmp = POS[i][3];
    for( int j=0; j<3; j++ ) {
      POS[i][j] += (tmp * trans[j]);
    }
  }
  _position->setTransform(POS);
}

bool
FGModelPlacement::getVisible () const
{
  return (_selector->getSelect() != 0);
}

void
FGModelPlacement::setVisible (bool visible)
{
  _selector->select(visible);
}

void
FGModelPlacement::setLongitudeDeg (double lon_deg)
{
  _lon_deg = lon_deg;
}

void
FGModelPlacement::setLatitudeDeg (double lat_deg)
{
  _lat_deg = lat_deg;
}

void
FGModelPlacement::setElevationFt (double elev_ft)
{
  _elev_ft = elev_ft;
}

void
FGModelPlacement::setPosition (double lon_deg, double lat_deg, double elev_ft)
{
  _lon_deg = lon_deg;
  _lat_deg = lat_deg;
  _elev_ft = elev_ft;
}

void
FGModelPlacement::setRollDeg (double roll_deg)
{
  _roll_deg = roll_deg;
}

void
FGModelPlacement::setPitchDeg (double pitch_deg)
{
  _pitch_deg = pitch_deg;
}

void
FGModelPlacement::setHeadingDeg (double heading_deg)
{
  _heading_deg = heading_deg;
}

void
FGModelPlacement::setOrientation (double roll_deg, double pitch_deg,
                                  double heading_deg)
{
  _roll_deg = roll_deg;
  _pitch_deg = pitch_deg;
  _heading_deg = heading_deg;
}

// end of model.cxx
