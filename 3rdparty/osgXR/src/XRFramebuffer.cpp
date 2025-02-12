// SPDX-License-Identifier: LGPL-2.1-only
// Copyright (C) 2021 James Hogan <james@albanarts.com>

#include "XRFramebuffer.h"

#include <osg/FrameBufferObject>
#include <osg/Image>
#include <osg/State>
#include <osg/Version>

using namespace osgXR;

typedef osg::GLExtensions OSG_GLExtensions;

static const OSG_GLExtensions* getGLExtensions(const osg::State& state)
{
    return state.get<osg::GLExtensions>();
}

XRFramebuffer::XRFramebuffer(uint32_t width, uint32_t height,
                             GLuint texture, GLuint depthTexture) :
    _width(width),
    _height(height),
    _depthFormat(GL_DEPTH_COMPONENT16),
    _fbo(0),
    _texture(texture),
    _depthTexture(depthTexture),
    _generated(false),
    _boundTexture(false),
    _boundDepthTexture(false),
    _deleteDepthTexture(false)
{
}

XRFramebuffer::~XRFramebuffer()
{
}

bool XRFramebuffer::valid(osg::State &state) const
{
    if (!_fbo)
        return false;

    const OSG_GLExtensions *fbo_ext = getGLExtensions(state);
    GLenum complete = fbo_ext->glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
    switch (complete)
    {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
        return true;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        OSG_WARN << "osgXR: FBO Incomplete attachment" << std::endl;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        OSG_WARN << "osgXR: FBO Incomplete missing attachment" << std::endl;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        OSG_WARN << "osgXR: FBO Incomplete draw buffer" << std::endl;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        OSG_WARN << "osgXR: FBO Incomplete read buffer" << std::endl;
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
        OSG_WARN << "osgXR: FBO Incomplete unsupported" << std::endl;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:
        OSG_WARN << "osgXR: FBO Incomplete multisample" << std::endl;
        break;
    default:
        OSG_WARN << "osgXR: FBO Incomplete ??? (0x" << std::hex << complete << std::dec << ")" << std::endl;
        break;
    }
    return false;
}

void XRFramebuffer::bind(osg::State &state)
{
    const OSG_GLExtensions *fbo_ext = getGLExtensions(state);

    if (!_fbo && !_generated)
    {
        fbo_ext->glGenFramebuffers(1, &_fbo);
        _generated = true;
    }

    if (_fbo)
    {
        fbo_ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, _fbo);
        if (!_boundTexture && _texture)
        {
            fbo_ext->glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _texture, 0);
            _boundTexture = true;
        }
        if (!_boundDepthTexture)
        {
            if (!_depthTexture)
            {
                glGenTextures(1, &_depthTexture);
                glBindTexture(GL_TEXTURE_2D, _depthTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, _depthFormat, _width, _height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
                glBindTexture(GL_TEXTURE_2D, 0);

                _deleteDepthTexture = true;
            }

            fbo_ext->glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, _depthTexture, 0);
            _boundDepthTexture = true;

            valid(state);
        }
    }
}

void XRFramebuffer::unbind(osg::State &state)
{
    const OSG_GLExtensions *fbo_ext = getGLExtensions(state);

    if (_fbo && _generated)
        fbo_ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
}

void XRFramebuffer::releaseGLObjects(osg::State &state)
{
    // FIXME can we do it like RenderBuffer::releaseGLObjects?
    // FIXME better yet, switch to use FrameBufferObject, dynamically bound

    // GL context must be current
    if (_fbo)
    {
        /*
        unsigned int contextID = state->getContextID();
        osg::get<GLFrameBufferObjectManager>(contextID)->scheduleGLObjectForDeletion(_fbo);
        */
        const OSG_GLExtensions *fbo_ext = getGLExtensions(state);
        fbo_ext->glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }
    if (_deleteDepthTexture)
    {
        glDeleteTextures(1, &_depthTexture);
        _depthTexture = 0;
        _deleteDepthTexture = false;
    }
}
