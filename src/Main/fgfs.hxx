// fgfs.hxx -- top level include file for FlightGear.
//
// Written by David Megginson, started 2000-12
//
// Copyright (C) 2000  David Megginson, david@megginson.com
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


#ifndef __FGFS_HXX
#define __FGFS_HXX 1


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#  include <float.h>                    
#endif



/**
 * Basic interface for all FlightGear subsystems.
 *
 * <p>This is an abstract interface that all FlightGear subsystems
 * will eventually implement.  It defines the basic operations for
 * each subsystem: initialization, property binding and unbinding, and
 * updating.  Interfaces may define additional methods, but the
 * preferred way of exchanging information with other subsystems is
 * through the property tree.</p>
 *
 * <p>To publish information through a property, a subsystem should
 * bind it to a variable or (if necessary) a getter/setter pair in the
 * bind() method, and release the property in the unbind() method:</p>
 *
 * <pre>
 * void MySubsystem::bind ()
 * {
 *   fgTie("/controls/elevator", &_elevator);
 *   fgSetArchivable("/controls/elevator");
 * }
 *
 * void MySubsystem::unbind ()
 * {
 *   fgUntie("/controls/elevator");
 * }
 * </pre>
 *
 * <p>To reference a property (possibly) from another subsystem, there
 * are two alternatives.  If the property will be referenced only
 * infrequently (say, in the init() method), then the fgGet* methods
 * declared in fg_props.hxx are the simplest:</p>
 *
 * <pre>
 * void MySubsystem::init ()
 * {
 *   _errorMargin = fgGetFloat("/display/error-margin-pct");
 * }
 * </pre>
 *
 * <p>On the other hand, if the property will be referenced frequently
 * (say, in the update() method), then the hash-table lookup required
 * by the fgGet* methods might be too expensive; instead, the
 * subsystem should obtain a reference to the actual property node in
 * its init() function and use that reference in the main loop:</p>
 *
 * <pre>
 * void MySubsystem::init ()
 * {
 *   _errorNode = fgGetNode("/display/error-margin-pct", true);
 * }
 *
 * void MySubsystem::update ()
 * {
 *   do_something(_errorNode.getFloatValue());
 * }
 * </pre>
 *
 * <p>The node returned will always be a pointer to SGPropertyNode,
 * and the subsystem should <em>not</em> delete it in its destructor
 * (the pointer belongs to the property tree, not the subsystem).</p>
 */
class FGSubsystem
{
public:

  /**
   * Virtual destructor to ensure that subclass destructors are called.
   */
  virtual ~FGSubsystem ();


  /**
   * Initialize the subsystem.
   *
   * <p>This method should set up the state of the subsystem, but
   * should not bind any properties.  Note that any dependencies on
   * the state of other subsystems should be placed here rather than
   * in the constructor, so that FlightGear can control the
   * initialization order.</p>
   */
  virtual void init () = 0;


  /**
   * Acquire the subsystem's property bindings.
   *
   * <p>This method should bind all properties that the subsystem
   * publishes.  It will be invoked after init, but before any
   * invocations of update.</p>
   */
  virtual void bind () = 0;


  /**
   * Release the subsystem's property bindings.
   *
   * <p>This method should release all properties that the subsystem
   * publishes.  It will be invoked by FlightGear (not the destructor)
   * just before the subsystem is removed.</p>
   */
  virtual void unbind () = 0;


  /**
   * Update the subsystem.
   *
   * <p>FlightGear invokes this method every time the subsystem should
   * update its state.  If the subsystem requires delta time information,
   * it should track it itself.</p>
   */
  virtual void update (int dt) = 0;

};


#endif // __FGFS_HXX

// end of fgfs.hxx
