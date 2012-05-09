// renderingpipeline.cxx -- description of the cameras needed by the Rembrandt renderer
//
// Written by Curtis Olson, started May 1997.
// This file contains parts of main.cxx prior to october 2004
//
// Copyright (C) 1997 - 2012  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <boost/lexical_cast.hpp>

#include <osg/GL>
#include <osg/FrameBufferObject> // For texture formats

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/scene/material/EffectBuilder.hxx>
#include <simgear/scene/material/EffectCullVisitor.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/math/SGMath.hxx>

#include "renderingpipeline.hxx"
#include "CameraGroup.hxx"
#include <Main/globals.hxx>
#include "renderer.hxx"

namespace flightgear {

FGRenderingPipeline* makeRenderingPipeline(const std::string& name,
                   const simgear::SGReaderWriterOptions* options)
{
    std::string fileName = "Effects/" + name;
    fileName += ".xml";
    std::string absFileName
        = simgear::SGModelLib::findDataFile(fileName, options);
    if (absFileName.empty()) {
        SG_LOG(SG_INPUT, SG_ALERT, "can't find \"" << fileName << "\"");
        return 0;
    }
    SGPropertyNode_ptr effectProps = new SGPropertyNode();
    try {
        readProperties(absFileName, effectProps.ptr(), 0, true);
    }
    catch (sg_io_exception& e) {
        SG_LOG(SG_INPUT, SG_ALERT, "error reading \"" << absFileName << "\": "
               << e.getFormattedMessage());
        return 0;
    }

    osg::ref_ptr<FGRenderingPipeline> pipeline = new FGRenderingPipeline;
    std::vector<SGPropertyNode_ptr> buffers = effectProps->getChildren("buffer");
    for (int i = 0; i < (int)buffers.size(); ++i) {
        pipeline->buffers.push_back(new FGRenderingPipeline::Buffer(buffers[i]));
    }

    std::vector<SGPropertyNode_ptr> stages = effectProps->getChildren("stage");
    for (int i = 0; i < (int)stages.size(); ++i) {
        pipeline->stages.push_back(new FGRenderingPipeline::Stage(stages[i]));
    }

    return pipeline.release();
}

}

template<typename T>
void findAttrOrHex(const simgear::effect::EffectPropertyMap<T>& pMap,
              const SGPropertyNode* prop,
              T& result)
{
    try {
        simgear::findAttr(pMap, prop, result);
    } catch (simgear::effect::BuilderException&) {
        std::string val = prop->getStringValue();
        try {
            result = boost::lexical_cast<T>(val);
        } catch (boost::bad_lexical_cast &) {
            throw simgear::effect::BuilderException(string("findAttrOrHex: could not find attribute ")
                                   + string(val));
        }
    }
}

const SGPropertyNode* getPropertyNode(const SGPropertyNode* prop)
{
    if (!prop)
        return 0;
    if (prop->nChildren() > 0) {
        const SGPropertyNode* propertyProp = prop->getChild("property");
        if (!propertyProp)
            return prop;
        return globals->get_props()->getNode(propertyProp->getStringValue());
    }
    return prop;
}

const SGPropertyNode* getPropertyChild(const SGPropertyNode* prop,
                                             const char* name)
{
    const SGPropertyNode* child = prop->getChild(name);
    if (!child)
        return 0;
    else
        return getPropertyNode(child);
}

simgear::effect::EffectNameValue<GLint> internalFormatInit[] =
{
    { "rgb8", GL_RGB8 },
    { "rgba8", GL_RGBA8 },
    { "rgb16", GL_RGB16 },
    { "rgba16", GL_RGBA16 },
    { "rg16", 0x822C },
    { "depth-component24", GL_DEPTH_COMPONENT24 },
    { "depth-component32", GL_DEPTH_COMPONENT32 }
};
simgear::effect::EffectPropertyMap<GLint> internalFormats(internalFormatInit);

simgear::effect::EffectNameValue<GLenum> sourceFormatInit[] =
{
    { "rg", 0x8227 },
    { "rgb", GL_RGB },
    { "rgba", GL_RGBA },
    { "depth-component", GL_DEPTH_COMPONENT }
};
simgear::effect::EffectPropertyMap<GLenum> sourceFormats(sourceFormatInit);

simgear::effect::EffectNameValue<GLenum> sourceTypeInit[] =
{
    { "unsigned-byte", GL_UNSIGNED_BYTE },
    { "unsigned-short", GL_UNSIGNED_SHORT },
    { "unsigned-int", GL_UNSIGNED_INT },
    { "float", GL_FLOAT }
};
simgear::effect::EffectPropertyMap<GLenum> sourceTypes(sourceTypeInit);

simgear::effect::EffectNameValue<GLenum> wrapModesInit[] =
{
    {"clamp", GL_CLAMP},
    {"clamp-to-border", GL_CLAMP_TO_BORDER_ARB},
    {"clamp-to-edge", GL_CLAMP_TO_EDGE},
    {"mirror", GL_MIRRORED_REPEAT_IBM},
    {"repeat", GL_REPEAT}
};
simgear::effect::EffectPropertyMap<GLenum> wrapModes(wrapModesInit);

FGRenderingPipeline::Buffer::Buffer(SGPropertyNode* prop)
{
    SGPropertyNode_ptr nameProp = prop->getChild("name");
    if (!nameProp.valid()) {
        throw sg_exception("Buffer name is mandatory");
    }
    name = nameProp->getStringValue();
    findAttrOrHex(internalFormats, prop->getChild("internal-format"), internalFormat);
    findAttrOrHex(sourceFormats, prop->getChild("source-format"), sourceFormat);
    findAttrOrHex(sourceTypes, prop->getChild("source-type"), sourceType);
    findAttrOrHex(wrapModes, prop->getChild("wrap-mode"), wrapMode);
    SGConstPropertyNode_ptr widthProp = getPropertyChild(prop, "width");
    if (!widthProp.valid())
        width = -1;
    else if (widthProp->getStringValue() == std::string("screen"))
        width = -1;
    else {
        width = widthProp->getIntValue();
    }
    SGConstPropertyNode_ptr heightProp = getPropertyChild(prop, "height");
    if (!heightProp.valid())
        height = -1;
    else if (heightProp->getStringValue() == std::string("screen"))
        height = -1;
    else
        height = heightProp->getIntValue();

    scaleFactor = prop->getFloatValue("scale-factor", 1.f);
    shadowComparison = prop->getBoolValue("shadow-comparison", false);
}

simgear::effect::EffectNameValue<osg::Camera::BufferComponent> componentsInit[] =
{
    {"depth",   osg::Camera::DEPTH_BUFFER},
    {"stencil", osg::Camera::STENCIL_BUFFER},
    {"packed-depth-stencil",   osg::Camera::PACKED_DEPTH_STENCIL_BUFFER},
    {"color0",  osg::Camera::COLOR_BUFFER0},
    {"color1",  osg::Camera::COLOR_BUFFER1},
    {"color2",  osg::Camera::COLOR_BUFFER2},
    {"color3",  osg::Camera::COLOR_BUFFER3}
};
simgear::effect::EffectPropertyMap<osg::Camera::BufferComponent> components(componentsInit);

FGRenderingPipeline::Stage::Stage(SGPropertyNode* prop)
{
    SGPropertyNode_ptr nameProp = prop->getChild("name");
    if (!nameProp.valid()) {
        throw sg_exception("Stage name is mandatory");
    }
    name = nameProp->getStringValue();
    SGPropertyNode_ptr typeProp = prop->getChild("type");
    if (!typeProp.valid()) {
        type = nameProp->getStringValue();
    } else {
        type = typeProp->getStringValue();
    }

    orderNum = prop->getIntValue("order-num", -1);
    effect = prop->getStringValue("effect", "");
    needsDuDv = prop->getBoolValue("needs-du-dv", false);
    scaleFactor = prop->getFloatValue("scale-factor", 1.f);

    std::vector<SGPropertyNode_ptr> attachments = prop->getChildren("attachment");
    for (int i = 0; i < (int)attachments.size(); ++i) {
        this->attachments.push_back(new FGRenderingPipeline::Attachment(attachments[i]));
    }
}

FGRenderingPipeline::Attachment::Attachment(SGPropertyNode* prop)
{
    simgear::findAttr(components, prop->getChild("component"), component);
    SGPropertyNode_ptr bufferProp = prop->getChild("buffer");
    if (!bufferProp.valid()) {
        throw sg_exception("Attachment buffer is mandatory");
    }
    buffer = bufferProp->getStringValue();
}

FGRenderingPipeline::FGRenderingPipeline()
{
}

class FGStageCameraCullCallback : public osg::NodeCallback {
public:
    FGStageCameraCullCallback(FGRenderingPipeline::Stage* s, flightgear::CameraInfo* i) : stage(s), info(i) {}
    virtual void operator()( osg::Node *n, osg::NodeVisitor *nv) {
        simgear::EffectCullVisitor* cv = dynamic_cast<simgear::EffectCullVisitor*>(nv);
        osg::Camera* camera = static_cast<osg::Camera*>(n);

        cv->clearBufferList();
        for (flightgear::RenderBufferMap::iterator ii = info->buffers.begin(); ii != info->buffers.end(); ++ii) {
            cv->addBuffer(ii->first, ii->second.texture );
        }

		if ( !info->getRenderStageInfo(stage->name).fullscreen )
			info->setMatrices( camera );

        cv->traverse( *camera );
    }

private:
    osg::ref_ptr<FGRenderingPipeline::Stage> stage;
    flightgear::CameraInfo* info;
};
