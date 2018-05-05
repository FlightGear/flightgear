// logger.hxx - log properties.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __LOGGER_HXX
#define __LOGGER_HXX 1

#include <memory>
#include <vector>

#include <simgear/compiler.h>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>

/**
 * Log any property values to any number of CSV files.
 */
class FGLogger : public SGSubsystem
{
public:
    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "logger"; }

private:
    /**
     * A single instance of a log file (the logger can contain many).
     */
    struct Log {
      Log ();

      std::vector<SGPropertyNode_ptr> nodes;
      std::unique_ptr<sg_ofstream> output;
      long interval_ms;
      double last_time_ms;
      char delimiter;
    };

    std::vector< std::unique_ptr<Log> > _logs;
};

#endif // __LOGGER_HXX
