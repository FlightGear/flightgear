// marker.cxx - 3D Marker pins in FlightGear
// Copyright (C) 2022 Tobias Dammers
// SPDX-License-Identifier: GPL-2.0-or-later OR MIT

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "Scenery/marker.hxx"
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/Node>
#include <osg/Billboard>
#include <osgText/Text>
#include <osgText/String>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>

osg::Node* fgCreateMarkerNode(const osgText::String& label, float font_size, float pin_height, float tip_height, const osg::Vec4f& color)
{
    auto mainNode = new osg::Group;

    auto textNode = new osg::Billboard;
    mainNode->addChild(textNode);

    textNode->setMode(osg::Billboard::AXIAL_ROT);

    auto text = new osgText::Text;
    text->setText(label);
    text->setAlignment(osgText::Text::CENTER_BOTTOM);
    text->setAxisAlignment(osgText::Text::XZ_PLANE);
    text->setFont("Fonts/LiberationFonts/LiberationSans-Regular.ttf");
    // text->setCharacterSizeMode(osgText::Text::OBJECT_COORDS_WITH_MAXIMUM_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT);
    text->setCharacterSize(font_size, 1.0f);
    text->setFontResolution(std::max(32.0f, font_size), std::max(32.0f, font_size));
    text->setColor(color);
    text->setPosition(osg::Vec3(0, 0, pin_height));
    text->setBackdropType(osgText::Text::OUTLINE);
    text->setBackdropColor(osg::Vec4(0, 0, 0, 0.75));
    textNode->addDrawable(text);

    float top_spacing = font_size * 0.25;

    if (pin_height - top_spacing > tip_height) {
        osg::Vec4f solid = color;
        osg::Vec4f transparent = color;
        osg::Vec3f nvec(0, 1, 0);

        solid[3] = 1.0f;
        transparent[3] = 0.0f;

        auto geoNode = new simgear::EffectGeode;
        mainNode->addChild(geoNode);
        auto pinGeo = new osg::Geometry;
        osg::Vec3Array* vtx = new osg::Vec3Array;
        osg::Vec3Array* nor = new osg::Vec3Array;
        osg::Vec4Array* rgb = new osg::Vec4Array;

        nor->push_back(nvec);

        vtx->push_back(osg::Vec3f(0, 0, tip_height));
        rgb->push_back(solid);

        vtx->push_back(osg::Vec3f(-font_size * 0.125, 0, pin_height - top_spacing));
        rgb->push_back(transparent);

        vtx->push_back(osg::Vec3f(0, 0, tip_height));
        rgb->push_back(solid);

        vtx->push_back(osg::Vec3f(0, font_size * 0.125, pin_height - top_spacing));
        rgb->push_back(transparent);

        vtx->push_back(osg::Vec3f(0, 0, tip_height));
        rgb->push_back(solid);

        vtx->push_back(osg::Vec3f(font_size * 0.125, 0, pin_height - top_spacing));
        rgb->push_back(transparent);

        vtx->push_back(osg::Vec3f(0, 0, tip_height));
        rgb->push_back(solid);

        vtx->push_back(osg::Vec3f(0, -font_size * 0.125, pin_height - top_spacing));
        rgb->push_back(transparent);

        vtx->push_back(osg::Vec3f(0, 0, tip_height));
        rgb->push_back(solid);

        vtx->push_back(osg::Vec3f(-font_size * 0.125, 0, pin_height - top_spacing));
        rgb->push_back(transparent);

        pinGeo->setVertexArray(vtx);
        pinGeo->setColorArray(rgb, osg::Array::BIND_PER_VERTEX);
        pinGeo->setNormalArray(nor, osg::Array::BIND_OVERALL);
        pinGeo->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP, 0, vtx->size()));
        geoNode->addDrawable(pinGeo);

        osg::ref_ptr<simgear::SGReaderWriterOptions> opt;
        opt = simgear::SGReaderWriterOptions::copyOrCreate(osgDB::Registry::instance()->getOptions());
        simgear::Effect* effect = simgear::makeEffect("Effects/marker-pin", true, opt);
        if (effect)
            geoNode->setEffect(effect);
    }

    mainNode->setNodeMask(~simgear::CASTSHADOW_BIT);
    return mainNode;
}
