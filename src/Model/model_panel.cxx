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
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include "panelnode.hxx"

#include "model_panel.hxx"

SG_USING_STD(vector);

using namespace simgear;

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
fgLoad3DModelPanel(const string &path, SGPropertyNode *prop_root)
{
    osg::Node* node = SGModelLib::loadModel(path, prop_root, load_panel);
    node->setNodeMask(~SG_NODEMASK_TERRAIN_BIT);
    return node;
}


// end of model_panel.cxx
