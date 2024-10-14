/*
 * SPDX-FileName: splash.cxx
 * SPDX-FileComment: draws the initial splash screen. Written by Curtis Olson, started July 1998.
 * SPDX-FileCopyrightText: Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
#include <osg/Version>

#include <osgText/Text>
#include <osgText/String>
#include <osgDB/ReadFile>

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/OsgUtils.hxx>
#include <simgear/props/condition.hxx>

#include "VRManager.hxx"
#include "renderer.hxx"
#include "splash.hxx"
#include <GUI/gui.h>
#include <Main/fg_os.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/locale.hxx>
#include <Main/util.hxx>
#include <Viewer/CameraGroup.hxx>

#include <sstream>


static const char* LICENSE_URL_TEXT = "Licensed under the GNU GPL. See https://www.flightgear.org for more information";

using namespace std::string_literals;
using namespace simgear;

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
#ifdef ENABLE_OSGXR
    uint32_t splashW = 1920, splashH = 1080;
    float aspect = (float)splashW / splashH;
    _splashSwapchain = new osgXR::Swapchain(splashW, splashH);
    _splashSwapchain->setAlphaBits(8);
    _splashSwapchain->allowRGBEncoding(osgXR::Swapchain::Encoding::ENCODING_SRGB);
    _splashLayer = new osgXR::CompositionLayerQuad(flightgear::VRManager::instance());
    _splashLayer->setSubImage(_splashSwapchain);
    _splashLayer->setSize(osg::Vec2f(aspect, 1.0f));
    _splashLayer->setPosition(osg::Vec3f(0, 0, -2.0f));
    _splashLayer->setAlphaMode(osgXR::CompositionLayer::BLEND_ALPHA_UNPREMULT);
#endif

    const std::string vertex_source =
        "#version 330 core\n"
        "\n"
        "layout(location = 0) in vec4 pos;\n"
        "layout(location = 3) in vec4 multitexcoord0;\n"
        "\n"
        "out vec2 texcoord;\n"
        "\n"
        "uniform mat4 osg_ModelViewProjectionMatrix;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = osg_ModelViewProjectionMatrix * pos;\n"
        "    texcoord = multitexcoord0.st;\n"
        "}\n";
    osg::Shader* vertex_shader = new osg::Shader(osg::Shader::VERTEX, vertex_source);

    const std::string fragment_source =
        "#version 330 core\n"
        "\n"
        "layout(location = 0) out vec4 fragColor;\n"
        "\n"
        "in vec2 texcoord;\n"
        "\n"
        "uniform sampler2D tex;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    fragColor = texture(tex, texcoord);\n"
        "}\n";
    osg::Shader* fragment_shader = new osg::Shader(osg::Shader::FRAGMENT, fragment_source);

    _program = new osg::Program();
    _program->addShader(vertex_shader);
    _program->addShader(fragment_shader);

    setName("splashGroup");
    setUpdateCallback(new SplashScreenUpdateCallback);
}

SplashScreen::~SplashScreen()
{
}

void SplashScreen::createNodes()
{
    // The splash FBO is rendered as sRGB, but for VR it needs to be in an sRGB
    // pixel format for it to be handled as such by OpenXR.
    bool useSRGB = false;
    osg::Camera* guiCamera = flightgear::getGUICamera(flightgear::CameraGroup::getDefault());
#if !defined(SG_MAC)
    // SRGB does detect on macOS, but doesn't actually work, so we
    // disable the check there.
    if (guiCamera) {
        osg::GraphicsContext* gc = guiCamera->getGraphicsContext();
        osg::GLExtensions* glext = gc->getState()->get<osg::GLExtensions>();
        if (glext) {
            useSRGB = osg::isGLExtensionOrVersionSupported(glext->contextID, "GL_EXT_texture_sRGB", 2.1f) &&
                      osg::isGLExtensionOrVersionSupported(glext->contextID, "GL_EXT_framebuffer_sRGB", 3.0f);
        }
    }
#endif
    // setup the base geometry 
    _splashFBOTexture = new osg::Texture2D;
    _splashFBOTexture->setInternalFormat(useSRGB ? GL_SRGB8 : GL_RGB);

    _splashFBOTexture->setResizeNonPowerOfTwoHint(false);
    _splashFBOTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    _splashFBOTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    _splashFBOCamera = createFBOCamera();
    addChild(_splashFBOCamera);

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setSupportsDisplayList(false);

    osg::Geode* geode = new osg::Geode;
    _splashFBOCamera->addChild(geode);
    geode->addDrawable(geometry);

    // get localized GPL licence text to be displayed at splash screen startup
    std::string licenseUrlText = globals->get_locale()->getLocalizedString("license-url", "sys", LICENSE_URL_TEXT);
    // add the splash image
    std::string splashImageName = selectSplashImage();
    addImage(splashImageName, true, 0, 0, 1, 1, nullptr, true);

    // parse the content from the tree
    // there can be many <content> <model-content> and <image> nodes
    // <content> is reserved for use in defaults.xml and is the basic 
    // text; model-content and images are for use in the model
    // to present model related information.
    auto root = globals->get_props()->getNode("/sim/startup");
    bool legacySplashLogoMode = false;

    // firstly add all image nodes.
    std::vector<SGPropertyNode_ptr> images = root->getChildren("image");
    if (!images.empty()) {
        for (const auto& image : images) {
            addImage(image->getStringValue("path", ""),
                     false,
                     image->getDoubleValue("x", 0.025f),
                     image->getDoubleValue("y", 0.935f),
                     image->getDoubleValue("width", 0.1),
                     image->getDoubleValue("height", 0.1), 
                     image->getNode("condition"),
                     false);
        }
    } else {
        // if there are no image nodes then revert to the legacy (2020.3 or before) way of doing things
        auto splashLogoImage = fgGetString("/sim/startup/splash-logo-image");
        if (!splashLogoImage.empty())
        {
            float logoX = fgGetDouble("/sim/startup/splash-logo-x-norm", 0.0);
            float logoY = 1.0 - fgGetDouble("/sim/startup/splash-logo-y-norm", 0.065);

            float logoWidth = fgGetDouble("/sim/startup/splash-logo-width", 0.6);

            auto img = addImage(splashLogoImage, false, logoX, logoY, logoWidth, 0, nullptr, false);
            if (img != nullptr)
                legacySplashLogoMode = true;
        }
    }

    // put this information into the property tree so the defaults or model can pull it in.
    fgSetString("/sim/startup/splash-authors", flightgear::getAircraftAuthorsText());
    fgSetString("/sim/startup/description", fgGetString("/sim/description"));
    fgSetString("/sim/startup/title", "FlightGear "s + fgGetString("/sim/version/flightgear"));

    if (!strcmp(FG_BUILD_TYPE, "Nightly")) {
        fgSetString("sim/build-warning", globals->get_locale()->getLocalizedString("unstable-warning", "sys", "unstable!"));
        fgSetBool("sim/build-warning-active", true);
    }

    // the licence node serves a dual purpose; both the licence URL and then after a delay
    // (currently 5 seconds) it will display helpful information.
    fgSetString("/sim/startup/licence", licenseUrlText);
    fgSetString("/sim/startup/tip", licenseUrlText);

#ifndef NDEBUG
    fgSetBool("/sim/startup/build-type-debug", true);
#else
    fgSetBool("/sim/startup/build-type-debug", false);
#endif

    fgSetBool("/sim/startup/legacy-splash-screen", _legacySplashScreenMode);
    fgSetBool("/sim/startup/legacy-splash-logo", legacySplashLogoMode);

    // load all model content first.
    for (const auto& content : root->getChildren("model-content")) {
        CreateTextFromNode(content, geode, true);
    }

    // default content comes in second; and has the ability to be overriden by the model
    for (const auto& content : root->getChildren("content")) {
        if (content->getIndex()) { // Skip 0 element - reserved for future usage.
            // default content can be hidden by the model. By hidden it will never be
            // added (there is also the possibility to use a condition to dynamically hide content)
            if (!content->getBoolValue("hide"))
                CreateTextFromNode(content, geode, false);
        }
    }
    // add main title last so it is atop all.
    addText(geode, osg::Vec2(0.025f, 0.02f), 0.08, "FlightGear "s + fgGetString("/sim/version/flightgear"), osgText::Text::LEFT_TOP);

   ///////////

    geometry = new osg::Geometry;
    geometry->setSupportsDisplayList(false);

    _splashSpinnerVertexArray = new osg::Vec3Array;
    for (int i=0; i < 8; ++i) {
        _splashSpinnerVertexArray->push_back(osg::Vec3(0.0f, 0.0f, -0.1f));
    }
    geometry->setVertexArray(_splashSpinnerVertexArray);

    // QColor buttonColor(27, 122, 211);
    osg::Vec4Array* colorArray = new osg::Vec4Array;
    colorArray->push_back(osg::Vec4(27 / 255.0f, 122 / 255.0f, 211 / 255.0f, 0.75f));
    geometry->setColorArray(colorArray);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 8));

    geode->addDrawable(geometry);

    //// Full screen quad setup ////////////////////

    _splashQuadCamera = new osg::Camera;
    _splashQuadCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _splashQuadCamera->setViewMatrix(osg::Matrix::identity());
    _splashQuadCamera->setProjectionMatrixAsOrtho2D(0.0, 1.0, 0.0, 1.0);
    _splashQuadCamera->setAllowEventFocus(false);
    _splashQuadCamera->setCullingActive(false);
    _splashQuadCamera->setRenderOrder(osg::Camera::NESTED_RENDER);
    
    osg::StateSet* stateSet = _splashQuadCamera->getOrCreateStateSet();
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setAttribute(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setRenderBinDetails(1000, "RenderBin");
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

    bool use_vertex_attribute_aliasing = false;
    if (guiCamera) {
        auto gc = guiCamera->getGraphicsContext();
        use_vertex_attribute_aliasing = gc->getState()->getUseVertexAttributeAliasing();
    }
    if (use_vertex_attribute_aliasing) {
        stateSet->setAttribute(_program);
        stateSet->addUniform(new osg::Uniform("tex", 0));
    }

    if (useSRGB) {
        stateSet->setMode(GL_FRAMEBUFFER_SRGB, osg::StateAttribute::ON);
    }

    geometry = osg::createTexturedQuadGeometry(osg::Vec3(0.0, 0.0, 0.0),
                                               osg::Vec3(1.0, 0.0, 0.0),
                                               osg::Vec3(0.0, 1.0, 0.0));
    geometry->setSupportsDisplayList(false);

    _splashFSQuadColor = new osg::Vec4Array;
    _splashFSQuadColor->push_back(osg::Vec4(1, 1.0f, 1, 1));
    geometry->setColorArray(_splashFSQuadColor);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    stateSet = geometry->getOrCreateStateSet();
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
    stateSet->setTextureAttribute(0, _splashFBOTexture);

    geode = new osg::Geode;
    geode->addDrawable(geometry);

#ifdef ENABLE_OSGXR
    _splashSwapchain->attachToMirror(stateSet);
#endif
    _splashQuadCamera->addChild(geode);
    addChild(_splashQuadCamera);
}

// creates a text element from a property node;
// <text> defines the text
// <text-prop> overrides the text with the contents of a property
// <dynamic-text> will link to a property and update the screen when the property changes
// <condition> Visibility Expression
// <color> <r><g><b> </color> define colour to be used
// <font> specifies the font alignment, size and typeface
// <max-width>  [optional] the max normalized width of the element
// <max-height> [optional] the max normalized height of the element. 
// <max-lines> [optional] the max number of lines this text can be wrapped over
//              wrapping takes place at max-width
// 
void SplashScreen::CreateTextFromNode(const SGPropertyNode_ptr& content, osg::Geode* geode, bool modelContent)
{
    auto text = content->getStringValue("text", "");
    std::string textFromProperty = content->getStringValue("text-prop", "");
    if (!textFromProperty.empty())
        text = fgGetString(textFromProperty);

    SGPropertyNode* dynamicValueNode = nullptr;
    std::string dynamicProperty = content->getStringValue("dynamic-text");
    if (!dynamicProperty.empty())
        dynamicValueNode = fgGetNode(dynamicProperty, true);
    
    auto conditionNode = content->getChild("condition");
    SGCondition* condition = nullptr;

    if (conditionNode != nullptr) {
        condition = sgReadCondition(fgGetNode("/"), conditionNode);
    }
    auto x = content->getDoubleValue("x", 0.5);
    auto y = content->getDoubleValue("y", 0.5);

    if (modelContent) {
        // the top 0.2 of the screen is for system usage
        if (y < 0.2) {
            SG_LOG(SG_VIEW, SG_ALERT, "model content cannot be above 0.2 y");
            y = 0.2;
        }
    }

    auto textItem = addText(geode, osg::Vec2(x, y),
        content->getDoubleValue("font/size", 0.06),
        text,
        osgutils::mapAlignment(content->getStringValue("font/alignment", "left-top")),
        dynamicValueNode,
        content->getDoubleValue("max-width", -1.0),
        osg::Vec4(content->getDoubleValue("color/r", 1), content->getDoubleValue("color/g", 1), content->getDoubleValue("color/b", 1), content->getDoubleValue("color/a", 1)),
        content->getStringValue("font/face", "Fonts/LiberationFonts/LiberationSans-BoldItalic.ttf"));

    textItem->condition = condition;

    if (textItem->condition != nullptr && !textItem->condition->test())
        textItem->textNode->setDrawMode(0);


    auto maxHeight = content->getDoubleValue("max-height", -1.0);
    auto maxLineCount = content->getIntValue("max-line-count", -1);

    if (maxLineCount > 0)
        textItem->maxLineCount = maxLineCount;
    if (maxHeight > 0)
        textItem->maxHeightFraction = maxHeight;
}

osg::ref_ptr<osg::Camera> SplashScreen::createFBOCamera()
{
    osg::ref_ptr<osg::Camera> c = new osg::Camera;
    c->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    c->setViewMatrix(osg::Matrix::identity());
    c->setClearMask( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    c->setClearColor( osg::Vec4( 0., 0., 0., 0. ) );
    c->setAllowEventFocus(false);
    c->setCullingActive(false);
    c->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    c->setRenderOrder(osg::Camera::PRE_RENDER);
    c->attach(osg::Camera::COLOR_BUFFER, _splashFBOTexture);
#ifdef ENABLE_OSGXR
    _splashSwapchain->attachToCamera(c);
#endif

    osg::StateSet* stateSet = c->getOrCreateStateSet();
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setAttribute(new osg::Depth(osg::Depth::ALWAYS, 0, 1, false));
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    bool use_vertex_attribute_aliasing = false;
    osg::Camera* guiCamera = flightgear::getGUICamera(flightgear::CameraGroup::getDefault());
    if (guiCamera) {
        auto gc = guiCamera->getGraphicsContext();
        use_vertex_attribute_aliasing = gc->getState()->getUseVertexAttributeAliasing();
    }
    if (use_vertex_attribute_aliasing) {
        stateSet->setAttribute(_program);
        stateSet->addUniform(new osg::Uniform("tex", 0));
    }

    return c;
}

// Load an image for display
// - path
// - x,y,width,height (normalized coordinates)
// - isBackground flag to indicate that this image is the background image. Only set during one call to this method when loading the splash image
//
const SplashScreen::ImageItem *SplashScreen::addImage(const std::string &path, bool isAbsolutePath, double x, double y, double width, double height, SGPropertyNode*
 conditionNode, bool isBackground)
{
    if (path.empty())
        return nullptr;

    SGPath imagePath;

    if (!isAbsolutePath)
        imagePath = globals->resolve_maybe_aircraft_path(path);
    else
        imagePath = path;

    if (!imagePath.exists() || !imagePath.isFile()) {
        SG_LOG(SG_VIEW, SG_INFO, "Splash Image " << path << " not be found");
        return nullptr;
    }

    ImageItem item;
    item.name = path;
    item.x = x;
    item.y = y;
    item.height = height;
    item.width = width;
    item.isBackground = isBackground;

    if (conditionNode != nullptr)
        item.condition = sgReadCondition(fgGetNode("/"), conditionNode);
    else
        item.condition = nullptr;

    osg::ref_ptr<simgear::SGReaderWriterOptions> staticOptions = simgear::SGReaderWriterOptions::copyOrCreate(osgDB::Registry::instance()->getOptions());
    staticOptions->setLoadOriginHint(simgear::SGReaderWriterOptions::LoadOriginHint::ORIGIN_SPLASH_SCREEN);

    item.Image = osgDB::readRefImageFile(imagePath.utf8Str(), staticOptions);
    if (!item.Image) {
        SG_LOG(SG_VIEW, SG_INFO, "Splash Image " << imagePath << " failed to load");
        return nullptr;
    }

    item.imageWidth = item.Image->s();
    item.imageHeight = item.Image->t();
    item.aspectRatio = static_cast<double>(item.imageWidth) / item.imageHeight;
    if (item.height == 0 && item.imageWidth != 0)
        item.height = item.imageHeight * (item.width / item.imageWidth);
    
    osg::Texture2D* imageTexture = new osg::Texture2D(item.Image);
    imageTexture->setResizeNonPowerOfTwoHint(false);
    imageTexture->setInternalFormat(GL_RGBA);
    imageTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    imageTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setSupportsDisplayList(false);

    item.vertexArray = new osg::Vec3Array;
    for (int i=0; i < 4; ++i) {
        item.vertexArray->push_back(osg::Vec3(0.0, 0.0, 0.0));
    }
    geometry->setVertexArray(item.vertexArray);

    osg::Vec2Array* imageTextureCoordinates = new osg::Vec2Array;
    imageTextureCoordinates->push_back(osg::Vec2(0, 0));
    imageTextureCoordinates->push_back(osg::Vec2(1.0, 0));
    imageTextureCoordinates->push_back(osg::Vec2(1.0, 1.0));
    imageTextureCoordinates->push_back(osg::Vec2(0, 1.0));
    geometry->setTexCoordArray(0, imageTextureCoordinates);

    osg::Vec4Array* imageColorArray = new osg::Vec4Array;
    imageColorArray->push_back(osg::Vec4(1, 1, 1, 1));
    geometry->setColorArray(imageColorArray);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, 0, 4));

    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
    stateSet->setTextureAttribute(0, imageTexture);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    
    osg::Geode* geode = new osg::Geode;
    _splashFBOCamera->addChild(geode);
    geode->addDrawable(geometry);

    item.geode = geode;
    item.nodeMask = geode->getNodeMask();

    if (item.condition != nullptr && !item.condition->test())
        item.geode->setNodeMask(0);

    _imageItems.push_back(item);
    return &_imageItems.back();
}

SplashScreen::TextItem *SplashScreen::addText(osg::Geode* geode ,
                           const osg::Vec2& pos, double size, const std::string& text,
                           const osgText::Text::AlignmentType alignment,
                           SGPropertyNode* dynamicValue,
                           double maxWidthFraction,
                           const osg::Vec4& textColor,
                           const std::string &fontFace )
{
    SGPath path = globals->resolve_maybe_aircraft_path(fontFace);

    TextItem item;
    osg::ref_ptr<osgText::Text> t = new osgText::Text;
    item.textNode = t;
    t->setFont(path.utf8Str());
    t->setColor(textColor);
    t->setFontResolution(64, 64);
    t->setText(text, osgText::String::Encoding::ENCODING_UTF8);
    t->setBackdropType(osgText::Text::OUTLINE);
    t->setBackdropColor(osg::Vec4(0.2, 0.2, 0.2, 1));
    t->setBackdropOffset(0.04);

    item.fractionalCharSize = size;
    item.fractionalPosition = pos;
    item.dynamicContent = dynamicValue;
    item.textNode->setAlignment(alignment);
    item.maxWidthFraction = maxWidthFraction;
    item.condition = nullptr; // default to always display.
    item.drawMode = t->getDrawMode();
    geode->addDrawable(item.textNode);

    _items.push_back(item);
    return &_items.back();
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
    if ((maxLineCount == 0) && (maxHeightFraction < 0.0)) {
        return;
    }

    double heightFraction = maxHeightFraction;
    if (heightFraction < 0.0) {
        heightFraction = 9999.0;
    }
    
    double baseSize = fractionalCharSize;
    textNode->update();
    while ((textNode->getLineCount() > maxLineCount) ||
           (baseSize * textNode->getLineCount() > heightFraction)) {
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

    // no splash screen specified - use one of the default ones
    SGPath tpath = globals->get_fg_root() / "Textures";
    paths = simgear::Dir(tpath).children(simgear::Dir::TYPE_FILE);
    paths.erase(std::remove_if(paths.begin(), paths.end(), [](const SGPath& p) {
        const auto f = p.file();
        if (f.find("Splash") != 0) return true;
        const auto ext = p.extension();
        return ext != "png" && ext != "jpg";
    }), paths.end());

    if (!paths.empty()) {
        // Select a random useable texture
        const int index = (int)(sg_random() * paths.size());
        return paths.at(index).utf8Str();
    }

    SG_LOG(SG_GUI, SG_ALERT, "Couldn't find any splash screens at all");
    return {};
}

void SplashScreen::doUpdate()
{
    if (!guiInit()) {
        // don't createNodes until OSG init operations have completed
        return;
    }

    double alpha = _splashAlphaNode->getDoubleValue();

    if (alpha <= 0 || !fgGetBool("/sim/startup/splash-screen")) {
        removeChild(0, getNumChildren());
        _splashFBOCamera = nullptr;
        _splashQuadCamera = nullptr;
#ifdef ENABLE_OSGXR
        _splashLayer->setVisible(false);
#endif
    } else if (getNumChildren() == 0) {
        createNodes();
        _splashStartTime.stamp();
        resize(fgGetInt("/sim/startup/xsize"),
               fgGetInt("/sim/startup/ysize"));
#ifdef ENABLE_OSGXR
        _splashLayer->setVisible(true);
        _splashSwapchain->setForcedAlpha(alpha);
#endif
    } else {
        (*_splashFSQuadColor)[0] = osg::Vec4(1.0, 1.0, 1.0, _splashAlphaNode->getFloatValue());
        _splashFSQuadColor->dirty();

        for (const TextItem& item : _items) {
            if (item.condition != nullptr) {

                if (item.condition->test())
                    item.textNode->setDrawMode(item.drawMode);
                else
                    item.textNode->setDrawMode(0);
            }
            if (item.dynamicContent) {
                item.textNode->setText(
                  item.dynamicContent->getStringValue(),
                  osgText::String::Encoding::ENCODING_UTF8);
            }
        }
        for (const ImageItem& image : _imageItems) {
            if (image.condition) {
                if (!image.condition->test())
                    image.geode->setNodeMask(0);
                else
                    image.geode->setNodeMask(image.nodeMask);
            }
        }
        updateSplashSpinner();
        updateTipText();
#ifdef ENABLE_OSGXR
        _splashSwapchain->setForcedAlpha(alpha);
#endif
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

void SplashScreen::updateTipText()
{
    // after 5 seconds change the tip; but only do this once.
    // the tip will be set into a property and this in turn will be 
    // displayed by the content using a dynamic-text element
    if (!_haveSetStartupTip && (_splashStartTime.elapsedMSec() > 5000)) {
        _haveSetStartupTip = true;
        FGLocale* locale = globals->get_locale();
        const int tipCount = locale->getLocalizedStringCount("tip", "tips");
        if (tipCount == 0) {
            return;
        }
        
        int tipIndex = globals->get_props()->getIntValue("/sim/session",0) % tipCount;

        std::string tipText = locale->getLocalizedStringWithIndex("tip", "tips", tipIndex);
        fgSetString("/sim/startup/tip", tipText);
    }
}

void SplashScreen::resize( int width, int height )
{
    if (getNumChildren() == 0) {
        return;
    }

    _width = width;
    _height = height;

    _splashQuadCamera->setViewport(0, 0, width, height);
    _splashFBOCamera->resizeAttachments(width, height);
    _splashFBOCamera->setViewport(0, 0, width, height);
    _splashFBOCamera->setProjectionMatrixAsOrtho2D(-width * 0.5, width * 0.5,
                                                   -height * 0.5, height * 0.5);
#ifdef ENABLE_OSGXR
    float aspect = (float)width / height;
    _splashSwapchain->setSize(width, height);
    _splashLayer->setSize(osg::Vec2f(aspect, 1.0f));
#endif

    const double screenAspectRatio = static_cast<double>(width) / height;
 
    // resize all of the images on the splash screen (including the background)
    for (const auto& _imageItem : _imageItems) {
     
         if (_imageItem.isBackground) {
             // background is based around the centre of the screen 
             // and adjusted so that the largest of width,height is used
             // to fill the screen so that the image fits without distortion 
             double halfWidth = width * 0.5;
             double halfHeight = height * 0.5;

             // if this is the background image and we are in legacy mode then
             // resize to keep the image scaled to fit in the centre
             if (_legacySplashScreenMode) {
                 halfWidth = width * 0.35;
                 halfHeight = height * 0.35;

                 if (screenAspectRatio > _imageItem.aspectRatio) {
                     // screen is wider than our image
                     halfWidth = halfHeight;
                 }
                 else {
                     // screen is taller than our image
                     halfHeight = halfWidth;
                 }
             }
             else {
                 // adjust vertex positions; image covers entire area
                 if (screenAspectRatio > _imageItem.aspectRatio) {
                     // screen is wider than our image
                     halfHeight = halfWidth / _imageItem.aspectRatio;
                 }
                 else {
                     // screen is taller than our image
                     halfWidth = halfHeight * _imageItem.aspectRatio;
                 }
             }
            (*_imageItem.vertexArray)[0] = osg::Vec3(-halfWidth, -halfHeight, 0.0);
            (*_imageItem.vertexArray)[1] = osg::Vec3(halfWidth, -halfHeight, 0.0);
            (*_imageItem.vertexArray)[2] = osg::Vec3(halfWidth, halfHeight, 0.0);
            (*_imageItem.vertexArray)[3] = osg::Vec3(-halfWidth, halfHeight, 0.0);
        }
        else {

             float imageWidth = _imageItem.width * width;
             float imageHeight = _imageItem.imageHeight * (imageWidth / _imageItem.imageWidth);

             float imageX = _imageItem.x * (width - imageWidth);
             float imageY = (1.0 - _imageItem.y) * (height - imageHeight);

             float originX = imageX - (width * 0.5);
             float originY = imageY - (height * 0.5);

            (*_imageItem.vertexArray)[0] = osg::Vec3(originX, originY, 0.0);
            (*_imageItem.vertexArray)[1] = osg::Vec3(originX + imageWidth, originY, 0.0);
            (*_imageItem.vertexArray)[2] = osg::Vec3(originX + imageWidth, originY + imageHeight, 0.0);
            (*_imageItem.vertexArray)[3] = osg::Vec3(originX, originY + imageHeight, 0.0);
        }
        _imageItem.vertexArray->dirty();
    }

    // adjust all text positions.
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
    text = globals->get_locale()->getLocalizedString(identifier, "sys");

    if( text.empty() )
      text = "<incomplete language resource>: "s + identifier;
  }

    if (!strcmp(identifier,"downloading-scenery")) {

        // get localized texts for units
        std::string kbytesUnitText = globals->get_locale()->getLocalizedString("units-kbytes", "sys", "KB");
        std::string mbytesUnitText = globals->get_locale()->getLocalizedString("units-mbytes", "sys", "MB");
        std::string kbytesPerSecUnitText = globals->get_locale()->getLocalizedString("units-kbytes-per-sec", "sys", "KB/s");
        std::string mbytesPerSecUnitText = globals->get_locale()->getLocalizedString("units-mbytes-per-sec", "sys", "MB/s");

        std::ostringstream oss;
        unsigned int kbytesPerSec = fgGetInt("/sim/terrasync/transfer-rate-bytes-sec") / 1024;
        unsigned int kbytesPending = fgGetInt("/sim/terrasync/pending-kbytes");
        unsigned int kbytesPendingExtract = fgGetInt("/sim/terrasync/extract-pending-kbytes");
        if (kbytesPending > 0) {
            if (kbytesPending > 1024) {
                int mBytesPending = kbytesPending >> 10;
                oss << " " << mBytesPending << " "s << mbytesUnitText;
            } else {
                oss << " " << kbytesPending << " "s << kbytesUnitText;
            }
        }

        if (kbytesPerSec > 100) {
            double mbytesPerSec = kbytesPerSec / 1024.0;
            oss << " - " << std::fixed << std::setprecision(1) << mbytesPerSec << " "s << mbytesPerSecUnitText;
        } else if (kbytesPerSec > 0) {
            oss << " - " << kbytesPerSec << " "s << kbytesPerSecUnitText;
        } else if (kbytesPendingExtract > 0) {
            const std::string extractText = globals->get_locale()->getLocalizedString("scenery-extract", "sys");
            std::ostringstream os2;

            if (kbytesPendingExtract > 1024) {
                int mBytesPendingExtract = kbytesPendingExtract >> 10;
                os2 << mBytesPendingExtract << " "s << mbytesUnitText;
            } else {
                os2 << kbytesPendingExtract << " "s << kbytesUnitText;
            }
            auto finalText = simgear::strutils::replace(extractText, "[VALUE]", os2.str());
            oss << " - " << finalText;
        }
        fgSetString("/sim/startup/splash-progress-spinner", oss.str());
    }

    if (!strcmp(identifier, "loading-scenery")) {

        // get localized texts for units
        std::string kbytesUnitText = globals->get_locale()->getLocalizedString("units-kbytes", "sys", "KB");
        std::string mbytesUnitText = globals->get_locale()->getLocalizedString("units-mbytes", "sys", "MB");

        unsigned int kbytesPendingExtract = fgGetInt("/sim/terrasync/extract-pending-kbytes");
        if (kbytesPendingExtract > 0) {
            const std::string extractText = globals->get_locale()->getLocalizedString("scenery-extract", "sys");
            std::ostringstream oss;
            if (kbytesPendingExtract > 1024) {
                int mBytesPendingExtract = kbytesPendingExtract >> 10;
                oss << mBytesPendingExtract << " "s << mbytesUnitText;
            } else {
                oss << kbytesPendingExtract << " "s << kbytesUnitText;
            }

            auto finalText = simgear::strutils::replace(extractText, "[VALUE]", oss.str());
            fgSetString("/sim/startup/splash-progress-spinner", finalText);
        } else {
            fgSetString("/sim/startup/splash-progress-spinner", "");
        }
    }

    // over-write the spinner
    if (!strncmp(identifier, "navdata-", 8)) {
        const std::string percentText = globals->get_locale()->getLocalizedString("navdata-load-percent", "sys");
        auto finalText = simgear::strutils::replace(percentText, "[VALUE]", std::to_string(percent));
        fgSetString("/sim/startup/splash-progress-spinner", finalText);
    }

    if( fgGetString("/sim/startup/splash-progress-text") == text )
      return;

    SG_LOG( SG_VIEW, SG_INFO, "Splash screen progress " << identifier );
    fgSetString("/sim/startup/splash-progress-text", text);
}
