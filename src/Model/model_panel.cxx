// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <string.h>             // for strcmp()

#include <vector>

#include <plib/sg.h>
#include <plib/ssg.h>
#include <plib/ul.h>

#include <simgear/misc/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/model/animation.hxx>
#include <simgear/scene/model/model.hxx>

#include "panelnode.hxx"

#include "model_panel.hxx"

SG_USING_STD(vector);



////////////////////////////////////////////////////////////////////////
// Global functions.
////////////////////////////////////////////////////////////////////////

// FIXME: should attempt to share more of the code with
// fgLoad3DModel() since these routines are almost identical with the
// exception of the panel node section.

ssgBranch *
fgLoad3DModelPanel( const string &fg_root, const string &path,
                    SGPropertyNode *prop_root,
                    double sim_time_sec )
{
  ssgBranch * model = 0;
  SGPropertyNode props;

                                // Load the 3D aircraft object itself
  SGPath xmlpath;
  SGPath modelpath = path;
  if ( ulIsAbsolutePathName( path.c_str() ) ) {
    xmlpath = modelpath;
  }
  else {
    xmlpath = fg_root;
    xmlpath.append(modelpath.str());
  }

                                // Check for an XML wrapper
  if (xmlpath.str().substr(xmlpath.str().size() - 4, 4) == ".xml") {
    readProperties(xmlpath.str(), &props);
    if (props.hasValue("/path")) {
      modelpath = modelpath.dir();
      modelpath.append(props.getStringValue("/path"));
    } else {
      if (model == 0)
        model = new ssgBranch;
    }
  }

                                // Assume that textures are in
                                // the same location as the XML file.
  if (model == 0) {
    ssgTexturePath((char *)xmlpath.dir().c_str());
    model = (ssgBranch *)ssgLoad((char *)modelpath.c_str());
    if (model == 0)
      throw sg_exception("Failed to load 3D model");
  }

                                // Set up the alignment node
  ssgTransform * alignmainmodel = new ssgTransform;
  alignmainmodel->addKid(model);
  sgMat4 res_matrix;
  sgMakeOffsetsMatrix(&res_matrix,
                      props.getFloatValue("/offsets/heading-deg", 0.0),
                      props.getFloatValue("/offsets/roll-deg", 0.0),
                      props.getFloatValue("/offsets/pitch-deg", 0.0),
                      props.getFloatValue("/offsets/x-m", 0.0),
                      props.getFloatValue("/offsets/y-m", 0.0),
                      props.getFloatValue("/offsets/z-m", 0.0));
  alignmainmodel->setTransform(res_matrix);

  unsigned int i;

                                // Load panels
  vector<SGPropertyNode_ptr> panel_nodes = props.getChildren("panel");
  for (i = 0; i < panel_nodes.size(); i++) {
    SG_LOG(SG_INPUT, SG_DEBUG, "Loading a panel");
    FGPanelNode * panel = new FGPanelNode(panel_nodes[i]);
    if (panel_nodes[i]->hasValue("name"))
        panel->setName((char *)panel_nodes[i]->getStringValue("name"));
    model->addKid(panel);
  }

                                // Load animations
  vector<SGPropertyNode_ptr> animation_nodes = props.getChildren("animation");
  for (i = 0; i < animation_nodes.size(); i++) {
    const char * name = animation_nodes[i]->getStringValue("name", 0);
    vector<SGPropertyNode_ptr> name_nodes =
      animation_nodes[i]->getChildren("object-name");
    sgMakeAnimation( model, name, name_nodes, prop_root, animation_nodes[i],
                     sim_time_sec);
  }

                                // Load sub-models
  vector<SGPropertyNode_ptr> model_nodes = props.getChildren("model");
  for (i = 0; i < model_nodes.size(); i++) {
    SGPropertyNode_ptr node = model_nodes[i];
    ssgTransform * align = new ssgTransform;
    sgMat4 res_matrix;
    sgMakeOffsetsMatrix(&res_matrix,
                        node->getFloatValue("offsets/heading-deg", 0.0),
                        node->getFloatValue("offsets/roll-deg", 0.0),
                        node->getFloatValue("offsets/pitch-deg", 0.0),
                        node->getFloatValue("offsets/x-m", 0.0),
                        node->getFloatValue("offsets/y-m", 0.0),
                        node->getFloatValue("offsets/z-m", 0.0));
    align->setTransform(res_matrix);

    ssgBranch * kid = sgLoad3DModel( fg_root, node->getStringValue("path"),
                                     prop_root, sim_time_sec );
    align->addKid(kid);
    model->addKid(align);
  }

  return alignmainmodel;
}


// end of model_panel.cxx
