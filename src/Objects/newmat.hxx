// newmat.hxx -- a material in the scene graph.
// TODO: this class needs to be renamed.
//
// Written by Curtis Olson, started May 1998.
// Overhauled by David Megginson, December 2001
//
// Copyright (C) 1998 - 2000  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _NEWMAT_HXX
#define _NEWMAT_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/compiler.h>
#include <simgear/misc/props.hxx>

#include <GL/glut.h>

#include STL_STRING      // Standard C++ string library

SG_USING_STD(string);


/**
 * A material in the scene graph.
 *
 * A material represents information about a single surface type
 * in the 3D scene graph, including texture, colour, lighting,
 * tiling, and so on; most of the materials in FlightGear are
 * defined in the $FG_ROOT/materials.xml file, and can be changed
 * at runtime.
 */
class FGNewMat {

public:


  ////////////////////////////////////////////////////////////////////
  // Public Constructors.
  ////////////////////////////////////////////////////////////////////

  /**
   * Construct a material from a set of properties.
   *
   * @param props A property node containing subnodes with the
   * state information for the material.  This node is usually
   * loaded from the $FG_ROOT/materials.xml file.
   */
  FGNewMat (const SGPropertyNode * props);


  /**
   * Construct a material from an absolute texture path.
   *
   * @param texture_path A string containing an absolute path
   * to a texture file (usually RGB).
   */
  FGNewMat (const string &texpath);


  /**
   * Construct a material around an existing SSG state.
   *
   * This constructor allows the application to create a custom,
   * low-level state for the scene graph and wrap a material around
   * it.  Note: the pointer ownership is transferred to the material.
   *
   * @param s The SSG state for this material.
   */
  FGNewMat (ssgSimpleState * s);

  /**
   * Destructor.
   */
  virtual ~FGNewMat ( void );



  ////////////////////////////////////////////////////////////////////
  // Public methods.
  ////////////////////////////////////////////////////////////////////

  /**
   * Force the texture to load if it hasn't already.
   *
   * @return true if the texture loaded, false if it was loaded
   * already.
   */
  virtual bool load_texture ();


  /**
   * Get the textured state.
   */
  virtual inline ssgSimpleState *get_textured () { return textured; }


  /**
   * Get the xsize of the texture, in meters.
   */
  virtual inline double get_xsize() const { return xsize; }


  /**
   * Get the ysize of the texture, in meters.
   */
  virtual inline double get_ysize() const { return ysize; }


  /**
   * Get the light coverage.
   *
   * A smaller number means more generated night lighting.
   *
   * @return The area (m^2?) covered by each light.
   */
  virtual inline double get_light_coverage () const { return light_coverage; }


  /**
   * Get the number of dynamic objects defined for this material.
   */
  virtual int get_object_count () const { return objects.size(); }


  /**
   * Get a dynamic object for this material.
   */
  virtual ssgEntity * get_object (int i) const { return objects[i].model; }


  /**
   * Get the average space for a dynamic object for this material.
   */
  virtual double get_object_coverage (int i) const {
    return objects[i].coverage;
  }


  /**
   * Get the target LOD range for a dynamic object for this material.
   */
  virtual double get_object_lod (int i) const {
    return objects[i].lod;
  }


  /**
   * Get the current state.
   */
  virtual inline ssgStateSelector *get_state () const { return state; }


  /**
   * Increment the reference count for this material.
   *
   * A material with 0 references may be deleted by the
   * material library.
   */
  virtual inline void ref () { refcount++; }


  /**
   * Decrement the reference count for this material.
   */
  virtual inline void deRef () { refcount--; }


  /**
   * Get the reference count for this material.
   *
   * @return The number of references (0 if none).
   */
  virtual inline int getRef () const { return refcount; }

protected:


  ////////////////////////////////////////////////////////////////////
  // Protected methods.
  ////////////////////////////////////////////////////////////////////

  /**
   * Initialization method, invoked by all public constructors.
   */
  virtual void init();


private:


  ////////////////////////////////////////////////////////////////////
  // Internal state.
  ////////////////////////////////////////////////////////////////////

  // names
  string texture_path;

  // pointers to ssg states
  ssgStateSelector *state;
  ssgSimpleState *textured;
  ssgSimpleState *nontextured;

  // texture size
  double xsize, ysize;

  // wrap texture?
  bool wrapu, wrapv;

  // use mipmapping?
  int mipmap;

  // coverage of night lighting.
  double light_coverage;

  // material properties
  sgVec4 ambient, diffuse, specular, emission;

  // true if texture loading deferred, and not yet loaded
  bool texture_loaded;

  struct Object
  {
    ssgEntity * model;
    double coverage;
    double lod;
  };

  vector<Object> objects;

  // ref count so we can properly delete if we have multiple
  // pointers to this record
  int refcount;



  ////////////////////////////////////////////////////////////////////
  // Internal constructors and methods.
  ////////////////////////////////////////////////////////////////////

  FGNewMat (const FGNewMat &mat); // unimplemented

  void read_properties (const SGPropertyNode * props);
  void build_ssg_state(bool defer_tex_load = false);
  void set_ssg_state( ssgSimpleState *s );

};

#endif // _NEWMAT_HXX 
