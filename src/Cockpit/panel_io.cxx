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
#include <map>

#include "panel.hxx"

using std::istream;
using std::hash_map;
using std::string;



////////////////////////////////////////////////////////////////////////
// Instruments.
////////////////////////////////////////////////////////////////////////

struct ActionData
{
  FGPanelAction * action;
  int button, x, y, w, h;
};

struct TransData
{
  enum Type {
    End = 0,
    Rotation,
    XShift,
    YShift
  };
  Type type;
  const char * propName;
  float min, max, factor, offset;
};

struct LayerData
{
  FGInstrumentLayer * layer;
  TransData transformations[16];
};

struct InstrumentData
{
  const char * name;
  int x, y, w, h;
  ActionData actions[16];
  LayerData layers[16];
};



////////////////////////////////////////////////////////////////////////
// Read and construct a panel.
////////////////////////////////////////////////////////////////////////


/**
 * Read a property list defining an instrument panel.
 *
 * Returns 0 if the read fails for any reason.
 */
FGPanel *
fgReadPanel (istream &input)
{
  SGPropertyList props;
  map<string,CroppedTexture> textures;


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
  // Read the cropped textures from the property list.
  //
  SGPropertyNode texture_group("/textures", &props);
  int nTextures = texture_group.size();
  cout << "There are " << nTextures << " textures" << endl;
  for (int i = 0; i < nTextures; i++) {
    SGPropertyNode tex = texture_group.getChild(i);
    const string &name = tex.getName();
    textures[name] = CroppedTexture(tex.getStringValue("path"),
				    tex.getFloatValue("x1"),
				    tex.getFloatValue("y1"),
				    tex.getFloatValue("x2", 1.0),
				    tex.getFloatValue("y2", 1.0));
    FG_LOG(FG_INPUT, FG_INFO, "Read texture " << name);
  }


  //
  // Construct a new, empty panel.
  //
  FGPanel * panel = new FGPanel(0, 0, 1024, 768);// FIXME: use variable size


  //
  // Assign the background texture, if any, or a bogus chequerboard.
  //
  string bgTexture = props.getStringValue("/background");
  if (bgTexture == "") {
    panel->setBackground(FGTextureManager::createTexture("FOO"));
  } else {
    panel->setBackground(textures[bgTexture].texture);
  }
  FG_LOG(FG_INPUT, FG_INFO, "Set background texture to " << bgTexture);


  //
  // Create each instrument.
  //
  FG_LOG(FG_INPUT, FG_INFO, "Reading panel instruments");
  SGPropertyNode instrument_group("/instruments", &props);
  int nInstruments = instrument_group.size();
  cout << "There are " << nInstruments << " instruments" << endl;
  for (int i = 0; i < nInstruments; i++) {
    SGPropertyNode inst = instrument_group.getChild(i);
    const string &name = inst.getName();
    FGLayeredInstrument * instrument =
      new FGLayeredInstrument(inst.getIntValue("x"),
			      inst.getIntValue("y"),
			      inst.getIntValue("w"),
			      inst.getIntValue("h"));


    //
    // Get the layers for each instrument.
    //
    SGPropertyNode layer_group = inst.getSubNode("layers");
    SGPropertyNode layer;
    int nLayers = layer_group.size();
    cout << "There are " << nLayers << " layers" << endl;
    for (int j = 0; j < nLayers; j++) {
      layer = layer_group.getChild(j);
      FGInstrumentLayer * l;
      string name = layer.getName();

      string tex = layer.getStringValue("texture");
      if (tex != "") {
	l = new FGTexturedLayer(textures[tex],
				layer.getIntValue("w", -1),
				layer.getIntValue("h", -1));
      } else {
	FG_LOG(FG_INPUT, FG_ALERT, "No texture for layer " << name);
	return 0;
      }

      //
      // Get the transformations for each layer.
      //
      SGPropertyNode trans_group = layer.getSubNode("transformations");
      SGPropertyNode trans;
      int nTransformations = trans_group.size();
      cout << "There are " << nTransformations << " transformations" << endl;
      for (int k = 0; k < nTransformations; k++) {
	trans = trans_group.getChild(k);
	string name = trans.getName();
	string type = trans.getStringValue("type");
	string propName = trans.getStringValue("property", "");
	SGValue * value = 0;
	cout << "Type is " << type << endl;
	if (propName != "") {
	  value = current_properties.getValue(propName, true);
	}
	if (type == "x-shift") {
	  l->addTransformation(FGInstrumentLayer::XSHIFT,
			       value,
			       trans.getFloatValue("min", 0.0),
			       trans.getFloatValue("max", 1.0),
			       trans.getFloatValue("factor", 1.0),
			       trans.getFloatValue("offset", 0.0));
	} else if (type == "y-shift") {
	  l->addTransformation(FGInstrumentLayer::YSHIFT,
			       value,
			       trans.getFloatValue("min", 0.0),
			       trans.getFloatValue("max", 1.0),
			       trans.getFloatValue("factor", 1.0),
			       trans.getFloatValue("offset", 0.0));
	} else if (type == "rotation") {
	  l->addTransformation(FGInstrumentLayer::ROTATION,
			       value,
			       trans.getFloatValue("min", -360.0),
			       trans.getFloatValue("max", 360.0),
			       trans.getFloatValue("factor", 1.0),
			       trans.getFloatValue("offset", 0.0));
	} else if (type == "") {
	  FG_LOG(FG_INPUT, FG_ALERT,
		 "'type' must be specified for transformation");
	  return 0;
	} else {
	  FG_LOG(FG_INPUT, FG_ALERT, "Unrecognized transformation: " << type);
	  return 0;
	}
	FG_LOG(FG_INPUT, FG_INFO, "Read transformation " << name);
      }

      instrument->addLayer(l);
      FG_LOG(FG_INPUT, FG_INFO, "Read layer " << name);
    }

    panel->addInstrument(instrument);
    FG_LOG(FG_INPUT, FG_INFO, "Read instrument " << name);
  }


  //
  // Return the new panel.
  //
  return panel;
}
