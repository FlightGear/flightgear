//  panel_io.cxx - I/O for 2D panel.
//
//  Written by David Megginson, started January 2000.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//  $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>		// for strcmp()

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>

#include <istream>
#include <fstream>
#include <string>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include <GUI/gui.h>

// #include "panel.hxx"
#include "panel_io.hxx"
#include <Instrumentation/KLN89/kln89.hxx>

//built-in layers
#include "built_in/FGMagRibbon.hxx"

using std::istream;
using std::ifstream;
using std::string;



////////////////////////////////////////////////////////////////////////
// Read and construct a panel.
//
// The panel is specified as a regular property list, and each of the
// instruments is its own, separate property list (and thus, a separate
// XML document).  The functions in this section read in the files
// as property lists, then extract properties to set up the panel
// itself.
//
// A panel contains zero or more instruments.
//
// An instrument contains one or more layers and zero or more actions.
//
// A layer contains zero or more transformations.
//
// Some special types of layers also contain other objects, such as 
// chunks of text or other layers.
//
// There are currently four types of layers:
//
// 1. Textured Layer (type="texture"), the default
// 2. Text Layer (type="text")
// 3. Switch Layer (type="switch")
// 4. Built-in Layer (type="built-in", must also specify class)
//
// The only built-in layer so far is the ribbon for the magnetic compass
// (class="compass-ribbon").
//
// There are three types of actions:
//
// 1. Adjust (type="adjust"), the default
// 2. Swap (type="swap")
// 3. Toggle (type="toggle")
//
// There are three types of transformations:
//
// 1. X shift (type="x-shift"), the default
// 2. Y shift (type="y-shift")
// 3. Rotation (type="rotation")
//
// Each of these may be associated with a property, so that a needle
// will rotate with the airspeed, for example, or may have a fixed
// floating-point value.
////////////////////////////////////////////////////////////////////////


/**
 * Read a cropped texture from the instrument's property list.
 *
 * The x1 and y1 properties give the starting position of the texture
 * (between 0.0 and 1.0), and the the x2 and y2 properties give the
 * ending position.  For example, to use the bottom-left quarter of a
 * texture, x1=0.0, y1=0.0, x2=0.5, y2=0.5.
 */
static FGCroppedTexture
readTexture (const SGPropertyNode * node)
{
    FGCroppedTexture texture(node->getStringValue("path"),
			     node->getFloatValue("x1"),
			     node->getFloatValue("y1"),
			     node->getFloatValue("x2", 1.0),
			     node->getFloatValue("y2", 1.0));
    SG_LOG(SG_COCKPIT, SG_DEBUG, "Read texture " << node->getName());
    return texture;
}


/**
 * Test for a condition in the current node.
 */

////////////////////////////////////////////////////////////////////////
// Read a condition and use it if necessary.
////////////////////////////////////////////////////////////////////////

static void
readConditions (SGConditional *component, const SGPropertyNode *node)
{
  const SGPropertyNode * conditionNode = node->getChild("condition");
  if (conditionNode != 0)
				// The top level is implicitly AND
    component->setCondition(sgReadCondition(globals->get_props(),
                                            conditionNode) );
}


/**
 * Read an action from the instrument's property list.
 *
 * The action will be performed when the user clicks a mouse button
 * within the specified region of the instrument.  Actions always work
 * by modifying the value of a property (see the SGPropertyNode
 * class).
 *
 * The following action types are defined:
 *
 * "adjust" - modify the value of a floating-point property by
 *    the increment specified.  This is the default.
 *
 * "swap" - swap the values of two-floating-point properties.
 *
 * "toggle" - toggle the value of a boolean property between true and
 *    false.
 *
 * For the adjust action, it is possible to specify an increment
 * (use a negative number for a decrement), a minimum allowed value,
 * a maximum allowed value, and a flag to indicate whether the value
 * should freeze or wrap-around when it reachs the minimum or maximum.
 *
 * The action will be scaled automatically if the instrument is not
 * being drawn at its regular size.
 */
static FGPanelAction *
readAction (const SGPropertyNode * node, float w_scale, float h_scale)
{
  unsigned int i, j;
  SGPropertyNode *binding;
  vector<SGPropertyNode_ptr>bindings = node->getChildren("binding");

  // button-less actions are fired initially
  if (!node->hasValue("w") || !node->hasValue("h")) {
    for (i = 0; i < bindings.size(); i++) {
      SGBinding b(bindings[i], globals->get_props());
      b.fire();
    }
    return 0;
  }

  string name = node->getStringValue("name");

  int button = node->getIntValue("button");
  int x = int(node->getIntValue("x") * w_scale);
  int y = int(node->getIntValue("y") * h_scale);
  int w = int(node->getIntValue("w") * w_scale);
  int h = int(node->getIntValue("h") * h_scale);
  bool repeatable = node->getBoolValue("repeatable", true);

  FGPanelAction * action = new FGPanelAction(button, x, y, w, h, repeatable);

  SGPropertyNode * dest = fgGetNode("/sim/bindings/panel", true);

  for (i = 0; i < bindings.size(); i++) {
    SG_LOG(SG_INPUT, SG_BULK, "Reading binding "
      << bindings[i]->getStringValue("command"));

    j = 0;
    while (dest->getChild("binding", j))
      j++;

    binding = dest->getChild("binding", j, true);
    copyProperties(bindings[i], binding);
    action->addBinding(new SGBinding(binding, globals->get_props()), 0);
  }

  if (node->hasChild("mod-up")) {
    bindings = node->getChild("mod-up")->getChildren("binding");
    for (i = 0; i < bindings.size(); i++) {
      j = 0;
      while (dest->getChild("binding", j))
        j++;

      binding = dest->getChild("binding", j, true);
      copyProperties(bindings[i], binding);
      action->addBinding(new SGBinding(binding, globals->get_props()), 1);
    }
  }

  readConditions(action, node);
  return action;
}


/**
 * Read a transformation from the instrument's property list.
 *
 * The panel module uses the transformations to slide or spin needles,
 * knobs, and other indicators, and to place layers in the correct
 * positions.  Every layer starts centered exactly on the x,y co-ordinate,
 * and many layers need to be moved or rotated simply to display the
 * instrument correctly.
 *
 * There are three types of transformations:
 *
 * "x-shift" - move the layer horizontally.
 *
 * "y-shift" - move the layer vertically.
 *
 * "rotation" - rotate the layer.
 *
 * Each transformation may have a fixed offset, and may also have
 * a floating-point property value to add to the offset.  The
 * floating-point property may be clamped to a minimum and/or
 * maximum range and scaled (after clamping).
 *
 * Note that because of the way OpenGL works, transformations will
 * appear to be applied backwards.
 */
static FGPanelTransformation *
readTransformation (const SGPropertyNode * node, float w_scale, float h_scale)
{
  FGPanelTransformation * t = new FGPanelTransformation;

  string name = node->getName();
  string type = node->getStringValue("type");
  string propName = node->getStringValue("property", "");
  SGPropertyNode * target = 0;

  if (type.empty()) {
    SG_LOG( SG_COCKPIT, SG_BULK,
            "No type supplied for transformation " << name
            << " assuming \"rotation\"" );
    type = "rotation";
  }

  if (!propName.empty()) {
    target = fgGetNode(propName.c_str(), true);
  }

  t->node = target;
  t->min = node->getFloatValue("min", -9999999);
  t->max = node->getFloatValue("max", 99999999);
  t->has_mod = node->hasChild("modulator");
  if (t->has_mod)
      t->mod = node->getFloatValue("modulator");
  t->factor = node->getFloatValue("scale", 1.0);
  t->offset = node->getFloatValue("offset", 0.0);

				// Check for an interpolation table
  const SGPropertyNode * trans_table = node->getNode("interpolation");
  if (trans_table != 0) {
    SG_LOG( SG_COCKPIT, SG_DEBUG, "Found interpolation table with "
            << trans_table->nChildren() << " children" );
    t->table = new SGInterpTable(trans_table);
  } else {
    t->table = 0;
  }
  
				// Move the layer horizontally.
  if (type == "x-shift") {
    t->type = FGPanelTransformation::XSHIFT;
//     t->min *= w_scale; //removed by Martin Dressler
//     t->max *= w_scale; //removed by Martin Dressler
    t->offset *= w_scale;
    t->factor *= w_scale; //Added by Martin Dressler
  } 

				// Move the layer vertically.
  else if (type == "y-shift") {
    t->type = FGPanelTransformation::YSHIFT;
    //t->min *= h_scale; //removed
    //t->max *= h_scale; //removed
    t->offset *= h_scale;
    t->factor *= h_scale; //Added
  } 

				// Rotate the layer.  The rotation
				// is in degrees, and does not need
				// to scale with the instrument size.
  else if (type == "rotation") {
    t->type = FGPanelTransformation::ROTATION;
  } 

  else {
    SG_LOG( SG_COCKPIT, SG_ALERT, "Unrecognized transformation type " << type );
    delete t;
    return 0;
  }

  readConditions(t, node);
  SG_LOG( SG_COCKPIT, SG_DEBUG, "Read transformation " << name );
  return t;
}


/**
 * Read a chunk of text from the instrument's property list.
 *
 * A text layer consists of one or more chunks of text.  All chunks
 * share the same font size and color (and eventually, font), but
 * each can come from a different source.  There are three types of
 * text chunks:
 *
 * "literal" - a literal text string (the default)
 *
 * "text-value" - the current value of a string property
 *
 * "number-value" - the current value of a floating-point property.
 *
 * All three may also include a printf-style format string.
 */
FGTextLayer::Chunk *
readTextChunk (const SGPropertyNode * node)
{
  FGTextLayer::Chunk * chunk;
  string name = node->getStringValue("name");
  string type = node->getStringValue("type");
  string format = node->getStringValue("format");

				// Default to literal text.
  if (type.empty()) {
    SG_LOG( SG_COCKPIT, SG_INFO, "No type provided for text chunk " << name
            << " assuming \"literal\"");
    type = "literal";
  }

				// A literal text string.
  if (type == "literal") {
    string text = node->getStringValue("text");
    chunk = new FGTextLayer::Chunk(text, format);
  }

				// The value of a string property.
  else if (type == "text-value") {
    SGPropertyNode * target =
      fgGetNode(node->getStringValue("property"), true);
    chunk = new FGTextLayer::Chunk(FGTextLayer::TEXT_VALUE, target, format);
  }

				// The value of a float property.
  else if (type == "number-value") {
    string propName = node->getStringValue("property");
    float scale = node->getFloatValue("scale", 1.0);
    float offset = node->getFloatValue("offset", 0.0);
    bool truncation = node->getBoolValue("truncate", false);
    SGPropertyNode * target = fgGetNode(propName.c_str(), true);
    chunk = new FGTextLayer::Chunk(FGTextLayer::DOUBLE_VALUE, target,
				   format, scale, offset, truncation);
  }

				// Unknown type.
  else {
    SG_LOG( SG_COCKPIT, SG_ALERT, "Unrecognized type " << type
            << " for text chunk " << name );
    return 0;
  }

  readConditions(chunk, node);
  return chunk;
}


/**
 * Read a single layer from an instrument's property list.
 *
 * Each instrument consists of one or more layers stacked on top
 * of each other; the lower layers show through only where the upper
 * layers contain an alpha component.  Each layer can be moved
 * horizontally and vertically and rotated using transformations.
 *
 * This module currently recognizes four kinds of layers:
 *
 * "texture" - a layer containing a texture (the default)
 *
 * "text" - a layer containing text
 *
 * "switch" - a layer that switches between two other layers
 *   based on the current value of a boolean property.
 *
 * "built-in" - a hard-coded layer supported by C++ code in FlightGear.
 *
 * Currently, the only built-in layer class is "compass-ribbon".
 */
static FGInstrumentLayer *
readLayer (const SGPropertyNode * node, float w_scale, float h_scale)
{
  FGInstrumentLayer * layer = NULL;
  string name = node->getStringValue("name");
  string type = node->getStringValue("type");
  int w = node->getIntValue("w", -1);
  int h = node->getIntValue("h", -1);
  bool emissive = node->getBoolValue("emissive", false);
  if (w != -1)
    w = int(w * w_scale);
  if (h != -1)
    h = int(h * h_scale);


  if (type.empty()) {
    SG_LOG( SG_COCKPIT, SG_BULK,
            "No type supplied for layer " << name
            << " assuming \"texture\"" );
    type = "texture";
  }


				// A textured instrument layer.
  if (type == "texture") {
    FGCroppedTexture texture = readTexture(node->getNode("texture"));
    layer = new FGTexturedLayer(texture, w, h);
    if (emissive) {
      FGTexturedLayer *tl=(FGTexturedLayer*)layer;
      tl->setEmissive(true);
    }

  }

				// A group of sublayers.
  else if (type == "group") {
    layer = new FGGroupLayer();
    for (int i = 0; i < node->nChildren(); i++) {
      const SGPropertyNode * child = node->getChild(i);
      if (!strcmp(child->getName(), "layer"))
	((FGGroupLayer *)layer)->addLayer(readLayer(child, w_scale, h_scale));
    }
  }


				// A textual instrument layer.
  else if (type == "text") {
    FGTextLayer * tlayer = new FGTextLayer(w, h); // FIXME

				// Set the text color.
    float red = node->getFloatValue("color/red", 0.0);
    float green = node->getFloatValue("color/green", 0.0);
    float blue = node->getFloatValue("color/blue", 0.0);
    tlayer->setColor(red, green, blue);

				// Set the point size.
    float pointSize = node->getFloatValue("point-size", 10.0) * w_scale;
    tlayer->setPointSize(pointSize);

				// Set the font.
    string fontName = node->getStringValue("font", "Helvetica");
    tlayer->setFontName(fontName);

    const SGPropertyNode * chunk_group = node->getNode("chunks");
    if (chunk_group != 0) {
      int nChunks = chunk_group->nChildren();
      for (int i = 0; i < nChunks; i++) {
	const SGPropertyNode * node = chunk_group->getChild(i);
	if (!strcmp(node->getName(), "chunk")) {
	  FGTextLayer::Chunk * chunk = readTextChunk(node);
	  if (chunk != 0)
	    tlayer->addChunk(chunk);
	} else {
	  SG_LOG( SG_COCKPIT, SG_INFO, "Skipping " << node->getName()
                  << " in chunks" );
	}
      }
      layer = tlayer;
    }
  }

				// A switch instrument layer.
  else if (type == "switch") {
    layer = new FGSwitchLayer();
    for (int i = 0; i < node->nChildren(); i++) {
      const SGPropertyNode * child = node->getChild(i);
      if (!strcmp(child->getName(), "layer"))
	((FGGroupLayer *)layer)->addLayer(readLayer(child, w_scale, h_scale));
    }
  }

				// A built-in instrument layer.
  else if (type == "built-in") {
    string layerclass = node->getStringValue("class");

    if (layerclass == "mag-ribbon") {
      layer = new FGMagRibbon(w, h);
    }

    else if (layerclass.empty()) {
      SG_LOG( SG_COCKPIT, SG_ALERT, "No class provided for built-in layer "
              << name );
      return 0;
    }

    else {
      SG_LOG( SG_COCKPIT, SG_ALERT, "Unknown built-in layer class "
              << layerclass);
      return 0;
    }
  }

				// An unknown type.
  else {
    SG_LOG( SG_COCKPIT, SG_ALERT, "Unrecognized layer type " << type );
    delete layer;
    return 0;
  }
  
  //
  // Get the transformations for each layer.
  //
  const SGPropertyNode * trans_group = node->getNode("transformations");
  if (trans_group != 0) {
    int nTransformations = trans_group->nChildren();
    for (int i = 0; i < nTransformations; i++) {
      const SGPropertyNode * node = trans_group->getChild(i);
      if (!strcmp(node->getName(), "transformation")) {
	FGPanelTransformation * t = readTransformation(node, w_scale, h_scale);
	if (t != 0)
	  layer->addTransformation(t);
      } else {
	SG_LOG( SG_COCKPIT, SG_INFO, "Skipping " << node->getName()
                << " in transformations" );
      }
    }
  }

  readConditions(layer, node);
  SG_LOG( SG_COCKPIT, SG_DEBUG, "Read layer " << name );
  return layer;
}


/**
 * Read an instrument from a property list.
 *
 * The instrument consists of a preferred width and height
 * (the panel may override these), together with a list of layers
 * and a list of actions to be performed when the user clicks 
 * the mouse over the instrument.  All co-ordinates are relative
 * to the instrument's position, so instruments are fully relocatable;
 * likewise, co-ordinates for actions and transformations will be
 * scaled automatically if the instrument is not at its preferred size.
 */
static FGPanelInstrument *
readInstrument (const SGPropertyNode * node)
{
  const string name = node->getStringValue("name");
  int x = node->getIntValue("x", -1);
  int y = node->getIntValue("y", -1);
  int real_w = node->getIntValue("w", -1);
  int real_h = node->getIntValue("h", -1);
  int w = node->getIntValue("w-base", -1);
  int h = node->getIntValue("h-base", -1);

  if (x == -1 || y == -1) {
    SG_LOG( SG_COCKPIT, SG_ALERT,
            "x and y positions must be specified and > 0" );
    return 0;
  }

  float w_scale = 1.0;
  float h_scale = 1.0;
  if (real_w != -1) {
    w_scale = float(real_w) / float(w);
    w = real_w;
  }
  if (real_h != -1) {
    h_scale = float(real_h) / float(h);
    h = real_h;
  }

  SG_LOG( SG_COCKPIT, SG_DEBUG, "Reading instrument " << name );

  FGLayeredInstrument * instrument =
    new FGLayeredInstrument(x, y, w, h);

  //
  // Get the actions for the instrument.
  //
  const SGPropertyNode * action_group = node->getNode("actions");
  if (action_group != 0) {
    int nActions = action_group->nChildren();
    for (int i = 0; i < nActions; i++) {
      const SGPropertyNode * node = action_group->getChild(i);
      if (!strcmp(node->getName(), "action")) {
	FGPanelAction * action = readAction(node, w_scale, h_scale);
	if (action != 0)
	  instrument->addAction(action);
      } else {
	SG_LOG( SG_COCKPIT, SG_INFO, "Skipping " << node->getName()
                << " in actions" );
      }
    }
  }

  //
  // Get the layers for the instrument.
  //
  const SGPropertyNode * layer_group = node->getNode("layers");
  if (layer_group != 0) {
    int nLayers = layer_group->nChildren();
    for (int i = 0; i < nLayers; i++) {
      const SGPropertyNode * node = layer_group->getChild(i);
      if (!strcmp(node->getName(), "layer")) {
	FGInstrumentLayer * layer = readLayer(node, w_scale, h_scale);
	if (layer != 0)
	  instrument->addLayer(layer);
      } else {
	SG_LOG( SG_COCKPIT, SG_INFO, "Skipping " << node->getName()
                << " in layers" );
      }
    }
  }

  readConditions(instrument, node);
  return instrument;
}


/**
 * Construct the panel from a property tree.
 */
FGPanel *
readPanel (const SGPropertyNode * root)
{
  SG_LOG( SG_COCKPIT, SG_INFO, "Reading properties for panel " <<
          root->getStringValue("name", "[Unnamed Panel]") );

  FGPanel * panel = new FGPanel();
  panel->setWidth(root->getIntValue("w", 1024));
  panel->setHeight(root->getIntValue("h", 443));

  //
  // Grab the visible external viewing area, default to 
  //
//  panel->setViewHeight(root->getIntValue("view-height",
//					 768 - panel->getHeight() + 2));

  //
  // Grab the panel's initial offsets, default to 0, 0.
  //
  if (!fgHasNode("/sim/panel/x-offset"))
    fgSetInt("/sim/panel/x-offset", root->getIntValue("x-offset", 0));

  // conditional removed by jim wilson to allow panel xml code 
  // with y-offset defined to work...
  if (!fgHasNode("/sim/panel/y-offset"))
    fgSetInt("/sim/panel/y-offset", root->getIntValue("y-offset", 0));

  panel->setAutohide(root->getBoolValue("autohide", true));

  //
  // Assign the background texture, if any, or a bogus chequerboard.
  //
  string bgTexture = root->getStringValue("background");
  if (bgTexture.empty())
    bgTexture = "FOO";
  panel->setBackground(FGTextureManager::createTexture(bgTexture.c_str()));

  //
  // Get multibackground if any...
  //
  string mbgTexture = root->getStringValue("multibackground[0]");
  if (!mbgTexture.empty()) {
    panel->setMultiBackground(FGTextureManager::createTexture(mbgTexture.c_str()), 0);

    mbgTexture = root->getStringValue("multibackground[1]");
    if (mbgTexture.empty())
      mbgTexture = "FOO";
    panel->setMultiBackground(FGTextureManager::createTexture(mbgTexture.c_str()), 1);

    mbgTexture = root->getStringValue("multibackground[2]");
    if (mbgTexture.empty())
      mbgTexture = "FOO";
    panel->setMultiBackground(FGTextureManager::createTexture(mbgTexture.c_str()), 2);

    mbgTexture = root->getStringValue("multibackground[3]");
    if (mbgTexture.empty())
      mbgTexture = "FOO";
    panel->setMultiBackground(FGTextureManager::createTexture(mbgTexture.c_str()), 3);

    mbgTexture = root->getStringValue("multibackground[4]");
    if (mbgTexture.empty())
      mbgTexture = "FOO";
    panel->setMultiBackground(FGTextureManager::createTexture(mbgTexture.c_str()), 4);

    mbgTexture = root->getStringValue("multibackground[5]");
    if (mbgTexture.empty())
      mbgTexture = "FOO";
    panel->setMultiBackground(FGTextureManager::createTexture(mbgTexture.c_str()), 5);

    mbgTexture = root->getStringValue("multibackground[6]");
    if (mbgTexture.empty())
      mbgTexture = "FOO";
    panel->setMultiBackground(FGTextureManager::createTexture(mbgTexture.c_str()), 6);

    mbgTexture = root->getStringValue("multibackground[7]");
    if (mbgTexture.empty())
      mbgTexture = "FOO";
    panel->setMultiBackground(FGTextureManager::createTexture(mbgTexture.c_str()), 7);

  }
  


  //
  // Create each instrument.
  //
  SG_LOG( SG_COCKPIT, SG_DEBUG, "Reading panel instruments" );
  const SGPropertyNode * instrument_group = root->getChild("instruments");
  if (instrument_group != 0) {
    int nInstruments = instrument_group->nChildren();
    for (int i = 0; i < nInstruments; i++) {
      const SGPropertyNode * node = instrument_group->getChild(i);
      if (!strcmp(node->getName(), "instrument")) {
        FGPanelInstrument * instrument = readInstrument(node);
        if (instrument != 0)
          panel->addInstrument(instrument);
      } else if (!strcmp(node->getName(), "special-instrument")) {
        //cout << "Special instrument found in instruments section!\n";
        const string name = node->getStringValue("name");
        if (name == "KLN89 GPS") {
          //cout << "Special instrument is KLN89\n";
          
          int x = node->getIntValue("x", -1);
          int y = node->getIntValue("y", -1);
          int real_w = node->getIntValue("w", -1);
          int real_h = node->getIntValue("h", -1);
//          int w = node->getIntValue("w-base", -1);
//          int h = node->getIntValue("h-base", -1);
          
          if (x == -1 || y == -1) {
            SG_LOG( SG_COCKPIT, SG_ALERT,
            "x and y positions must be specified and > 0" );
            return 0;
          }
          
//          float w_scale = 1.0;
//          float h_scale = 1.0;
          if (real_w != -1) {
//            w_scale = float(real_w) / float(w);
//            w = real_w;
          }
          if (real_h != -1) {
//            h_scale = float(real_h) / float(h);
//            h = real_h;
          }
          
          SG_LOG( SG_COCKPIT, SG_BULK, "Reading instrument " << name );
          
          // Warning - hardwired size!!!
          RenderArea2D* instrument = new RenderArea2D(158, 40, 158, 40, x, y);
          KLN89* gps = (KLN89*)globals->get_subsystem("kln89");
		  if (gps == NULL) {
			  gps = new KLN89(instrument);
			  globals->add_subsystem("kln89", gps);
		  }
		  //gps->init();  // init seems to get called automagically.
		  FGSpecialInstrument* gpsinst = new FGSpecialInstrument(gps);
          panel->addInstrument(gpsinst);
        } else {
          SG_LOG( SG_COCKPIT, SG_WARN, "Unknown special instrument found" );
        }
      } else {
        SG_LOG( SG_COCKPIT, SG_WARN, "Skipping " << node->getName()
        << " in instruments section" );
      }
    }
  }
  SG_LOG( SG_COCKPIT, SG_BULK, "Done reading panel instruments" );


  //
  // Return the new panel.
  //
  return panel;
}


/**
 * Read a panel from a property list.
 *
 * Each panel instrument will appear in its own, separate
 * property list.  The top level simply names the panel and
 * places the instruments in their appropriate locations (and
 * optionally resizes them if necessary).
 *
 * Returns 0 if the read fails for any reason.
 */
FGPanel *
fgReadPanel (istream &input)
{
  SGPropertyNode root;

  try {
    readProperties(input, &root);
  } catch (const sg_exception &e) {
    guiErrorMessage("Error reading panel: ", e);
    return 0;
  }
  return readPanel(&root);
}


/**
 * Read a panel from a property list.
 *
 * This function opens a stream to a file, then invokes the
 * main fgReadPanel() function.
 */
FGPanel *
fgReadPanel (const string &relative_path)
{
  SGPath path = globals->resolve_aircraft_path(relative_path);
  SGPropertyNode root;

  if (!path.exists()) {
      auto msg = string{"Missing panel file: "} + relative_path;
      guiErrorMessage(msg.c_str());
      return nullptr;
  }

  try {
    readProperties(path, &root);
  } catch (const sg_exception &e) {
    guiErrorMessage("Error reading panel: ", e);
    return 0;
  }
  return readPanel(&root);
}



// end of panel_io.cxx



