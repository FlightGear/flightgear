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

#include <config.h>

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
#include <osg/TextureRectangle>

#include <osgText/Text>
#include <osgDB/ReadFile>

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/fg_os.hxx>
#include <Main/locale.hxx>
#include "splash.hxx"
#include "renderer.hxx"

#include <sstream>

const char* LICENSE_URL_TEXT = "Licensed under the GNU GPL. See http://www.flightgear.org for more information";

class SplashScreenUpdateCallback : public osg::NodeCallback {
public:
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        SplashScreen* screen = static_cast<SplashScreen*>(node);
        screen->doUpdate();
        traverse(node, nv);
    }
};

SplashScreen::SplashScreen() :
    _splashAlphaNode(fgGetNode("/sim/startup/splash-alpha", true))
{
    setName("splashGroup");
    setUpdateCallback(new SplashScreenUpdateCallback);
}

SplashScreen::~SplashScreen()
{
}

void SplashScreen::createNodes()
{
    _splashImage = osgDB::readImageFile(selectSplashImage());

    int width = _splashImage->s();
    int height = _splashImage->t();
    _splashImageAspectRatio = static_cast<double>(width) / height;

    osg::TextureRectangle* splashTexture = new osg::TextureRectangle(_splashImage);
    splashTexture->setTextureSize(width, height);
    splashTexture->setInternalFormat(GL_RGB);
    splashTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    splashTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    splashTexture->setImage(_splashImage);

    _splashFBOCamera = new osg::Camera;
    _splashFBOCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _splashFBOCamera->setViewMatrix(osg::Matrix::identity());
    _splashFBOCamera->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    _splashFBOCamera->setClearColor( osg::Vec4( 0., 0., 0., 0. ) );
    _splashFBOCamera->setAllowEventFocus(false);
    _splashFBOCamera->setCullingActive(false);

    _splashFBOTexture = new osg::Texture2D;
    _splashFBOTexture->setInternalFormat(GL_RGB);
    _splashFBOTexture->setResizeNonPowerOfTwoHint(false);
    _splashFBOTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _splashFBOTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    _splashFBOCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    _splashFBOCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    _splashFBOCamera->attach(osg::Camera::COLOR_BUFFER, _splashFBOTexture);

    addChild(_splashFBOCamera);
    
    osg::StateSet* stateSet = _splashFBOCamera->getOrCreateStateSet();
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setAttribute(new osg::Depth(osg::Depth::ALWAYS, 0, 1, false));
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setSupportsDisplayList(false);

    _splashImageVertexArray = new osg::Vec3Array;
    for (int i=0; i < 4; ++i) {
        _splashImageVertexArray->push_back(osg::Vec3(0.0, 0.0, 0.0));
    }
    geometry->setVertexArray(_splashImageVertexArray);

    osg::Vec2Array* imageTCs = new osg::Vec2Array;
    imageTCs->push_back(osg::Vec2(0, 0));
    imageTCs->push_back(osg::Vec2(width, 0));
    imageTCs->push_back(osg::Vec2(width, height));
    imageTCs->push_back(osg::Vec2(0, height));
    geometry->setTexCoordArray(0, imageTCs);

    osg::Vec4Array* colorArray = new osg::Vec4Array;
    colorArray->push_back(osg::Vec4(1, 1, 1, 1));
    geometry->setColorArray(colorArray);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, 0, 4));

    stateSet = geometry->getOrCreateStateSet();
    stateSet->setTextureMode(0, GL_TEXTURE_RECTANGLE, osg::StateAttribute::ON);
    stateSet->setTextureAttribute(0, splashTexture);

    osg::Geode* geode = new osg::Geode;
    _splashFBOCamera->addChild(geode);
    geode->addDrawable(geometry);

    if (_legacySplashScreenMode) {
        addText(geode, osg::Vec2(0.025, 0.025), 0.03,
                std::string("FlightGear ") + fgGetString("/sim/version/flightgear") +
                std::string(" ") + std::string(LICENSE_URL_TEXT),
                osgText::Text::LEFT_TOP,
                nullptr,
                0.9);
    } else {
        setupLogoImage();

        addText(geode, osg::Vec2(0.025, 0.025), 0.10, std::string("FlightGear ") + fgGetString("/sim/version/flightgear"), osgText::Text::LEFT_TOP);
        addText(geode, osg::Vec2(0.025, 0.15), 0.03, LICENSE_URL_TEXT,
                osgText::Text::LEFT_TOP,
                nullptr,
                0.6);

        if (!_aircraftLogoVertexArray) {
            addText(geode, osg::Vec2(0.025, 0.935), 0.10,
                    fgGetString("/sim/description"),
                    osgText::Text::LEFT_BOTTOM,
                    nullptr,
                    0.6);
            _items.back().maxLineCount = 1;
        }

        addText(geode, osg::Vec2(0.025, 0.975), 0.03,
                fgGetString("/sim/author"),
                osgText::Text::LEFT_BOTTOM,
                nullptr,
                0.6);
        _items.back().maxLineCount = 1;
    }

    addText(geode, osg::Vec2(0.975, 0.935), 0.03,
            "loading status",
            osgText::Text::RIGHT_BOTTOM,
            fgGetNode("/sim/startup/splash-progress-text", true),
            0.4);

    addText(geode, osg::Vec2(0.975, 0.975), 0.03,
            "spinner",
            osgText::Text::RIGHT_BOTTOM,
            fgGetNode("/sim/startup/splash-progress-spinner", true));

    ///////////

    geometry = new osg::Geometry;
    geometry->setSupportsDisplayList(false);

    _splashSpinnerVertexArray = new osg::Vec3Array;
    for (int i=0; i < 8; ++i) {
        _splashSpinnerVertexArray->push_back(osg::Vec3(0.0, 0.0, -0.1));
    }
    geometry->setVertexArray(_splashSpinnerVertexArray);

    // QColor buttonColor(27, 122, 211);
    colorArray = new osg::Vec4Array;
    colorArray->push_back(osg::Vec4(27 / 255.0, 122 / 255.0, 211 / 255.0, 0.75));
    geometry->setColorArray(colorArray);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 8));

    geode->addDrawable(geometry);

    //// Full screen quad setup ////////////////////

    _splashQuadCamera = new osg::Camera;
    _splashQuadCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _splashQuadCamera->setViewMatrix(osg::Matrix::identity());
    _splashQuadCamera->setProjectionMatrixAsOrtho2D(0.0, 1.0, 0.0, 1.0);
    _splashQuadCamera->setClearMask( 0 );
    _splashQuadCamera->setAllowEventFocus(false);
    _splashQuadCamera->setCullingActive(false);
    _splashQuadCamera->setRenderOrder(osg::Camera::POST_RENDER, 20000);

    stateSet = _splashQuadCamera->getOrCreateStateSet();
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setAttribute(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setRenderBinDetails(1, "RenderBin");

    geometry = new osg::Geometry;
    geometry->setSupportsDisplayList(false);

    osg::Vec3Array* splashFullscreenQuadVertices = new osg::Vec3Array;
    splashFullscreenQuadVertices->push_back(osg::Vec3(0.0, 0.0, 0.0));
    splashFullscreenQuadVertices->push_back(osg::Vec3(1.0, 0.0, 0.0));
    splashFullscreenQuadVertices->push_back(osg::Vec3(1.0, 1.0, 0.0));
    splashFullscreenQuadVertices->push_back(osg::Vec3(0.0, 1.0, 0.0));
    geometry->setVertexArray(splashFullscreenQuadVertices);

    osg::Vec2Array* quadTexCoords = new osg::Vec2Array;
    quadTexCoords->push_back(osg::Vec2(0, 0));
    quadTexCoords->push_back(osg::Vec2(1.0f, 0));
    quadTexCoords->push_back(osg::Vec2(1.0f, 1.0f));
    quadTexCoords->push_back(osg::Vec2(0, 1.0f));
    geometry->setTexCoordArray(0, quadTexCoords);

    _splashFSQuadColor = new osg::Vec4Array;
    _splashFSQuadColor->push_back(osg::Vec4(1, 1.0f, 1, 1));
    geometry->setColorArray(_splashFSQuadColor);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, 0, 4));

    stateSet = geometry->getOrCreateStateSet();
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
    stateSet->setTextureAttribute(0, _splashFBOTexture);

    geode = new osg::Geode;
    geode->addDrawable(geometry);

    _splashQuadCamera->addChild(geode);
    addChild(_splashQuadCamera);
}

void SplashScreen::setupLogoImage()
{
    // check for a logo image, add underneath other text
    SGPath logoPath = globals->resolve_maybe_aircraft_path(fgGetString("/sim/startup/splash-logo-image"));
    if (!logoPath.exists()) {
        return;
    }

    _logoImage = osgDB::readImageFile(logoPath.utf8Str());
    if (!_logoImage) {
        return;
    }

    osg::Texture2D* logoTexture = new osg::Texture2D(_logoImage);
    logoTexture->setResizeNonPowerOfTwoHint(false);
    logoTexture->setInternalFormat(GL_RGBA);
    logoTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    logoTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setSupportsDisplayList(false);

    _aircraftLogoVertexArray = new osg::Vec3Array;
    for (int i=0; i < 4; ++i) {
        _aircraftLogoVertexArray->push_back(osg::Vec3(0.0, 0.0, 0.0));
    }
    geometry->setVertexArray(_aircraftLogoVertexArray);

    osg::Vec2Array* logoTCs = new osg::Vec2Array;
    logoTCs->push_back(osg::Vec2(0, 0));
    logoTCs->push_back(osg::Vec2(1.0, 0));
    logoTCs->push_back(osg::Vec2(1.0, 1.0));
    logoTCs->push_back(osg::Vec2(0, 1.0));
    geometry->setTexCoordArray(0, logoTCs);

    osg::Vec4Array* colorArray = new osg::Vec4Array;
    colorArray->push_back(osg::Vec4(1, 1, 1, 1));
    geometry->setColorArray(colorArray);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, 0, 4));

    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
    stateSet->setTextureAttribute(0, logoTexture);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);

    osg::Geode* geode = new osg::Geode;
    _splashFBOCamera->addChild(geode);
    geode->addDrawable(geometry);

}

void SplashScreen::addText(osg::Geode* geode ,
                           const osg::Vec2& pos, double size, const std::string& text,
                           const osgText::Text::AlignmentType alignment,
                           SGPropertyNode* dynamicValue,
                           double maxWidthFraction)
{
    SGPath path = globals->resolve_resource_path("Fonts/LiberationFonts/LiberationSans-BoldItalic.ttf");

    TextItem item;
    osg::ref_ptr<osgText::Text> t = new osgText::Text;
    item.textNode = t;
    t->setFont(path.utf8Str());
    t->setColor(osg::Vec4(1, 1, 1, 1));
    t->setFontResolution(64, 64);
    t->setText(text);
    t->setBackdropType(osgText::Text::OUTLINE);
    t->setBackdropColor(osg::Vec4(0.2, 0.2, 0.2, 1));
    t->setBackdropOffset(0.04);
    
    item.fractionalCharSize = size;
    item.fractionalPosition = pos;
    item.dynamicContent = dynamicValue;
    item.textNode->setAlignment(alignment);
    item.maxWidthFraction = maxWidthFraction;
    geode->addDrawable(item.textNode);

    _items.push_back(item);
}

void SplashScreen::TextItem::reposition(int width, int height) const
{
    const int halfWidth = width >> 1;
    const int halfHeight = height >> 1;
    osg::Vec3 pixelPos(fractionalPosition.x() * width - halfWidth,
                       (1.0 - fractionalPosition.y()) * height - halfHeight,
                       -0.1);
    textNode->setPosition(pixelPos);
    textNode->setCharacterSize(fractionalCharSize * height);

    if (maxWidthFraction > 0.0) {
        textNode->setMaximumWidth(maxWidthFraction * width);
    }

    recomputeSize(height);
}

void SplashScreen::TextItem::recomputeSize(int height) const
{
    if (maxLineCount == 0) {
        return;
    }

    double baseSize = fractionalCharSize;
    textNode->update();
    while (textNode->getLineCount() > maxLineCount) {
        baseSize *= 0.8; // 20% shrink each time
        textNode->setCharacterSize(baseSize * height);
        textNode->update();
    }
}

std::string SplashScreen::selectSplashImage()
{
    sg_srandom_time(); // init random seed

    simgear::PropertyList previewNodes = fgGetNode("/sim/previews", true)->getChildren("preview");
    std::vector<SGPath> paths;

    for (auto n : previewNodes) {
        if (!n->getBoolValue("splash", false)) {
            continue;
        }

        SGPath tpath = globals->resolve_maybe_aircraft_path(n->getStringValue("path"));
        if (tpath.exists()) {
            paths.push_back(tpath);
        }
    }

    if (paths.empty()) {
        // look for a legacy aircraft splash
        simgear::PropertyList nodes = fgGetNode("/sim/startup", true)->getChildren("splash-texture");
        for (auto n : nodes) {
            SGPath tpath = globals->resolve_maybe_aircraft_path(n->getStringValue());
            if (tpath.exists()) {
                paths.push_back(tpath);
                _legacySplashScreenMode = true;
            }
        }
    }

    if (!paths.empty()) {
        // Select a random useable texture
        const int index = (int)(sg_random() * paths.size());
        return paths.at(index).utf8Str();
    }

    // no splash screen specified - select random image
    SGPath tpath = globals->get_fg_root() / "Textures";
    int num = (int)(sg_random() * 5.0 + 1.0);
    std::ostringstream oss;
    oss << "Splash" << num << ".png";
    tpath.append(oss.str());
    return tpath.utf8Str();
}

void SplashScreen::doUpdate()
{
    double alpha = _splashAlphaNode->getDoubleValue();

    if (alpha <= 0 || !fgGetBool("/sim/startup/splash-screen")) {
        removeChild(0, getNumChildren());
        _splashFBOCamera = nullptr;
        _splashQuadCamera = nullptr;
    } else if (getNumChildren() == 0) {
        createNodes();
        _splashStartTime.stamp();
        resize(fgGetInt("/sim/startup/xsize"),
               fgGetInt("/sim/startup/ysize"));
    } else {
        (*_splashFSQuadColor)[0] = osg::Vec4(1.0, 1.0, 1.0, _splashAlphaNode->getFloatValue());
        _splashFSQuadColor->dirty();

        for (const TextItem& item : _items) {
            if (item.dynamicContent) {
                item.textNode->setText(item.dynamicContent->getStringValue());
            }
        }

        updateSplashSpinner();
    }
}

float scaleAndOffset(float v, float halfWidth)
{
    return halfWidth * ((v * 2.0) - 1.0);
}

void SplashScreen::updateSplashSpinner()
{
    const int elapsedMsec = _splashStartTime.elapsedMSec();
    float splashSpinnerPos = (elapsedMsec % 2000) / 2000.0f;
    float endPos = splashSpinnerPos + 0.25f;
    float wrapStartPos = 0.0f;
    float wrapEndPos = 0.0f; // no wrapped quad
    if (endPos > 1.0f) {
        wrapEndPos = endPos - 1.0f;
    }

    const float halfWidth = _width * 0.5f;
    const float halfHeight = _height * 0.5f;
    const float bottomY = -halfHeight;
    const float topY = bottomY + 8;
    const float z = -0.05f;

    splashSpinnerPos = scaleAndOffset(splashSpinnerPos, halfWidth);
    endPos = scaleAndOffset(endPos, halfWidth);
    wrapStartPos = scaleAndOffset(wrapStartPos, halfWidth);
    wrapEndPos = scaleAndOffset(wrapEndPos, halfWidth);

    osg::Vec3 positions[8] = {
        osg::Vec3(splashSpinnerPos, bottomY, z),
        osg::Vec3(endPos, bottomY, z),
        osg::Vec3(endPos,topY, z),
        osg::Vec3(splashSpinnerPos, topY, z),
        osg::Vec3(wrapStartPos, bottomY, z),
        osg::Vec3(wrapEndPos, bottomY, z),
        osg::Vec3(wrapEndPos,topY, z),
        osg::Vec3(wrapStartPos, topY, z)

    };

    for (int i=0; i<8; ++i) {
        (*_splashSpinnerVertexArray)[i] = positions[i];
    }

    _splashSpinnerVertexArray->dirty();
}

void SplashScreen::resize( int width, int height )
{
    if (getNumChildren() == 0) {
        return;
    }

    _width = width;
    _height = height;
    _splashFBOCamera->setViewport(0, 0, width, height);
    _splashFBOCamera->setProjectionMatrixAsOrtho2D(-width * 0.5, width * 0.5,
                                                   -height * 0.5, height * 0.5);

    _splashQuadCamera->setViewport(0, 0, width, height);
    _splashFBOCamera->resizeAttachments(width, height);

    double halfWidth = width * 0.5;
    double halfHeight = height * 0.5;
    const double screenAspectRatio = static_cast<double>(width) / height;

    if (_legacySplashScreenMode) {
        halfWidth = width * 0.4;
        halfHeight = height * 0.4;

        if (screenAspectRatio > _splashImageAspectRatio) {
            // screen is wider than our image
            halfWidth = halfHeight;
        } else {
            // screen is taller than our image
            halfHeight = halfWidth;
        }
    } else {
        // adjust vertex positions; image covers entire area
        if (screenAspectRatio > _splashImageAspectRatio) {
            // screen is wider than our image
            halfHeight = halfWidth / _splashImageAspectRatio;
        } else {
            // screen is taller than our image
            halfWidth = halfHeight * _splashImageAspectRatio;
        }
    }

    // adjust vertex positions and mark as dirty
    osg::Vec3 positions[4] = {
        osg::Vec3(-halfWidth, -halfHeight, 0.0),
        osg::Vec3(halfWidth, -halfHeight, 0.0),
        osg::Vec3(halfWidth, halfHeight, 0.0),
        osg::Vec3(-halfWidth, halfHeight, 0.0)
    };

    for (int i=0; i<4; ++i) {
        (*_splashImageVertexArray)[i] = positions[i];
    }

    _splashImageVertexArray->dirty();

    if (_aircraftLogoVertexArray) {
        float logoWidth = fgGetDouble("/sim/startup/splash-logo-width", 0.6) * width;
        float logoHeight = _logoImage->t() * (logoWidth / _logoImage->s());
        float originX = width * -0.5;
        float originY = height * ((1.0 - 0.935) - 0.5);
        osg::Vec3 positions[4] = {
            osg::Vec3(originX, originY, 0.0),
            osg::Vec3(originX + logoWidth, originY, 0.0),
            osg::Vec3(originX + logoWidth, originY + logoHeight, 0.0),
            osg::Vec3(originX, originY + logoHeight, 0.0)
        };

        for (int i=0; i<4; ++i) {
            (*_aircraftLogoVertexArray)[i] = positions[i];
        }

        _aircraftLogoVertexArray->dirty();
    }

    for (const TextItem& item : _items) {
        item.reposition(width, height);
    }
}

void fgSplashProgress( const char *identifier, unsigned int percent )
{
  fgSetString("/sim/startup/splash-progress-spinner", "");

  std::string text;
  if (identifier[0] != 0)
  {
    std::string id = std::string("splash/") + identifier;
    text = globals->get_locale()->getLocalizedString(id.c_str(), "sys", "");

    if( text.empty() )
      text = "<incomplete language resource>: " + id;
  }

    if (!strcmp(identifier,"downloading-scenery")) {
        std::ostringstream oss;
        unsigned int kbytesPerSec = fgGetInt("/sim/terrasync/transfer-rate-bytes-sec") / 1024;
        unsigned int kbytesPending = fgGetInt("/sim/terrasync/pending-kbytes");
        if (kbytesPending > 0) {
            if (kbytesPending > 1024) {
                int mBytesPending = kbytesPending >> 10;
                oss << " " << mBytesPending << "Mb";
            } else {
                oss << " " << kbytesPending << "Kb";
            }
        }
        if (kbytesPerSec > 0) {
            if (kbytesPerSec > 100) {
                double mbytesPerSec = kbytesPerSec / 1024.0;
                oss << " - " << std::fixed << std::setprecision(1) << mbytesPerSec << "Mb/sec";
            } else {
                oss << " - " << kbytesPerSec << " Kb/sec";
            }
        }
        fgSetString("/sim/startup/splash-progress-spinner", oss.str());
    }

    // over-write the spinner
    if (!strncmp(identifier, "navdata-", 8)) {
        std::ostringstream oss;
        oss << percent << "% complete";
        fgSetString("/sim/startup/splash-progress-spinner", oss.str());
    }

    if( fgGetString("/sim/startup/splash-progress-text") == text )
      return;

    SG_LOG( SG_VIEW, SG_INFO, "Splash screen progress " << identifier );
    fgSetString("/sim/startup/splash-progress-text", text);
}
