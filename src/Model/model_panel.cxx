// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <plib/ssg.h>

#include <simgear/props/props.hxx>
#include <simgear/scene/model/model.hxx>

#include "panelnode.hxx"

#include "model_panel.hxx"

SG_USING_STD(vector);

static
ssgEntity *load_panel(SGPropertyNode *n)
{
  return new FGPanelNode(n);
}

////////////////////////////////////////////////////////////////////////
// Global functions.
////////////////////////////////////////////////////////////////////////

ssgBranch *
fgLoad3DModelPanel( const string &fg_root, const string &path,
                    SGPropertyNode *prop_root,
                    double sim_time_sec )
{
  return sgLoad3DModel( fg_root, path, prop_root, sim_time_sec, load_panel );
}


// end of model_panel.cxx
