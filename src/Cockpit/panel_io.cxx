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
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/props.hxx>

#include <iostream>
#include <string>

#include "panel.hxx"

using std::istream;
using std::string;



////////////////////////////////////////////////////////////////////////
// Read and construct a panel.
////////////////////////////////////////////////////////////////////////


/**
 * Read a cropped texture.
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
 * Read an action.
 */
static FGPanelAction *
readAction (SGPropertyNode node)
{
  FGPanelAction * action = 0;

  cerr << "Reading action\n";

  string type = node.getStringValue("type");

  int button = node.getIntValue("button");
  int x = node.getIntValue("x");
  int y = node.getIntValue("y");
  int w = node.getIntValue("w");
  int h = node.getIntValue("h");

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

				// No type supplied
  else if (type == "") {
    FG_LOG(FG_INPUT, FG_ALERT, "No type specified for action "
	   << node.getName());
    return 0;
  } 

				// Unrecognized type
  else {
    FG_LOG(FG_INPUT, FG_ALERT, "Unrecognized action type " << node.getName());
    return 0;
  }

  return action;
}


/**
 * Read a single transformation.
 */
static FGPanelTransformation *
readTransformation (SGPropertyNode node)
{
  FGPanelTransformation * t = new FGPanelTransformation;

  string name = node.getName();
  string type = node.getStringValue("type");
  string propName = node.getStringValue("property", "");
  SGValue * value = 0;

  if (propName != "") {
    value = current_properties.getValue(propName, true);
  }

  t->value = value;
  t->min = node.getFloatValue("min", 0.0);
  t->max = node.getFloatValue("max", 1.0);
  t->factor = node.getFloatValue("factor", 1.0);
  t->offset = node.getFloatValue("offset", 0.0);
  if (type == "x-shift") {
    t->type = FGPanelTransformation::XSHIFT;
  } else if (type == "y-shift") {
    t->type = FGPanelTransformation::YSHIFT;
  } else if (type == "rotation") {
    t->type = FGPanelTransformation::ROTATION;
  } else if (type == "") {
    FG_LOG(FG_INPUT, FG_ALERT,
	   "'type' must be specified for transformation");
    delete t;
    return 0;
  } else {
    FG_LOG(FG_INPUT, FG_ALERT, "Unrecognized transformation: " << type);
    delete t;
    return 0;
  }

  FG_LOG(FG_INPUT, FG_INFO, "Read transformation " << name);
  return t;
}


/**
 * Read a single layer of an instrument.
 */
static FGInstrumentLayer *
readLayer (SGPropertyNode node)
{
  FGInstrumentLayer * l;
  string name = node.getName();

  CroppedTexture texture = readTexture(node.getSubNode("texture"));
  l = new FGTexturedLayer(texture,
			  node.getIntValue("w", -1),
			  node.getIntValue("h", -1));
  
  //
  // Get the transformations for each layer.
  //
  SGPropertyNode trans_group = node.getSubNode("transformations");
  int nTransformations = trans_group.size();
  for (int k = 0; k < nTransformations; k++) {
    FGPanelTransformation * t = readTransformation(trans_group.getChild(k));
    if (t == 0) {
      delete l;
      return 0;
    }
    l->addTransformation(t);
  }
  
  FG_LOG(FG_INPUT, FG_INFO, "Read layer " << name);
  return l;
}


/**
 * Read an instrument.
 */
static FGPanelInstrument *
readInstrument (SGPropertyNode node)
{
  const string &name = node.getStringValue("name");

  FG_LOG(FG_INPUT, FG_INFO, "Reading instrument " << name);

  FGLayeredInstrument * instrument =
    new FGLayeredInstrument(0, 0,
			    node.getIntValue("w"),
			    node.getIntValue("h"));

  //
  // Get the actions for the instrument.
  //
  SGPropertyNode action_group = node.getSubNode("actions");
  int nActions = action_group.size();
  cerr << "There are " << nActions << " actions\n";
  for (int j = 0; j < nActions; j++) {
    FGPanelAction * action = readAction(action_group.getChild(j));
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
    FGInstrumentLayer * layer = readLayer(layer_group.getChild(j));
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
 * Read a property list defining an instrument panel.
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

    string path = node.getStringValue("path");
    int x = node.getIntValue("x");
    int y = node.getIntValue("y");
    int w = node.getIntValue("w");
    int h = node.getIntValue("h");

    FG_LOG(FG_INPUT, FG_INFO, "Reading instrument "
	   << node.getName()
	   << " from "
	   << path);

    if (!readPropertyList(path, &props2)) {
      delete panel;
      return 0;
    }

    FGPanelInstrument * instrument =
      readInstrument(SGPropertyNode("/", &props2));
    if (instrument == 0) {
      delete instrument;
      delete panel;
      return 0;
    }
    instrument->setPosition(x, y);
    panel->addInstrument(instrument);
  }
  FG_LOG(FG_INPUT, FG_INFO, "Done reading panel instruments");


  //
  // Return the new panel.
  //
  return panel;
}


// end of panel_io.cxx
