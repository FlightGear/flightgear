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

#include <simgear/compiler.h>

// #ifdef SG_MATH_EXCEPTION_CLASH
// #  include <math.h>
// #endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#  include <float.h>                    
#endif

#include STL_STRING
SG_USING_STD(string);

#include <vector>
SG_USING_STD(vector);

#include <map>
SG_USING_STD(map);

#include <simgear/misc/props.hxx>



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
 * void MySubsystem::update (double delta_time_sec)
 * {
 *   do_something(_errorNode.getFloatValue());
 * }
 * </pre>
 *
 * <p>The node returned will always be a pointer to SGPropertyNode,
 * and the subsystem should <em>not</em> delete it in its destructor
 * (the pointer belongs to the property tree, not the subsystem).</p>
 *
 * <p>The program may ask the subsystem to suspend or resume
 * sim-time-dependent operations; by default, the suspend() and
 * resume() methods set the protected variable <var>_suspended</var>,
 * which the subsystem can reference in its update() method, but
 * subsystems may also override the suspend() and resume() methods to
 * take different actions.</p>
 */
class FGSubsystem
{
public:

  /**
   * Default constructor.
   */
  FGSubsystem ();

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
  virtual void init ();


  /**
   * Reinitialize the subsystem.
   *
   * <p>This method should cause the subsystem to reinitialize itself,
   * and (normally) to reload any configuration files.</p>
   */
  virtual void reinit ();


  /**
   * Acquire the subsystem's property bindings.
   *
   * <p>This method should bind all properties that the subsystem
   * publishes.  It will be invoked after init, but before any
   * invocations of update.</p>
   */
  virtual void bind ();


  /**
   * Release the subsystem's property bindings.
   *
   * <p>This method should release all properties that the subsystem
   * publishes.  It will be invoked by FlightGear (not the destructor)
   * just before the subsystem is removed.</p>
   */
  virtual void unbind ();


  /**
   * Update the subsystem.
   *
   * <p>FlightGear invokes this method every time the subsystem should
   * update its state.</p>
   *
   * @param delta_time_sec The delta time, in seconds, since the last
   * update.  On first update, delta time will be 0.
   */
  virtual void update (double delta_time_sec) = 0;


  /**
   * Suspend operation of this subsystem.
   *
   * <p>This method instructs the subsystem to suspend
   * sim-time-dependent operations until asked to resume.  The update
   * method will still be invoked so that the subsystem can take any
   * non-time-dependent actions, such as updating the display.</p>
   *
   * <p>It is not an error for the suspend method to be invoked when
   * the subsystem is already suspended; the invocation should simply
   * be ignored.</p>
   */
  virtual void suspend ();


  /**
   * Suspend or resum operation of this subsystem.
   *
   * @param suspended true if the subsystem should be suspended, false
   * otherwise.
   */
  virtual void suspend (bool suspended);


  /**
   * Resume operation of this subsystem.
   *
   * <p>This method instructs the subsystem to resume
   * sim-time-depended operations.  It is not an error for the resume
   * method to be invoked when the subsystem is not suspended; the
   * invocation should simply be ignored.</p>
   */
  virtual void resume ();


  /**
   * Test whether this subsystem is suspended.
   *
   * @return true if the subsystem is suspended, false if it is not.
   */
  virtual bool is_suspended () const;


protected:

  mutable SGPropertyNode_ptr _freeze_master_node;
  bool _suspended;

};



/**
 * A group of FlightGear subsystems.
 */
class FGSubsystemGroup : public FGSubsystem
{
public:

    FGSubsystemGroup ();
    virtual ~FGSubsystemGroup ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double delta_time_sec);

    virtual void set_subsystem (const string &name,
                                FGSubsystem * subsystem,
                                double min_step_sec = 0);
    virtual FGSubsystem * get_subsystem (const string &name);
    virtual void remove_subsystem (const string &name);
    virtual bool has_subsystem (const string &name) const;

private:

    struct Member {

        Member ();
        Member (const Member &member);
        virtual ~Member ();

        virtual void update (double delta_time_sec);

        string name;
        FGSubsystem * subsystem;
        double min_step_sec;
        double elapsed_sec;
    };

    Member * get_member (const string &name, bool create = false);

    vector<Member *> _members;
};



/**
 * Manage subsystems for the application.
 */
class FGSubsystemMgr : public FGSubsystem
{
public:

    enum GroupType {
        INIT = 0,
        GENERAL,
        MAX_GROUPS
    };

    FGSubsystemMgr ();
    virtual ~FGSubsystemMgr ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double delta_time_sec);

    virtual void add (GroupType group, const string &name,
                      FGSubsystem * subsystem,
                      double min_time_sec = 0);

    virtual FGSubsystemGroup * get_group (GroupType group);

private:

    FGSubsystemGroup _groups[MAX_GROUPS];

};



#endif // __FGFS_HXX

// end of fgfs.hxx
