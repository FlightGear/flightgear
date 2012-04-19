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
    std::string fileName(name);
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
    findAttrOrHex(sourceTypes, prop->getChild("wrap-mode"), wrapMode);
    SGPropertyNode_ptr widthProp = prop->getChild("width");
    if (!widthProp.valid())
        width = -1;
    else if (widthProp->getStringValue() == std::string("screen"))
        width = -1;
    else
        width = widthProp->getIntValue();
    SGPropertyNode_ptr heightProp = prop->getChild("height");
    if (!heightProp.valid())
        height = -1;
    else if (heightProp->getStringValue() == std::string("screen"))
        height = -1;
    else
        height = heightProp->getIntValue();

    scaleFactor = prop->getFloatValue("scale-factor", 1.f);
    shadowComparison = prop->getBoolValue("shadow-comparison", false);
}

FGRenderingPipeline::Stage::Stage(SGPropertyNode* prop)
{
    SGPropertyNode_ptr nameProp = prop->getChild("name");
    if (!nameProp.valid()) {
        throw sg_exception("Stage name is mandatory");
    }
    name = nameProp->getStringValue();
    SGPropertyNode_ptr typeProp = prop->getChild("type");
    if (!typeProp.valid()) {
        throw sg_exception("Stage type is mandatory");
    }
    type = typeProp->getStringValue();
}

FGRenderingPipeline::FGRenderingPipeline()
{
}

flightgear::CameraInfo* FGRenderingPipeline::buildCamera(flightgear::CameraGroup* cgroup, unsigned flags, osg::Camera* camera,
                                    const osg::Matrix& view, const osg::Matrix& projection, osg::GraphicsContext* gc)
{
    flightgear::CameraInfo* info = new flightgear::CameraInfo(flags);
    buildBuffers( info );
    
    for (size_t i = 0; i < stages.size(); ++i) {
        osg::ref_ptr<Stage> stage = stages[i];
        buildStage(info, stage, cgroup, camera, view, projection, gc);
    }

    return 0;
}

osg::Texture2D* buildDeferredBuffer(GLint internalFormat, GLenum sourceFormat, GLenum sourceType, GLenum wrapMode, bool shadowComparison)
{
    osg::Texture2D* tex = new osg::Texture2D;
    tex->setResizeNonPowerOfTwoHint( false );
    tex->setInternalFormat( internalFormat );
    tex->setShadowComparison(shadowComparison);
    if (shadowComparison) {
        tex->setShadowTextureMode(osg::Texture2D::LUMINANCE);
        tex->setBorderColor(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
    }
    tex->setSourceFormat(sourceFormat);
    tex->setSourceType(sourceType);
    tex->setFilter( osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR );
    tex->setFilter( osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR );
    tex->setWrap( osg::Texture::WRAP_S, (osg::Texture::WrapMode)wrapMode );
    tex->setWrap( osg::Texture::WRAP_T, (osg::Texture::WrapMode)wrapMode );
	return tex;
}

void FGRenderingPipeline::buildBuffers( flightgear::CameraInfo* info )
{
    for (size_t i = 0; i < buffers.size(); ++i) {
        osg::ref_ptr<Buffer> buffer = buffers[i];
        info->addBuffer(buffer->name, buildDeferredBuffer( buffer->internalFormat,
                                                            buffer->sourceFormat,
                                                            buffer->sourceType,
                                                            buffer->wrapMode,
                                                            buffer->shadowComparison) );
    }
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

void FGRenderingPipeline::buildStage(flightgear::CameraInfo* info,
                                        Stage* stage,
                                        flightgear::CameraGroup* cgroup,
                                        osg::Camera* camera,
                                        const osg::Matrix& view,
                                        const osg::Matrix& projection,
                                        osg::GraphicsContext* gc)
{
    osg::ref_ptr<osg::Camera> stageCamera;
    if (stage->type == "main-camera")
        stageCamera = camera;
    else
        stageCamera = new osg::Camera;

    stageCamera->setName(stage->name);
    stageCamera->setGraphicsContext(gc);
    stageCamera->setCullCallback(new FGStageCameraCullCallback(stage, info));
    if (stage->type != "main-camera")
        stageCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
}

void FGRenderingPipeline::buildMainCamera(flightgear::CameraInfo* info,
                                        Stage* stage,
                                        flightgear::CameraGroup* cgroup,
                                        osg::Camera* camera,
                                        const osg::Matrix& view,
                                        const osg::Matrix& projection,
                                        osg::GraphicsContext* gc)
{
}
