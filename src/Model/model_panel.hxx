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

using std::vector;

#include <simgear/props/props.hxx>


// Don't pull in the headers, since we don't need them here.

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
 * Load a 3D model with or without XML wrapper.  This version supports
 * also loading the instrument panel and is wired in with dependencies
 * on panelnode.hxx, and thus files in src/Cockpit and also GUI/mouse
 * input code to support the 3d clickable hotspots.
 *
 * If the path ends in ".xml", then it will be used as a property-
 * list wrapper to add animations to the model.
 *
 * Subsystems should not normally invoke this function directly;
 * instead, they should use the SGModelLoader declared in loader.hxx.
 */
osg::Node *fgLoad3DModelPanel( const std::string &path, SGPropertyNode *prop_root);


#endif // __MODEL_HXX
