// logger.hxx - log properties.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __LOGGER_HXX
#define __LOGGER_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/exception.hxx>
#include <simgear/misc/props.hxx>

#include <iostream>
SG_USING_STD(ostream);

#include <vector>
SG_USING_STD(vector);

#include "fgfs.hxx"


/**
 * Log any property values to any number of CSV files.
 */
class FGLogger : public FGSubsystem
{
public:

  FGLogger ();
  virtual ~FGLogger ();

				// Implementation of FGSubsystem
  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (int dt);

private:

  /**
   * A single instance of a log file (the logger can contain many).
   */
  struct Log {
    Log ();
    virtual ~Log ();
    vector<SGPropertyNode *> nodes;
    ostream * output;
    long interval_ms;
    long last_time_ms;
  };

  vector<Log> _logs;

};

#endif // __LOGGER_HXX
