// logger.cxx - log properties.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "logger.hxx"

#include <ios>
#include <string>
#include <cstdlib>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include "fg_props.hxx"
#include "globals.hxx"
#include "util.hxx"

using std::string;
using std::endl;

////////////////////////////////////////////////////////////////////////
// Implementation of FGLogger
////////////////////////////////////////////////////////////////////////

void
FGLogger::init ()
{
  SGPropertyNode * logging = fgGetNode("/logging");
  if (logging == 0)
    return;

  std::vector<SGPropertyNode_ptr> children = logging->getChildren("log");
  _logs.reserve(children.size());

  for (const auto& child: children) {
    if (!child->getBoolValue("enabled", false))
        continue;

    _logs.emplace_back(new Log());
    Log &log = *_logs.back();

    string filename = child->getStringValue("filename");
    if (filename.empty()) {
        filename = "fg_log.csv";
        child->setStringValue("filename", filename.c_str());
    }

    // Security: the path comes from the global Property Tree; it *must* be
    //           validated before we overwrite the file.
    const SGPath authorizedPath = fgValidatePath(SGPath::fromUtf8(filename),
                                                 /* write */ true);

    if (authorizedPath.isNull()) {
      const string propertyPath = child->getChild("filename")
                                       ->getPath(/* simplify */ true);
      const string msg =
        "The FGLogger logging system, via the '" + propertyPath + "' property, "
        "was asked to write to '" + filename + "', however this path is not "
        "authorized for writing anymore for security reasons. " +
        "Please choose another location, for instance in the $FG_HOME/Export "
        "folder (" + (globals->get_fg_home() / "Export").utf8Str() + ").";

      SG_LOG(SG_GENERAL, SG_ALERT, msg);
      exit(EXIT_FAILURE);
    }

    string delimiter = child->getStringValue("delimiter");
    if (delimiter.empty()) {
        delimiter = ",";
        child->setStringValue("delimiter", delimiter.c_str());
    }
        
    log.interval_ms = child->getLongValue("interval-ms");
    log.last_time_ms = globals->get_sim_time_sec() * 1000;
    log.delimiter = delimiter.c_str()[0];
    // Security: use the return value of fgValidatePath()
    log.output.reset(new sg_ofstream(authorizedPath, std::ios_base::out));
    if ( !(*log.output) ) {
      SG_LOG(SG_GENERAL, SG_ALERT, "Cannot write log to " << filename);
      _logs.pop_back();
      continue;
    }

    //
    // Process the individual entries (Time is automatic).
    //
    std::vector<SGPropertyNode_ptr> entries = child->getChildren("entry");
    (*log.output) << "Time";
    for (unsigned int j = 0; j < entries.size(); j++) {
      SGPropertyNode * entry = entries[j];

      //
      // Set up defaults.
      //
      if (!entry->hasValue("property")) {
          entry->setBoolValue("enabled", false);
          continue;
      }

      if (!entry->getBoolValue("enabled"))
          continue;

      SGPropertyNode * node =
	fgGetNode(entry->getStringValue("property"), true);
      log.nodes.push_back(node);
      (*log.output) << log.delimiter
		    << entry->getStringValue("title", node->getPath().c_str());
    }
    (*log.output) << endl;
  }
}

void
FGLogger::reinit ()
{
    _logs.clear();
    init();
}

void
FGLogger::bind ()
{
}

void
FGLogger::unbind ()
{
}

void
FGLogger::update (double dt)
{
    double sim_time_sec = globals->get_sim_time_sec();
    double sim_time_ms = sim_time_sec * 1000;
    for (unsigned int i = 0; i < _logs.size(); i++) {
        while ((sim_time_ms - _logs[i]->last_time_ms) >= _logs[i]->interval_ms) {
            _logs[i]->last_time_ms += _logs[i]->interval_ms;
            (*_logs[i]->output) << sim_time_sec;
            for (unsigned int j = 0; j < _logs[i]->nodes.size(); j++) {
                (*_logs[i]->output) << _logs[i]->delimiter
                                    << _logs[i]->nodes[j]->getStringValue();
            }
            (*_logs[i]->output) << endl;
        }
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGLogger::Log
////////////////////////////////////////////////////////////////////////

FGLogger::Log::Log ()
  : interval_ms(0),
    last_time_ms(-999999.0),
    delimiter(',')
{
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGLogger> registrantFGLogger;

// end of logger.cxx
