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
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//  $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H          
#  include <windows.h>
#endif

#include <simgear/compiler.h>
#include <simgear/misc/fgpath.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/props.hxx>

#include <iostream>
#include <fstream>
#include <string>

#include <Main/options.hxx>

#include "panel.hxx"
#include "steam.hxx"
#include "panel_io.hxx"

using std::istream;
using std::ifstream;
using std::string;



////////////////////////////////////////////////////////////////////////
// Built-in layer for the magnetic compass ribbon layer.
//
// TODO: move this out into a special directory for built-in
// layers of various sorts.
////////////////////////////////////////////////////////////////////////

class FGMagRibbon : public FGTexturedLayer
{
public:
  FGMagRibbon (int w, int h);
  virtual ~FGMagRibbon () {}

  virtual void draw ();
};

FGMagRibbon::FGMagRibbon (int w, int h)
  : FGTexturedLayer(w, h)
{
  CroppedTexture texture("Textures/Panel/compass-ribbon.rgb");
  setTexture(texture);
}

void
FGMagRibbon::draw ()
{
  double heading = FGSteam::get_MH_deg();
  double xoffset, yoffset;

  while (heading >= 360.0) {
    heading -= 360.0;
  }
  while (heading < 0.0) {
    heading += 360.0;
  }

  if (heading >= 60.0 && heading <= 180.0) {
    xoffset = heading / 240.0;
    yoffset = 0.75;
  } else if (heading >= 150.0 && heading <= 270.0) {
    xoffset = (heading - 90.0) / 240.0;
    yoffset = 0.50;
  } else if (heading >= 240.0 && heading <= 360.0) {
    xoffset = (heading - 180.0) / 240.0;
    yoffset = 0.25;
  } else {
    if (heading < 270.0)
      heading += 360.0;
    xoffset = (heading - 270.0) / 240.0;
    yoffset = 0.0;
  }

  xoffset = 1.0 - xoffset;
				// Adjust to put the number in the centre
  xoffset -= 0.25;

  CroppedTexture &t = getTexture();
  t.minX = xoffset;
  t.minY = yoffset;
  t.maxX = xoffset + 0.5;
  t.maxY = yoffset + 0.25;
  FGTexturedLayer::draw();
}



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
static CroppedTexture
readTexture (SGPropertyNode node)
{
  CroppedTexture texture(node.getStringValue("path"),
			 node.getFloatValue("x1"),
			 node.getFloatValue("y1"),
			 node.getFloatValue("x2", 1.0),
			 node.getFloatValue("y2", 1.0));
  FG_LOG(FG_INPUT, FG_INFO, "Read texture " << node.getName());
  return texture;
}


/**
 * Read an action from the instrument's property list.
 *
 * The action will be performed when the user clicks a mouse button
 * within the specified region of the instrument.  Actions always
 * work by modifying the value of a property (see the SGValue class).
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
readAction (SGPropertyNode node, float hscale, float vscale)
{
  FGPanelAction * action = 0;

  string name = node.getStringValue("name");
  string type = node.getStringValue("type");

  int button = node.getIntValue("button");
  int x = int(node.getIntValue("x") * hscale);
  int y = int(node.getIntValue("y") * vscale);
  int w = int(node.getIntValue("w") * hscale);
  int h = int(node.getIntValue("h") * vscale);

  if (type == "") {
    FG_LOG(FG_INPUT, FG_ALERT,
	   "No type supplied for action " << name << " assuming \"adjust\"");
    type = "adjust";
  }

				// Adjust a property value
  if (type == "adjust") {
    string propName = node.getStringValue("property");
    SGValue * value = current_properties.getValue(propName, true);
    float increment = node.getFloatValue("increment", 1.0);
    float min = node.getFloatValue("min", 0.0);
    float max = node.getFloatValue("max", 0.0);
    bool wrap = node.getBoolValue("wrap", false);
    if (min == max)
      FG_LOG(FG_INPUT, FG_ALERT, "Action " << node.getName()
	     << " has same min and max value");
    action = new FGAdjustAction(button, x, y, w, h, value,
				increment, min, max, wrap);
  } 

				// Swap two property values
  else if (type == "swap") {
    string propName1 = node.getStringValue("property1");
    string propName2 = node.getStringValue("property2");
    SGValue * value1 = current_properties.getValue(propName1, true);
    SGValue * value2 = current_properties.getValue(propName2, true);
    action = new FGSwapAction(button, x, y, w, h, value1, value2);
  } 

				// Toggle a boolean value
  else if (type == "toggle") {
    string propName = node.getStringValue("property");
    SGValue * value = current_properties.getValue(propName, true);
    action = new FGToggleAction(button, x, y, w, h, value);
  } 

				// Unrecognized type
  else {
    FG_LOG(FG_INPUT, FG_ALERT, "Unrecognized action type " << type);
    return 0;
  }

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
readTransformation (SGPropertyNode node, float hscale, float vscale)
{
  FGPanelTransformation * t = new FGPanelTransformation;

  string name = node.getName();
  string type = node.getStringValue("type");
  string propName = node.getStringValue("property", "");
  SGValue * value = 0;

  if (type == "") {
    FG_LOG(FG_INPUT, FG_ALERT,
	   "No type supplied for transformation " << name
	   << " assuming \"rotation\"");
    type = "rotation";
  }

  if (propName != "") {
    value = current_properties.getValue(propName, true);
  }

  t->value = value;
  t->min = node.getFloatValue("min", -9999999);
  t->max = node.getFloatValue("max", 99999999);
  t->factor = node.getFloatValue("scale", 1.0);
  t->offset = node.getFloatValue("offset", 0.0);

				// Move the layer horizontally.
  if (type == "x-shift") {
    t->type = FGPanelTransformation::XSHIFT;
    t->min *= hscale;
    t->max *= hscale;
    t->offset *= hscale;
  } 

				// Move the layer vertically.
  else if (type == "y-shift") {
    t->type = FGPanelTransformation::YSHIFT;
    t->min *= vscale;
    t->max *= vscale;
    t->offset *= vscale;
  } 

				// Rotate the layer.  The rotation
				// is in degrees, and does not need
				// to scale with the instrument size.
  else if (type == "rotation") {
    t->type = FGPanelTransformation::ROTATION;
  } 

  else {
    FG_LOG(FG_INPUT, FG_ALERT, "Unrecognized transformation type " << type);
    delete t;
    return 0;
  }

  FG_LOG(FG_INPUT, FG_INFO, "Read transformation " << name);
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
readTextChunk (SGPropertyNode node)
{
  FGTextLayer::Chunk * chunk;
  string name = node.getStringValue("name");
  string type = node.getStringValue("type");
  string format = node.getStringValue("format");

				// Default to literal text.
  if (type == "") {
    FG_LOG(FG_INPUT, FG_INFO, "No type provided for text chunk " << name
	   << " assuming \"literal\"");
    type = "literal";
  }

				// A literal text string.
  if (type == "literal") {
    string text = node.getStringValue("text");
    chunk = new FGTextLayer::Chunk(text, format);
  }

				// The value of a string property.
  else if (type == "text-value") {
    SGValue * value =
      current_properties.getValue(node.getStringValue("property"), true);
    chunk = new FGTextLayer::Chunk(FGTextLayer::TEXT_VALUE, value, format);
  }

				// The value of a float property.
  else if (type == "number-value") {
    string propName = node.getStringValue("property");
    float scale = node.getFloatValue("scale", 1.0);
    SGValue * value = current_properties.getValue(propName, true);
    chunk = new FGTextLayer::Chunk(FGTextLayer::DOUBLE_VALUE, value,
				   format, scale);
  }

				// Unknown type.
  else {
    FG_LOG(FG_INPUT, FG_ALERT, "Unrecognized type " << type
	   << " for text chunk " << name);
    return 0;
  }

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
readLayer (SGPropertyNode node, float hscale, float vscale)
{
  FGInstrumentLayer * layer;
  string name = node.getStringValue("name");
  string type = node.getStringValue("type");
  int w = node.getIntValue("w", -1);
  int h = node.getIntValue("h", -1);
  if (w != -1)
    w = int(w * hscale);
  if (h != -1)
    h = int(h * vscale);


  if (type == "") {
    FG_LOG(FG_INPUT, FG_ALERT,
	   "No type supplied for layer " << name
	   << " assuming \"texture\"");
    type = "texture";
  }


				// A textured instrument layer.
  if (type == "texture") {
    CroppedTexture texture = readTexture(node.getSubNode("texture"));
    layer = new FGTexturedLayer(texture, w, h);
  }


				// A textual instrument layer.
  else if (type == "text") {
    FGTextLayer * tlayer = new FGTextLayer(w, h); // FIXME

				// Set the text color.
    float red = node.getFloatValue("color/red", 0.0);
    float green = node.getFloatValue("color/green", 0.0);
    float blue = node.getFloatValue("color/blue", 0.0);
    tlayer->setColor(red, green, blue);

				// Set the point size.
    float pointSize = node.getFloatValue("point-size", 10.0) * hscale;
    tlayer->setPointSize(pointSize);

				// Set the font.
    // TODO

    SGPropertyNode chunk_group = node.getSubNode("chunks");
    int nChunks = chunk_group.size();
    for (int i = 0; i < nChunks; i++) {
      FGTextLayer::Chunk * chunk = readTextChunk(chunk_group.getChild(i));
      if (chunk == 0) {
	delete layer;
	return 0;
      }
      tlayer->addChunk(chunk);
    }
    layer = tlayer;
  }

				// A switch instrument layer.
  else if (type == "switch") {
    SGValue * value =
      current_properties.getValue(node.getStringValue("property"));
    FGInstrumentLayer * layer1 =
      readLayer(node.getSubNode("layer1"), hscale, vscale);
    FGInstrumentLayer * layer2 =
      readLayer(node.getSubNode("layer2"), hscale, vscale);
    layer = new FGSwitchLayer(w, h, value, layer1, layer2);
  }

				// A built-in instrument layer.
  else if (type == "built-in") {
    string layerclass = node.getStringValue("class");

    if (layerclass == "mag-ribbon") {
      layer = new FGMagRibbon(w, h);
    }

    else if (layerclass == "") {
      FG_LOG(FG_INPUT, FG_ALERT, "No class provided for built-in layer "
	     << name);
      return 0;
    }

    else {
      FG_LOG(FG_INPUT, FG_ALERT, "Unknown built-in layer class "
	     << layerclass);
      return 0;
    }
  }

				// An unknown type.
  else {
    FG_LOG(FG_INPUT, FG_ALERT, "Unrecognized layer type " << type);
    delete layer;
    return 0;
  }
  
  //
  // Get the transformations for each layer.
  //
  SGPropertyNode trans_group = node.getSubNode("transformations");
  int nTransformations = trans_group.size();
  for (int k = 0; k < nTransformations; k++) {
    FGPanelTransformation * t = readTransformation(trans_group.getChild(k),
						   hscale, vscale);
    if (t == 0) {
      delete layer;
      return 0;
    }
    layer->addTransformation(t);
  }
  
  FG_LOG(FG_INPUT, FG_INFO, "Read layer " << name);
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
readInstrument (SGPropertyNode node, int x, int y, int real_w, int real_h)
{
  int w = node.getIntValue("w");
  int h = node.getIntValue("h");
  const string &name = node.getStringValue("name");

  float hscale = 1.0;
  float vscale = 1.0;
  if (real_w != -1) {
    hscale = float(real_w) / float(w);
    w = real_w;
    cerr << "hscale is " << hscale << endl;
  }
  if (real_h != -1) {
    vscale = float(real_h) / float(h);
    h = real_h;
    cerr << "vscale is " << hscale << endl;
  }

  FG_LOG(FG_INPUT, FG_INFO, "Reading instrument " << name);

  FGLayeredInstrument * instrument =
    new FGLayeredInstrument(x, y, w, h);

  //
  // Get the actions for the instrument.
  //
  SGPropertyNode action_group = node.getSubNode("actions");
  int nActions = action_group.size();
  for (int j = 0; j < nActions; j++) {
    FGPanelAction * action = readAction(action_group.getChild(j),
					hscale, vscale);
    if (action == 0) {
      delete instrument;
      return 0;
    }
    instrument->addAction(action);
  }

  //
  // Get the layers for the instrument.
  //
  SGPropertyNode layer_group = node.getSubNode("layers");
  int nLayers = layer_group.size();
  for (int j = 0; j < nLayers; j++) {
    FGInstrumentLayer * layer = readLayer(layer_group.getChild(j),
					  hscale, vscale);
    if (layer == 0) {
      delete instrument;
      return 0;
    }
    instrument->addLayer(layer);
  }

  FG_LOG(FG_INPUT, FG_INFO, "Done reading instrument " << name);
  return instrument;
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
  SGPropertyList props;


  //
  // Read the property list from disk.
  //
  if (!readPropertyList(input, &props)) {
    FG_LOG(FG_INPUT, FG_ALERT, "Malformed property list for panel.");
    return 0;
  }
  FG_LOG(FG_INPUT, FG_INFO, "Read properties for panel " <<
	 props.getStringValue("/name"));

  //
  // Construct a new, empty panel.
  //
  FGPanel * panel = new FGPanel(0, 0, 1024, 768);// FIXME: use variable size

  //
  // Assign the background texture, if any, or a bogus chequerboard.
  //
  string bgTexture = props.getStringValue("/background");
  if (bgTexture == "")
    bgTexture = "FOO";
  panel->setBackground(FGTextureManager::createTexture(bgTexture.c_str()));
  FG_LOG(FG_INPUT, FG_INFO, "Set background texture to " << bgTexture);


  //
  // Create each instrument.
  //
  FG_LOG(FG_INPUT, FG_INFO, "Reading panel instruments");
  SGPropertyNode instrument_group("/instruments", &props);
  int nInstruments = instrument_group.size();
  for (int i = 0; i < nInstruments; i++) {
    SGPropertyList props2;
    SGPropertyNode node = instrument_group.getChild(i);

    FGPath path(current_options.get_fg_root());
    path.append(node.getStringValue("path"));

    FG_LOG(FG_INPUT, FG_INFO, "Reading instrument "
	   << node.getName()
	   << " from "
	   << path.str());

    int x = node.getIntValue("x", -1);
    int y = node.getIntValue("y", -1);
    int w = node.getIntValue("w", -1);
    int h = node.getIntValue("h", -1);

    if (x == -1 || y == -1) {
      FG_LOG(FG_INPUT, FG_ALERT, "x and y positions must be specified and >0");
      delete panel;
      return 0;
    }

    if (!readPropertyList(path.str(), &props2)) {
      delete panel;
      return 0;
    }

    FGPanelInstrument * instrument =
      readInstrument(SGPropertyNode("/", &props2), x, y, w, h);
    if (instrument == 0) {
      delete instrument;
      delete panel;
      return 0;
    }
    panel->addInstrument(instrument);
  }
  FG_LOG(FG_INPUT, FG_INFO, "Done reading panel instruments");


  //
  // Return the new panel.
  //
  return panel;
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
  FGPath path(current_options.get_fg_root());
  path.append(relative_path);
  ifstream input(path.c_str());
  if (!input.good()) {
    FG_LOG(FG_INPUT, FG_ALERT,
	   "Cannot read panel configuration from " << path.str());
    return 0;
  }
  FGPanel * panel = fgReadPanel(input);
  input.close();
  return panel;
}



// end of panel_io.cxx
