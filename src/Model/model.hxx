// model.hxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __MODEL_HXX
#define __MODEL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <vector>

SG_USING_STD(vector);

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/math/point3d.hxx>
#include <simgear/props/props.hxx>


// Don't pull in the headers, since we don't need them here.
class ssgBranch;
class ssgCutout;
class ssgEntity;
class ssgRangeSelector;
class ssgSelector;
class ssgTransform;

class SGInterpTable;
class FGCondition;
class FGLocation;


// Has anyone done anything *really* stupid, like making min and max macros?
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


/**
 * Load a 3D model with or without XML wrapper.
 *
 * If the path ends in ".xml", then it will be used as a property-
 * list wrapper to add animations to the model.
 *
 * Subsystems should not normally invoke this function directly;
 * instead, they should use the FGModelLoader declared in loader.hxx.
 */
ssgBranch * fgLoad3DModel( const string& fg_root, const string &path,
                           SGPropertyNode *prop_root, double sim_time_sec );


////////////////////////////////////////////////////////////////////////
// Model placement.
////////////////////////////////////////////////////////////////////////

/**
 * A wrapper for a model with a definite placement.
 */
class FGModelPlacement
{
public:

  FGModelPlacement ();
  virtual ~FGModelPlacement ();

  virtual void init( const string &fg_root,
                     const string &path,
                     SGPropertyNode *prop_root,
                     double sim_time_sec );
  virtual void update( const Point3D scenery_center );

  virtual ssgEntity * getSceneGraph () { return (ssgEntity *)_selector; }

  virtual FGLocation * getFGLocation () { return _location; }

  virtual bool getVisible () const;
  virtual void setVisible (bool visible);

  virtual double getLongitudeDeg () const { return _lon_deg; }
  virtual double getLatitudeDeg () const { return _lat_deg; }
  virtual double getElevationFt () const { return _elev_ft; }

  virtual void setLongitudeDeg (double lon_deg);
  virtual void setLatitudeDeg (double lat_deg);
  virtual void setElevationFt (double elev_ft);
  virtual void setPosition (double lon_deg, double lat_deg, double elev_ft);

  virtual double getRollDeg () const { return _roll_deg; }
  virtual double getPitchDeg () const { return _pitch_deg; }
  virtual double getHeadingDeg () const { return _heading_deg; }

  virtual void setRollDeg (double roll_deg);
  virtual void setPitchDeg (double pitch_deg);
  virtual void setHeadingDeg (double heading_deg);
  virtual void setOrientation (double roll_deg, double pitch_deg,
                               double heading_deg);
  
  // Addition by Diarmuid Tyson for Multiplayer Support
  // Allows multiplayer to get players position transform
  virtual const sgVec4 *get_POS() { return POS; }

private:

                                // Geodetic position
  double _lon_deg;
  double _lat_deg;
  double _elev_ft;

                                // Orientation
  double _roll_deg;
  double _pitch_deg;
  double _heading_deg;

  ssgSelector * _selector;
  ssgTransform * _position;

                                // Location
  FGLocation * _location;


  // Addition by Diarmuid Tyson for Multiplayer Support
  // Moved from update method
  // POS for transformation Matrix
  sgMat4 POS;

};

#endif // __MODEL_HXX
