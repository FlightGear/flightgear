// logger.cxx - log properties.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#include "logger.hxx"

#include STL_FSTREAM
#ifndef SG_HAVE_NATIVE_SGI_COMPILERS
SG_USING_STD(ofstream);
SG_USING_STD(endl);
#endif

#include <string>
SG_USING_STD(string);

#include <simgear/debug/logstream.hxx>

#include "fg_props.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of FGLogger
////////////////////////////////////////////////////////////////////////

FGLogger::FGLogger ()
{
}

FGLogger::~FGLogger ()
{
}

void
FGLogger::init ()
{
  SGPropertyNode * logging = fgGetNode("/logging");
  if (logging == 0)
    return;

  vector<SGPropertyNode *> children = logging->getChildren("log");
  for (unsigned int i = 0; i < children.size(); i++) {
    _logs.push_back(Log());
    Log &log = _logs[_logs.size()-1];
    SGPropertyNode * child = children[i];
    string filename = child->getStringValue("filename", "fg_log.csv");
    log.interval_ms = child->getLongValue("interval-ms", 0);
    log.delimiter = child->getStringValue("delimiter", ",")[0];
    log.output = new ofstream(filename.c_str());
    if (!log.output) {
      SG_LOG(SG_INPUT, SG_ALERT, "Cannot write log to " << filename);
      continue;
    }
    vector<SGPropertyNode *> entries = child->getChildren("entry");
    (*log.output) << "Time";
    for (unsigned int j = 0; j < entries.size(); j++) {
      SGPropertyNode * entry = entries[j];
      SGPropertyNode * node =
	fgGetNode(entry->getStringValue("property"), true);
      log.nodes.push_back(node);
      (*log.output) << log.delimiter
		    << entry->getStringValue("title", node->getPath());
    }
    (*log.output) << endl;
  }
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
FGLogger::update (int dt)
{
  long elapsed_ms = globals->get_elapsed_time_ms();
  for (unsigned int i = 0; i < _logs.size(); i++) {
    if ((elapsed_ms - _logs[i].last_time_ms) >= _logs[i].interval_ms) {
      _logs[i].last_time_ms = elapsed_ms;
      (*_logs[i].output) << globals->get_elapsed_time_ms();
      for (unsigned int j = 0; j < _logs[i].nodes.size(); j++) {
	(*_logs[i].output) << _logs[i].delimiter
			   << _logs[i].nodes[j]->getStringValue();
      }
      (*_logs[i].output) << endl;
    }
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGLogger::Log
////////////////////////////////////////////////////////////////////////

FGLogger::Log::Log ()
  : output(0),
    interval_ms(0),
    last_time_ms(-999999L),
    delimiter(',')
{
}

FGLogger::Log::~Log ()
{
  delete output;
}

// end of logger.cxx
