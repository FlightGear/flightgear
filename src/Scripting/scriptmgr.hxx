// scriptmgr.hxx - run user scripts
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __SCRIPTMGR_HXX
#define __SCRIPTMGR_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>	// for SG_USING_STD
#include <simgear/structure/subsystem_mgr.hxx>

class pslExtension;


/**
 * Manager for user scripts in PSL (PLIB's scripting language).
 *
 * The initial draft of this subsystem does nothing on update, and
 * simply executes scripts on demand from various input bindings, but
 * I plan to merge in code from Erik Hofman for events and scheduled
 * tasks.
 *
 * Right now, there are three extension commands for
 * FlightGear:
 *
 * print(...) - prints all of its arguments to standard output.
 * get_property(name) - get a property value
 * set_property(name, value) - set a property value
 */
class FGScriptMgr : public SGSubsystem
{
public:

    /**
     * Constructor.
     */
    FGScriptMgr ();


    /**
     * Destructor.
     */
    virtual ~FGScriptMgr ();


    /**
     * Initialize PSL.
     */
    virtual void init ();


    /**
     * Update (no-op for now).
     */
    virtual void update (double delta_time_sec);


    /**
     * Run an in-memory user script.
     *
     * Any included files are referenced relative to $FG_ROOT.  This
     * is not very efficient right now, since it recompiles the script
     * every time it runs.  The script must have a main() function.
     *
     * @param script A string containing the script.
     * @return true if the script compiled properly, false otherwise.
     * @see #run_inline
     */
    virtual bool run (const char * script) const;


    /**
     * Run an inline in-memory user script.
     *
     * The script is executed as if it were surrounded by a main()
     * function, so it cannot contain any top-level constructions like
     * #include statements or function declarations.  This is useful
     * for quick one-liners.
     *
     * @param script A string containing the inline script.
     * @return true if the script compiled properly, false otherwise.
     * @see #run
     */
    virtual bool run_inline (const char * script) const;
    
};


#endif // __SCRIPTMGR_HXX
