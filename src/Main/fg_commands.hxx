// fg_commands.hxx - built-in commands for FlightGear.

#ifndef __FG_COMMANDS_HXX
#define __FG_COMMANDS_HXX

#ifndef __cplusplus
# error This library requires C++
#endif


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/structure/commands.hxx>
#include <simgear/props/props.hxx>

/**
 * Initialize the built-in commands.
 */
void fgInitCommands ();

#endif

// end of fg_commands.hxx
