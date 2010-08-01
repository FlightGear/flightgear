// Owner Drawn Gauge helper class
//
// Written by Harald JOHNSEN, started May 2005.
//
// Copyright (C) 2005  Harald JOHNSEN
//
// Ported to OSG by Tim Moore - Jun 2007
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
//

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/Geode>
#include <osg/NodeVisitor>
#include <osg/Matrix>
#include <osg/PolygonMode>
#include <osg/ShadeModel>
#include <osg/StateSet>
#include <osgDB/FileNameUtils>

#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/debug/logstream.hxx>

#include <Main/globals.hxx>
#include <Main/renderer.hxx>
#include <Scenery/scenery.hxx>
#include "od_gauge.hxx"

FGODGauge::FGODGauge() :
    rtAvailable( false )// ,
//     rt( 0 )
{
}

void FGODGauge::allocRT () {
    camera = new osg::Camera;
    // Only the far camera should trigger this texture to be rendered.
    camera->setNodeMask(simgear::BACKGROUND_BIT);
    camera->setProjectionMatrix(osg::Matrix::ortho2D(-textureWH/2.0, textureWH/2.0, -textureWH/2.0, textureWH/2.0));
    camera->setViewport(0, 0, textureWH, textureWH);
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setRenderOrder(osg::Camera::PRE_RENDER);
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f , 0.0f));
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT, osg::Camera::FRAME_BUFFER);
    osg::StateSet* stateSet = camera->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setAttributeAndModes(new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
            osg::PolygonMode::FILL),
            osg::StateAttribute::ON);
    stateSet->setAttributeAndModes(new osg::AlphaFunc(osg::AlphaFunc::GREATER,
            0.0f),
            osg::StateAttribute::ON);
    stateSet->setAttribute(new osg::ShadeModel(osg::ShadeModel::FLAT));
    stateSet->setAttributeAndModes(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA,
            osg::BlendFunc::ONE_MINUS_SRC_ALPHA),
            osg::StateAttribute::ON);
    if (!texture.valid()) {
        texture = new osg::Texture2D;
        texture->setTextureSize(textureWH, textureWH);
        texture->setInternalFormat(GL_RGBA);
        texture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
        texture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
    }
    camera->attach(osg::Camera::COLOR_BUFFER, texture.get());
    globals->get_renderer()->addCamera(camera.get(), false);
    rtAvailable = true;

    // GLint colorBits = 0;
//     glGetIntegerv( GL_BLUE_BITS, &colorBits );
//     rt = new RenderTexture();
//     if( colorBits < 8 )
//         rt->Reset("rgba=5,5,5,1 ctt");
//     else
//         rt->Reset("rgba ctt");

//     if( rt->Initialize(256, 256, true) ) {
//         SG_LOG(SG_ALL, SG_INFO, "FGODGauge:Initialize sucessfull");
//         if (rt->BeginCapture())
//         {
//             SG_LOG(SG_ALL, SG_INFO, "FGODGauge:BeginCapture sucessfull, RTT available");
//             rtAvailable = true;
//             glViewport(0, 0, textureWH, textureWH);
//             glMatrixMode(GL_PROJECTION);
//             glLoadIdentity();
//             gluOrtho2D( -256.0, 256.0, -256.0, 256.0 );
//             glMatrixMode(GL_MODELVIEW);
//             glLoadIdentity();
//             glDisable(GL_LIGHTING);
//             glEnable(GL_COLOR_MATERIAL);
//             glDisable(GL_CULL_FACE);
//             glDisable(GL_FOG);
//             glDisable(GL_DEPTH_TEST);
//             glClearColor(0.0, 0.0, 0.0, 0.0);
//             glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//             glBindTexture(GL_TEXTURE_2D, 0);
//             glEnable(GL_TEXTURE_2D);
//             glEnable(GL_ALPHA_TEST);
//             glAlphaFunc(GL_GREATER, 0.0f);
//             glDisable(GL_SMOOTH);
//             glEnable(GL_BLEND);
//             glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
//             rt->EndCapture();
//         } else
//             SG_LOG(SG_ALL, SG_WARN, "FGODGauge:BeginCapture failed, RTT not available, using backbuffer");
//     } else
//         SG_LOG(SG_ALL, SG_WARN, "FGODGauge:Initialize failed, RTT not available, using backbuffer");
}

FGODGauge::~FGODGauge() {
//     delete rt;
}

void FGODGauge::init () {
}

void FGODGauge::update (double dt) {
}


void FGODGauge::setSize(int viewSize) {
    textureWH = viewSize;
//     glViewport(0, 0, textureWH, textureWH);
}

bool FGODGauge::serviceable(void) {
    return rtAvailable;
}

/**
 * Replace a texture in the airplane model with the gauge texture.
 */

class ReplaceStaticTextureVisitor : public osg::NodeVisitor
{
public:
    ReplaceStaticTextureVisitor(const std::string& name,
        osg::Texture2D* _newTexture) :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        newTexture(_newTexture)
    {
        textureFileName = osgDB::getSimpleFileName(name);
    }

    virtual void apply(osg::Node& node)
    {
        osg::StateSet* ss = node.getStateSet();
        if (ss)
            changeStateSetTexture(ss);
        traverse(node);
    }

    virtual void apply(osg::Geode& node)
    {
        int numDrawables = node.getNumDrawables();
        for (int i = 0; i < numDrawables; i++) {
            osg::Drawable* drawable = node.getDrawable(i);
            osg::StateSet* ss = drawable->getStateSet();
            if (ss)
                changeStateSetTexture(ss);
        }
        traverse(node);
}
protected:
    void changeStateSetTexture(osg::StateSet *ss)
    {
        osg::Texture2D* tex
                = dynamic_cast<osg::Texture2D*>(ss->getTextureAttribute(0,
                osg::StateAttribute::TEXTURE));
        if (!tex || tex == newTexture || !tex->getImage())
            return;
        std::string fileName = tex->getImage()->getFileName();
        std::string simpleName = osgDB::getSimpleFileName(fileName);
        if (osgDB::equalCaseInsensitive(textureFileName, simpleName))
            ss->setTextureAttribute(0, newTexture);
    }
    std::string textureFileName;
    osg::Texture2D* newTexture;
};

void FGODGauge::set_texture(const char * name, osg::Texture2D* new_texture)
{
    osg::Group* root = globals->get_scenery()->get_aircraft_branch();
    ReplaceStaticTextureVisitor visitor(name, new_texture);
    root->accept(visitor);
}

