// splash.cxx -- draws the initial splash screen
//
// Written by Curtis Olson, started July 1998.  (With a little looking
// at Freidemann's panel code.) :-)
//
// Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/Depth>
#include <osg/Geometry>
#include <osg/Node>
#include <osg/NodeCallback>
#include <osg/NodeVisitor>
#include <osg/StateSet>
#include <osg/Switch>
#include <osg/Texture2D>
#include <osgUtil/CullVisitor>
#include <osgText/Text>
#include <osgDB/ReadFile>

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>

#include <plib/fnt.h>

#include "GUI/FGFontCache.hxx"
#include "GUI/FGColor.hxx"

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/fg_os.hxx>
#include "splash.hxx"
#include "renderer.hxx"

class FGSplashUpdateCallback : public osg::Drawable::UpdateCallback {
public:
  FGSplashUpdateCallback(osg::Vec4Array* colorArray, SGPropertyNode* prop) :
    _colorArray(colorArray),
    _colorProperty(prop),
    _alphaProperty(fgGetNode("/sim/startup/splash-alpha", true))
  { }
  virtual void update(osg::NodeVisitor*, osg::Drawable*)
  {
    FGColor c(0, 0, 0);
    if (_colorProperty) {
      c.merge(_colorProperty);
      (*_colorArray)[0][0] = c.red();
      (*_colorArray)[0][1] = c.green();
      (*_colorArray)[0][2] = c.blue();
    }
    (*_colorArray)[0][3] = _alphaProperty->getFloatValue();
    _colorArray->dirty();
  }
private:
  osg::ref_ptr<osg::Vec4Array> _colorArray;
  SGSharedPtr<const SGPropertyNode> _colorProperty;
  SGSharedPtr<const SGPropertyNode> _alphaProperty;
};

class FGSplashTextUpdateCallback : public osg::Drawable::UpdateCallback {
public:
  FGSplashTextUpdateCallback(const SGPropertyNode* prop) :
    _textProperty(prop),
    _alphaProperty(fgGetNode("/sim/startup/splash-alpha", true)),
    _styleProperty(fgGetNode("/sim/gui/style[0]", true))
  {}
  virtual void update(osg::NodeVisitor*, osg::Drawable* drawable)
  {
    assert(dynamic_cast<osgText::Text*>(drawable));
    osgText::Text* text = static_cast<osgText::Text*>(drawable);

    FGColor c(1.0, 0.9, 0.0);
    c.merge(_styleProperty->getNode("colors/splash-font"));
    float alpha = _alphaProperty->getFloatValue();
    text->setColor(osg::Vec4(c.red(), c.green(), c.blue(), alpha));

    const char* s = _textProperty->getStringValue();
    if (s && fgGetBool("/sim/startup/splash-progress", true))
      text->setText(s);
    else
      text->setText("");
  }
private:
  SGSharedPtr<const SGPropertyNode> _textProperty;
  SGSharedPtr<const SGPropertyNode> _alphaProperty;
  SGSharedPtr<const SGPropertyNode> _styleProperty;
};



class FGSplashContentProjectionCalback : public osg::NodeCallback {
public:
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  { 
    assert(dynamic_cast<osgUtil::CullVisitor*>(nv));
    osgUtil::CullVisitor* cullVisitor = static_cast<osgUtil::CullVisitor*>(nv);

    // adjust the projection matrix in a way that preserves the aspect ratio
    // of the content ...
    const osg::Viewport* viewport = cullVisitor->getViewport();
    float viewportAspect = float(viewport->height())/float(viewport->width());

    float height, width;
    if (viewportAspect < 1) {
      height = 1;
      width = 1/viewportAspect;
    } else {
      height = viewportAspect;
      width = 1;
    }

    osg::RefMatrix* matrix = new osg::RefMatrix;
    matrix->makeOrtho2D(-width, width, -height, height);

    // The trick is to have the projection matrix adapted independent
    // of the scenegraph but dependent on the viewport of this current
    // camera we cull for. Therefore we do not put that projection matrix into
    // an additional camera rather than from within that cull callback.
    cullVisitor->pushProjectionMatrix(matrix);
    traverse(node, nv);
    cullVisitor->popProjectionMatrix();
  }
};

char *genNameString()
{
    std::string website = "http://www.flightgear.org";
    std::string programName = "FlightGear";
    char *name = new char[26];
    name[20] = 114;
    name[8] = 119;
    name[5] = 47;
    name[12] = 108;
    name[2] = 116;
    name[1] = 116;
    name[16] = 116;
    name[13] = 105;
    name[17] = 103;
    name[19] = 97;
    name[25] = 0;
    name[0] = 104;
    name[24] = 103;
    name[21] = 46;
    name[15] = 104;
    name[3] = 112;
    name[22] = 111;
    name[18] = 101;
    name[7] = 119;
    name[14] = 103;
    name[23] = 114;
    name[4] = 58;
    name[11] = 102;
    name[9] = 119;
    name[10] = 46;
    name[6] = 47;
    return name;
}

static osg::Node* fgCreateSplashCamera()
{
  const char* splash_texture = fgGetString("/sim/startup/splash-texture");
  SGSharedPtr<SGPropertyNode> style = fgGetNode("/sim/gui/style[0]", true);

  char *namestring = genNameString();
  fgSetString("/sim/startup/program-name", namestring);
  delete[] namestring;

  SGPath tpath;
  if (splash_texture  && strcmp(splash_texture, "")) {
      tpath = globals->resolve_maybe_aircraft_path(splash_texture);
      if (tpath.isNull())
      {
          SG_LOG( SG_VIEW, SG_ALERT, "Cannot find splash screen file '" << splash_texture
                  << "'. Using default." );
      }
  }

  if (tpath.isNull()) {
    // no splash screen specified - select random image
    tpath = globals->get_fg_root();
    // load in the texture data
    int num = (int)(sg_random() * 5.0 + 1.0);
    char num_str[5];
    snprintf(num_str, 4, "%d", num);

    tpath.append( "Textures/Splash" );
    tpath.concat( num_str );
    tpath.concat( ".png" );
  }

  osg::Texture2D* splashTexture = new osg::Texture2D;
  splashTexture->setImage(osgDB::readImageFile(tpath.c_str()));

  osg::Camera* camera = new osg::Camera;
  camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
  camera->setProjectionMatrix(osg::Matrix::ortho2D(-1, 1, -1, 1));
  camera->setViewMatrix(osg::Matrix::identity());
  camera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
  camera->setClearMask(0);
  camera->setAllowEventFocus(false);
  camera->setCullingActive(false);

  osg::StateSet* stateSet = camera->getOrCreateStateSet();
  stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  stateSet->setAttribute(new osg::BlendFunc);
  stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
  stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
  stateSet->setAttribute(new osg::Depth(osg::Depth::ALWAYS, 0, 1, false));
  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);


  osg::Geometry* geometry = new osg::Geometry;
  geometry->setSupportsDisplayList(false);

  osg::Vec3Array* vertexArray = new osg::Vec3Array;
  vertexArray->push_back(osg::Vec3(-1, -1, 0));
  vertexArray->push_back(osg::Vec3( 1, -1, 0));
  vertexArray->push_back(osg::Vec3( 1,  1, 0));
  vertexArray->push_back(osg::Vec3(-1,  1, 0));
  geometry->setVertexArray(vertexArray);
  osg::Vec4Array* colorArray = new osg::Vec4Array;
  colorArray->push_back(osg::Vec4(0, 0, 0, 1));
  geometry->setColorArray(colorArray);
  geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  geometry->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, 0, 4));
  geometry->setUpdateCallback(new FGSplashUpdateCallback(colorArray,
                              style->getNode("colors/splash-screen")));

  osg::Geode* geode = new osg::Geode;
  geode->addDrawable(geometry);

  stateSet = geode->getOrCreateStateSet();
  stateSet->setRenderBinDetails(1, "RenderBin");
  camera->addChild(geode);


  // The group is needed because of osg is handling the cull callbacks in a
  // different way for groups than for a geode. It does not hurt here ...
  osg::Group* group = new osg::Group;
  group->setCullCallback(new FGSplashContentProjectionCalback);
  camera->addChild(group);

  geode = new osg::Geode;
  stateSet = geode->getOrCreateStateSet();
  stateSet->setRenderBinDetails(2, "RenderBin");
  group->addChild(geode);


  geometry = new osg::Geometry;
  geometry->setSupportsDisplayList(false);

  vertexArray = new osg::Vec3Array;
  vertexArray->push_back(osg::Vec3(-0.84, -0.84, 0));
  vertexArray->push_back(osg::Vec3( 0.84, -0.84, 0));
  vertexArray->push_back(osg::Vec3( 0.84,  0.84, 0));
  vertexArray->push_back(osg::Vec3(-0.84,  0.84, 0));
  geometry->setVertexArray(vertexArray);
  osg::Vec2Array* texCoordArray = new osg::Vec2Array;
  texCoordArray->push_back(osg::Vec2(0, 0));
  texCoordArray->push_back(osg::Vec2(1, 0));
  texCoordArray->push_back(osg::Vec2(1, 1));
  texCoordArray->push_back(osg::Vec2(0, 1));
  geometry->setTexCoordArray(0, texCoordArray);
  colorArray = new osg::Vec4Array;
  colorArray->push_back(osg::Vec4(1, 1, 1, 1));
  geometry->setColorArray(colorArray);
  geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  geometry->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, 0, 4));
  geometry->setUpdateCallback(new FGSplashUpdateCallback(colorArray, 0));
  stateSet = geometry->getOrCreateStateSet();
  stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
  stateSet->setTextureAttribute(0, splashTexture);
  geode->addDrawable(geometry);


  osgText::Text* text = new osgText::Text;
  std::string fn = style->getStringValue("fonts/splash", "");
  text->setFont(globals->get_fontcache()->getfntpath(fn.c_str()).str());
  text->setCharacterSize(0.06);
  text->setColor(osg::Vec4(1, 1, 1, 1));
  text->setPosition(osg::Vec3(0, -0.92, 0));
  text->setAlignment(osgText::Text::CENTER_CENTER);
  SGPropertyNode* prop = fgGetNode("/sim/startup/splash-progress-text", true);
  prop->setStringValue("initializing");
  text->setUpdateCallback(new FGSplashTextUpdateCallback(prop));
  geode->addDrawable(text);

  text = new osgText::Text;
  text->setFont(globals->get_fontcache()->getfntpath(fn.c_str()).str());
  text->setCharacterSize(0.08);
  text->setColor(osg::Vec4(1, 1, 1, 1));
  text->setPosition(osg::Vec3(0, 0.92, 0));
  text->setAlignment(osgText::Text::CENTER_CENTER);
  prop = fgGetNode("/sim/startup/program-name", "FlightGear");
  text->setUpdateCallback(new FGSplashTextUpdateCallback(prop));
  geode->addDrawable(text);


  text = new osgText::Text;
  text->setFont(globals->get_fontcache()->getfntpath(fn.c_str()).str());
  text->setCharacterSize(0.06);
  text->setColor(osg::Vec4(1, 1, 1, 1));
  text->setPosition(osg::Vec3(0, 0.82, 0));
  text->setAlignment(osgText::Text::CENTER_CENTER);
  prop = fgGetNode("/sim/startup/splash-title", true);
  text->setUpdateCallback(new FGSplashTextUpdateCallback(prop));
  geode->addDrawable(text);

  return camera;
}

// update callback for the switch node guarding that splash
class FGSplashGroupUpdateCallback : public osg::NodeCallback {
public:
  FGSplashGroupUpdateCallback() :
    _splashAlphaNode(fgGetNode("/sim/startup/splash-alpha", true))
  { }
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<osg::Group*>(node));
    osg::Group* group = static_cast<osg::Group*>(node);

    double alpha = _splashAlphaNode->getDoubleValue();
    if (alpha <= 0 || !fgGetBool("/sim/startup/splash-screen"))
      group->removeChild(0, group->getNumChildren());
    else if (group->getNumChildren() == 0)
      group->addChild(fgCreateSplashCamera());

    traverse(node, nv);
  }
private:
  SGSharedPtr<const SGPropertyNode> _splashAlphaNode;
};

osg::Node* fgCreateSplashNode() {
  osg::Group* group = new osg::Group;
  group->setUpdateCallback(new FGSplashGroupUpdateCallback);
  return group;
}

// Initialize the splash screen
void fgSplashInit () {
  SG_LOG( SG_VIEW, SG_INFO, "Initializing splash screen" );
  globals->get_renderer()->splashinit();
}

void fgSplashProgress ( const char *text ) {
  if (!strcmp(fgGetString("/sim/startup/splash-progress-text"), text)) {
    return;
  }
  
  SG_LOG( SG_VIEW, SG_INFO, "Splash screen progress " << text );
  fgSetString("/sim/startup/splash-progress-text", text);
}
