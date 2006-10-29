// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <osg/Geode>

#include <simgear/props/props.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include "panelnode.hxx"

#include "model_panel.hxx"

SG_USING_STD(vector);

static
osg::Node* load_panel(SGPropertyNode *n)
{
  osg::Geode* geode = new osg::Geode;
  geode->addDrawable(new FGPanelNode(n));
  return geode;
}


////////////////////////////////////////////////////////////////////////
// Global functions.
////////////////////////////////////////////////////////////////////////

osg::Node *
fgLoad3DModelPanel( const string &fg_root, const string &path,
                    SGPropertyNode *prop_root,
                    double sim_time_sec, const SGPath& livery )
{
  osg::Node* node = sgLoad3DModel( fg_root, path, prop_root, sim_time_sec,
                                   load_panel, 0, livery );
  node->setNodeMask(~SG_NODEMASK_TERRAIN_BIT);
  return node;
}


// end of model_panel.cxx
