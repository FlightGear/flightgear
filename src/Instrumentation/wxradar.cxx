// Wx Radar background texture
//
// Written by Harald JOHNSEN, started May 2005.
// With major amendments by Vivian MEAZZA May 2007
// Ported to OSG by Tim Moore Jun 2007
//
//
// Copyright (C) 2005  Harald JOHNSEN
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

#include <osg/Array>
#include <osg/Geometry>
#include <osg/Matrixf>
#include <osg/PrimitiveSet>
#include <osg/StateSet>
#include <osg/Version>
#include <osgDB/ReaderWriter>
#include <osgDB/WriteFile>

#include <simgear/constants.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <sstream>
#include <iomanip>

using std::stringstream;
using std::endl;
using std::setprecision;
using std::fixed;
using std::setw;
using std::setfill;
using std::string;

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Cockpit/panel.hxx>

#include "instrument_mgr.hxx"
#include "od_gauge.hxx"
#include "wxradar.hxx"

#include <iostream>             // for cout, endl

using std::cout;
using std::endl;

static const float UNIT = 1.0f / 8.0f;  // 8 symbols in a row/column in the texture
static const char *DEFAULT_FONT = "typewriter.txf";

wxRadarBg::wxRadarBg(SGPropertyNode *node) :
    _name(node->getStringValue("name", "radar")),
    _num(node->getIntValue("number", 0)),
    _time(0.0),
    _interval(node->getDoubleValue("update-interval-sec", 1.0)),
    _elapsed_time(0),
    _persistance(0),
    _odg(0),
    _range_nm(0),
    _scale(0),
    _angle_offset(0),
    _view_heading(0),
    _x_offset(0),
    _y_offset(0),
    _radar_ref_rng(0),
    _lat(0),
    _lon(0),
    _antenna_ht(node->getDoubleValue("antenna-ht-ft", 0.0)),
    _resultTexture(0),
    _wxEcho(0),
    _font_size(0),
    _font_spacing(0)
{
    string branch;
    branch = "/instrumentation/" + _name;
    _Instrument = fgGetNode(branch.c_str(), _num, true);

    const char *tacan_source = node->getStringValue("tacan-source", "/instrumentation/tacan");
    _Tacan = fgGetNode(tacan_source, true);

    _font_node = _Instrument->getNode("font", true);

#define INITFONT(p, val, type) if (!_font_node->hasValue(p)) _font_node->set##type##Value(p, val)
    INITFONT("name", DEFAULT_FONT, String);
    INITFONT("size", 8, Float);
    INITFONT("line-spacing", 0.25, Float);
    INITFONT("color/red", 0, Float);
    INITFONT("color/green", 0.8, Float);
    INITFONT("color/blue", 0, Float);
    INITFONT("color/alpha", 1, Float);
#undef INITFONT

    _font_node->addChangeListener(this, true);
}


wxRadarBg::~wxRadarBg ()
{
    _font_node->removeChangeListener(this);
    delete _odg;
}


void
wxRadarBg::init ()
{
    _serviceable_node = _Instrument->getNode("serviceable", true);
    _sceneryLoaded = fgGetNode("/sim/sceneryloaded", true);

    // texture name to use in 2D and 3D instruments
    _texture_path = _Instrument->getStringValue("radar-texture-path",
        "Aircraft/Instruments/Textures/od_wxradar.rgb");
    _resultTexture = FGTextureManager::createTexture(_texture_path.c_str(), false);

    string path = _Instrument->getStringValue("echo-texture-path",
        "Aircraft/Instruments/Textures/wxecho.rgb");
    SGPath tpath = globals->resolve_aircraft_path(path);

    // no mipmap or else alpha will mix with pixels on the border of shapes, ruining the effect
    _wxEcho = SGLoadTexture2D(tpath, NULL, false, false);


    _Instrument->setFloatValue("trk", 0.0);
    _Instrument->setFloatValue("tilt", 0.0);
    _Instrument->setStringValue("status", "");
    // those properties are used by a radar instrument of a MFD
    // input switch = OFF | TST | STBY | ON
    // input mode = WX | WXA | MAP
    // output status = STBY | TEST | WX | WXA | MAP | blank
    // input lightning = true | false
    // input TRK = +/- n degrees
    // input TILT = +/- n degree
    // input autotilt = true | false
    // input range = n nm (20/40/80)
    // input display-mode = arc | rose | map | plan

    _odg = new FGODGauge;
    _odg->setSize(512);

    _ai_enabled_node = fgGetNode("/sim/ai/enabled", true);

    _user_lat_node = fgGetNode("/position/latitude-deg", true);
    _user_lon_node = fgGetNode("/position/longitude-deg", true);
    _user_alt_node = fgGetNode("/position/altitude-ft", true);

    _user_speed_east_fps_node   = fgGetNode("/velocities/speed-east-fps", true);
    _user_speed_north_fps_node  = fgGetNode("/velocities/speed-north-fps", true);

    _tacan_serviceable_node = _Tacan->getNode("serviceable", true);
    _tacan_distance_node    = _Tacan->getNode("indicated-distance-nm", true);
    _tacan_name_node        = _Tacan->getNode("name", true);
    _tacan_bearing_node     = _Tacan->getNode("indicated-bearing-true-deg", true);
    _tacan_in_range_node    = _Tacan->getNode("in-range", true);

    _radar_mode_control_node = _Instrument->getNode("mode-control", true);
    _radar_coverage_node     = _Instrument->getNode("limit-deg", true);
    _radar_ref_rng_node      = _Instrument->getNode("reference-range-nm", true);
    _radar_hdg_marker_node   = _Instrument->getNode("heading-marker", true);

    SGPropertyNode *n = _Instrument->getNode("display-controls", true);
    _radar_weather_node     = n->getNode("WX", true);
    _radar_position_node    = n->getNode("pos", true);
    _radar_data_node        = n->getNode("data", true);
    _radar_symbol_node      = n->getNode("symbol", true);
    _radar_centre_node      = n->getNode("centre", true);
    _radar_rotate_node      = n->getNode("rotate", true);
    _radar_tcas_node        = n->getNode("tcas", true);
    _radar_absalt_node      = n->getNode("abs-altitude", true);

    _radar_centre_node->setBoolValue(false);
    if (!_radar_coverage_node->hasValue())
        _radar_coverage_node->setFloatValue(120);
    if (!_radar_ref_rng_node->hasValue())
        _radar_ref_rng_node->setDoubleValue(35);
    if (!_radar_hdg_marker_node->hasValue())
        _radar_hdg_marker_node->setBoolValue(true);

    _x_offset = 0;
    _y_offset = 0;

    // OSG geometry setup. The polygons for the radar returns will be
    // stored in a single Geometry. The geometry will have several
    // primitive sets so we can have different kinds of polys and
    // choose a different overall color for each set.
    _radarGeode = new osg::Geode;
    osg::StateSet *stateSet = _radarGeode->getOrCreateStateSet();
    stateSet->setTextureAttributeAndModes(0, _wxEcho.get());
    _geom = new osg::Geometry;
    _geom->setUseDisplayList(false);
    // Initially allocate space for 128 quads
    _vertices = new osg::Vec2Array;
    _vertices->setDataVariance(osg::Object::DYNAMIC);
    _vertices->reserve(128 * 4);
    _geom->setVertexArray(_vertices);
    _texCoords = new osg::Vec2Array;
    _texCoords->setDataVariance(osg::Object::DYNAMIC);
    _texCoords->reserve(128 * 4);
    _geom->setTexCoordArray(0, _texCoords);
    osg::Vec3Array *colors = new osg::Vec3Array;
    colors->push_back(osg::Vec3(1.0f, 1.0f, 1.0f)); // color of echos
    colors->push_back(osg::Vec3(1.0f, 0.0f, 0.0f)); // arc mask
    colors->push_back(osg::Vec3(0.0f, 0.0f, 0.0f)); // rest of mask
    _geom->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
    _geom->setColorArray(colors);
    osg::PrimitiveSet *pset = new osg::DrawArrays(osg::PrimitiveSet::QUADS);
    pset->setDataVariance(osg::Object::DYNAMIC);
    _geom->addPrimitiveSet(pset);
    pset = new osg::DrawArrays(osg::PrimitiveSet::QUADS);
    pset->setDataVariance(osg::Object::DYNAMIC);
    _geom->addPrimitiveSet(pset);
    pset = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES);
    pset->setDataVariance(osg::Object::DYNAMIC);
    _geom->addPrimitiveSet(pset);
    _geom->setInitialBound(osg::BoundingBox(osg::Vec3f(-256.0f, -256.0f, 0.0f),
        osg::Vec3f(256.0f, 256.0f, 0.0f)));
    _radarGeode->addDrawable(_geom);
    _odg->allocRT();
    // Texture in the 2D panel system
    FGTextureManager::addTexture(_texture_path.c_str(), _odg->getTexture());

    _textGeode = new osg::Geode;

    osg::Camera *camera = _odg->getCamera();
    camera->addChild(_radarGeode.get());
    camera->addChild(_textGeode.get());

    updateFont();
}


// Local coordinates for each echo
const osg::Vec3f echoCoords[4] = {
    osg::Vec3f(-.7f, -.7f, 0.0f), osg::Vec3f(.7f, -.7f, 0.0f),
    osg::Vec3f(.7f, .7f, 0.0f), osg::Vec3f(-.7f, .7f, 0.0f)
};


const osg::Vec2f echoTexCoords[4] = {
    osg::Vec2f(0.0f, 0.0f), osg::Vec2f(UNIT, 0.0f),
    osg::Vec2f(UNIT, UNIT), osg::Vec2f(0.0f, UNIT)
};


// helper
static void
addQuad(osg::Vec2Array *vertices, osg::Vec2Array *texCoords,
        const osg::Matrixf& transform, const osg::Vec2f& texBase)
{
    for (int i = 0; i < 4; i++) {
        const osg::Vec3f coords = transform.preMult(echoCoords[i]);
        texCoords->push_back(texBase + echoTexCoords[i]);
        vertices->push_back(osg::Vec2f(coords.x(), coords.y()));
    }
}


// Rotate by a heading value
static inline
osg::Matrixf wxRotate(float angle)
{
    return osg::Matrixf::rotate(angle, 0.0f, 0.0f, -1.0f);
}


void
wxRadarBg::update (double delta_time_sec)
{
    if (!_sceneryLoaded->getBoolValue())
        return;

    if (!_odg || !_serviceable_node->getBoolValue()) {
        _Instrument->setStringValue("status", "");
        return;
    }

    _time += delta_time_sec;
    if (_time < _interval)
        return;

    _time = 0.0;

    string mode = _Instrument->getStringValue("display-mode", "arc");
    if (mode == "map") {
        if (_display_mode != MAP) {
            _display_mode = MAP;
            center_map();
        }
    } else if (mode == "plan") {
        _display_mode = PLAN;}
    else if (mode == "bscan") {
        _display_mode = BSCAN;
    } else {
        _display_mode = ARC;
    }

    string switchKnob = _Instrument->getStringValue("switch", "on");
    if (switchKnob == "off") {
        _Instrument->setStringValue("status", "");
    } else if (switchKnob == "stby") {
        _Instrument->setStringValue("status", "STBY");
    } else if (switchKnob == "tst") {
        _Instrument->setStringValue("status", "TST");
        // find something interesting to do...
    } else {
        float r = _Instrument->getFloatValue("range", 40.0);
        if (r != _range_nm) {
            center_map();
            _range_nm = r;
        }

        _radar_ref_rng = _radar_ref_rng_node->getDoubleValue();
        _view_heading = fgGetDouble("/orientation/heading-deg") * SG_DEGREES_TO_RADIANS;
        _centerTrans.makeTranslate(0.0f, 0.0f, 0.0f);

        _scale = 200.0 / _range_nm;
        _angle_offset = 0;

        if (_display_mode == ARC) {
            _scale = 2*200.0f / _range_nm;
            _angle_offset = -_view_heading;
            _centerTrans.makeTranslate(0.0f, -200.0f, 0.0f);

        } else if (_display_mode == MAP) {
            apply_map_offset();

            bool centre = _radar_centre_node->getBoolValue();
            if (centre) {
                center_map();
                _radar_centre_node->setBoolValue(false);
            }

            //SG_LOG(SG_INSTR, SG_DEBUG, "Radar: displacement "
            //        << _x_offset <<", "<<_y_offset
            //        << " user_speed_east_fps * SG_FPS_TO_KT "
            //        << user_speed_east_fps * SG_FPS_TO_KT
            //        << " user_speed_north_fps * SG_FPS_TO_KT "
            //        << user_speed_north_fps * SG_FPS_TO_KT
            //        << " dt " << delta_time_sec);

            _centerTrans.makeTranslate(_x_offset, _y_offset, 0.0f);

        } else if (_display_mode == PLAN) {
            if (_radar_rotate_node->getBoolValue()) {
                _angle_offset = -_view_heading;
            }
        } else if (_display_mode == BSCAN) {
            _angle_offset = -_view_heading;
        } else {
            // rose
        }

        _vertices->clear();
        _texCoords->clear();
        _textGeode->removeDrawables(0, _textGeode->getNumDrawables());

#if 0
        //TODO FIXME Mask below (only used for ARC mode) isn't properly aligned, i.e.
        // it assumes the a/c position at the center of the display - though it's somewhere at
        // bottom part for ARC mode.
        // The mask hadn't worked at all for a while (probably since the OSG port) due to
        // another bug (which is fixed now). Now, the mask is disabled completely until s.o.
        // adapted the coordinates below. And the mask is only really useful to limit displayed
        // weather blobs (not support yet).
        // Aircraft echos are already limited properly through wxradar's "limit-deg" property.
        {
            osg::DrawArrays *maskPSet
                = static_cast<osg::DrawArrays*>(_geom->getPrimitiveSet(1));
            osg::DrawArrays *trimaskPSet
                = static_cast<osg::DrawArrays*>(_geom->getPrimitiveSet(2));

            if (_display_mode == ARC) {
                // erase what is out of sight of antenna
                /*
                |\     /|
                | \   / |
                |  \ /  |
                ---------
                |       |
                |       |
                ---------
                */
                float xOffset = 256.0f;
                float yOffset = 200.0f;

                int firstQuadVert = _vertices->size();
                _texCoords->push_back(osg::Vec2f(0.5f, 0.25f));
                _vertices->push_back(osg::Vec2f(-xOffset, 0.0 + yOffset));
                _texCoords->push_back(osg::Vec2f(1.0f, 0.25f));
                _vertices->push_back(osg::Vec2f(xOffset, 0.0 + yOffset));
                _texCoords->push_back(osg::Vec2f(1.0f, 0.5f));
                _vertices->push_back(osg::Vec2f(xOffset, 256.0 + yOffset));
                _texCoords->push_back(osg::Vec2f(0.5f, 0.5f));
                _vertices->push_back(osg::Vec2f(-xOffset, 256.0 + yOffset));
                maskPSet->set(osg::PrimitiveSet::QUADS, firstQuadVert, 4);
                firstQuadVert += 4;

                // The triangles aren't supposed to be textured, but there's
                // no need to set up a different Geometry, switch modes,
                // etc. I happen to know that there's a white pixel in the
                // texture at 1.0, 0.0 :)
                float centerY = tan(30 * SG_DEGREES_TO_RADIANS);
                _vertices->push_back(osg::Vec2f(0.0, 0.0));
                _vertices->push_back(osg::Vec2f(-256.0, 0.0));
                _vertices->push_back(osg::Vec2f(-256.0, 256.0 * centerY));

                _vertices->push_back(osg::Vec2f(0.0, 0.0));
                _vertices->push_back(osg::Vec2f(256.0, 0.0));
                _vertices->push_back(osg::Vec2f(256.0, 256.0 * centerY));

                _vertices->push_back(osg::Vec2f(-256, 0.0));
                _vertices->push_back(osg::Vec2f(256.0, 0.0));
                _vertices->push_back(osg::Vec2f(-256.0, -256.0));

                _vertices->push_back(osg::Vec2f(256, 0.0));
                _vertices->push_back(osg::Vec2f(256.0, -256.0));
                _vertices->push_back(osg::Vec2f(-256.0, -256.0));

                const osg::Vec2f whiteSpot(1.0f, 0.0f);
                for (int i = 0; i < 3 * 4; i++)
                    _texCoords->push_back(whiteSpot);

                trimaskPSet->set(osg::PrimitiveSet::TRIANGLES, firstQuadVert, 3 * 4);

            } else
            {
                maskPSet->set(osg::PrimitiveSet::QUADS, 0, 0);
                trimaskPSet->set(osg::PrimitiveSet::TRIANGLES, 0, 0);
            }

            maskPSet->dirty();
            trimaskPSet->dirty();
        }
#endif

        // remember index of next vertex
        int vIndex = _vertices->size();

        update_weather();

        osg::DrawArrays *quadPSet
            = static_cast<osg::DrawArrays*>(_geom->getPrimitiveSet(0));

        update_aircraft();
        update_tacan();
        update_heading_marker();

        // draw all new vertices are quads
        quadPSet->set(osg::PrimitiveSet::QUADS, vIndex, _vertices->size()-vIndex);
        quadPSet->dirty();
    }
}


void
wxRadarBg::update_weather()
{
    string modeButton = _Instrument->getStringValue("mode", "WX");
// FIXME: implementation of radar echoes missing
//    _radarEchoBuffer = *sgEnviro.get_radar_echo();

    // pretend we have a scan angle bigger then the FOV
    // TODO:check real fov, enlarge if < nn, and do clipping if > mm
//    const float fovFactor = 1.45f;
    _Instrument->setStringValue("status", modeButton.c_str());

// FIXME: implementation of radar echoes missing
#if 0
    list_of_SGWxRadarEcho *radarEcho = &_radarEchoBuffer;
    list_of_SGWxRadarEcho::iterator iradarEcho, end = radarEcho->end();
    const float LWClevel[] = { 0.1f, 0.5f, 2.1f };

    // draw the cloud radar echo
    bool drawClouds = _radar_weather_node->getBoolValue();
    if (drawClouds) {

        // we do that in 3 passes, one for each color level
        // this is to 'merge' same colors together
        for (int level = 0; level <= 2; level++) {
            float col = level * UNIT;

            for (iradarEcho = radarEcho->begin(); iradarEcho != end; ++iradarEcho) {
                int cloudId = iradarEcho->cloudId;
                bool upgrade = (cloudId >> 5) & 1;
                float lwc = iradarEcho->LWC + (upgrade ? 1.0f : 0.0f);

                // skip ns
                if (iradarEcho->LWC >= 0.5 && iradarEcho->LWC <= 0.6)
                    continue;

                if (iradarEcho->lightning || lwc < LWClevel[level])
                    continue;

                float radius = sqrt(iradarEcho->dist) * SG_METER_TO_NM * _scale;
                float size = iradarEcho->radius * 2.0 * SG_METER_TO_NM * _scale;

                if (radius - size > 180)
                    continue;

                float angle = (iradarEcho->heading - _angle_offset) //* fovFactor
                    + 0.5 * SG_PI;

                // Rotate echo into position, and rotate echo to have
                // a constant orientation towards the
                // airplane. Compass headings increase in clockwise
                // direction, while graphics rotations follow
                // right-hand (counter-clockwise) rule.
                const osg::Vec2f texBase(col, (UNIT * (float) (4 + (cloudId & 3))));

                osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
                    * osg::Matrixf::translate(0.0f, radius, 0.0f)
                    * wxRotate(angle) * _centerTrans);
                addQuad(_vertices, _texCoords, m, texBase);

                //SG_LOG(SG_INSTR, SG_DEBUG, "Radar: drawing clouds"
                //        << " ID=" << cloudId
                //        << " x=" << x
                //        << " y="<< y
                //        << " radius=" << radius
                //        << " view_heading=" << _view_heading * SG_RADIANS_TO_DEGREES
                //        << " heading=" << iradarEcho->heading * SG_RADIANS_TO_DEGREES
                //        << " angle=" << angle * SG_RADIANS_TO_DEGREES);
            }
        }
    }

    // draw lightning echos
    bool drawLightning = _Instrument->getBoolValue("lightning", true);
    if (drawLightning) {
        const osg::Vec2f texBase(3 * UNIT, 4 * UNIT);

        for (iradarEcho = radarEcho->begin(); iradarEcho != end; ++iradarEcho) {
            if (!iradarEcho->lightning)
                continue;

            float size = UNIT * 0.5f;
            float radius = iradarEcho->dist * _scale;
            float angle = iradarEcho->heading * SG_DEGREES_TO_RADIANS
                - _angle_offset;

            osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
                * wxRotate(-angle)
                * osg::Matrixf::translate(0.0f, radius, 0.0f)
                * wxRotate(angle) * _centerTrans);
            addQuad(_vertices, _texCoords, m, texBase);
        }
    }
#endif
}


void
wxRadarBg::update_data(const SGPropertyNode *ac, double altitude, double heading,
                       double radius, double bearing, bool selected)
{
    osgText::Text *callsign = new osgText::Text;
    callsign->setFont(_font.get());
    callsign->setFontResolution(12, 12);
    callsign->setCharacterSize(_font_size);
    callsign->setColor(selected ? osg::Vec4(1, 1, 1, 1) : _font_color);
    osg::Matrixf m(wxRotate(-bearing)
        * osg::Matrixf::translate(0.0f, radius, 0.0f)
        * wxRotate(bearing) * _centerTrans);

    osg::Vec3 pos = m.preMult(osg::Vec3(16, 16, 0));
    // cast to int's, otherwise text comes out ugly
    callsign->setPosition(osg::Vec3((int)pos.x(), (int)pos.y(), 0));
    callsign->setAlignment(osgText::Text::LEFT_BOTTOM_BASE_LINE);
    callsign->setLineSpacing(_font_spacing);

    const char *identity = ac->getStringValue("transponder-id");
    if (!identity[0])
        identity = ac->getStringValue("callsign");

    stringstream text;
    text << identity << endl
        << setprecision(0) << fixed
        << setw(3) << setfill('0') << heading * SG_RADIANS_TO_DEGREES << "\xB0 "
        << setw(0) << altitude << "ft" << endl
        << ac->getDoubleValue("velocities/true-airspeed-kt") << "kts";

    callsign->setText(text.str());
    _textGeode->addDrawable(callsign);
}


void
wxRadarBg::update_aircraft()
{
    double diff;
    double age_factor = 1.0;
    double test_rng;
    double test_brg;
    double range;
    double bearing;
    float echo_radius;
    double angle;

    if (!ground_echoes.empty()){
        ground_echoes_iterator = ground_echoes.begin();

        while(ground_echoes_iterator != ground_echoes.end()) {
            diff = _elapsed_time - (*ground_echoes_iterator)->elapsed_time;

            if( diff > _persistance) {
                ground_echoes.erase(ground_echoes_iterator++);
            } else {
//                double test_brg = (*ground_echoes_iterator)->bearing;
//                double bearing = test_brg * SG_DEGREES_TO_RADIANS;
//                float angle = calcRelBearing(bearing, _view_heading);
                double bumpinessFactor  = (*ground_echoes_iterator)->bumpiness;
                float heading = fgGetDouble("/orientation/heading-deg");
                if ( _display_mode == BSCAN ){
                    test_rng = (*ground_echoes_iterator)->elevation * 6;
                    test_brg = (*ground_echoes_iterator)->bearing;
                    angle = calcRelBearingDeg(test_brg, heading) * 6;
                    range = sqrt(test_rng * test_rng + angle * angle);
                    bearing = atan2(angle, test_rng);
                    //cout << "angle " << angle <<" bearing "
                    //    << bearing / SG_DEGREES_TO_RADIANS <<  endl;
                    echo_radius = (0.1 + (1.9 * bumpinessFactor)) * 240 * age_factor;
                } else {
                    test_rng = (*ground_echoes_iterator)->range;
                    range = test_rng * SG_METER_TO_NM;
                    test_brg = (*ground_echoes_iterator)->bearing;
                    bearing = test_brg * SG_DEGREES_TO_RADIANS;
                    echo_radius = (0.1 + (1.9 * bumpinessFactor)) * 120 * age_factor;
                    bearing += _angle_offset;
                }

                float radius = range * _scale;
                //double heading = 90 * SG_DEGREES_TO_RADIANS;
                //heading += _angle_offset;

                age_factor = 1;

                if (diff != 0)
                    age_factor = 1 - (0.5 * diff/_persistance);

                float size = echo_radius * UNIT;

                const osg::Vec2f texBase(3 * UNIT, 3 * UNIT);
                osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
                    * osg::Matrixf::translate(0.0f, radius, 0.0f)
                    * wxRotate(bearing) * _centerTrans);
                addQuad(_vertices, _texCoords, m, texBase);

                ++ground_echoes_iterator;

                //cout << "test bearing " << test_brg 
                //<< " test_rng " << test_rng * SG_METER_TO_NM
                //<< " persistance " << _persistance
                //<< endl;
            }

        }

    }
    if (!_ai_enabled_node->getBoolValue())
        return;

    bool draw_tcas     = _radar_tcas_node->getBoolValue();
    bool draw_absolute = _radar_absalt_node->getBoolValue();
    bool draw_echoes   = _radar_position_node->getBoolValue();
    bool draw_symbols  = _radar_symbol_node->getBoolValue();
    bool draw_data     = _radar_data_node->getBoolValue();
    if (!draw_echoes && !draw_symbols && !draw_data)
        return;

    double user_lat = _user_lat_node->getDoubleValue();
    double user_lon = _user_lon_node->getDoubleValue();
    double user_alt = _user_alt_node->getDoubleValue();

    float limit = _radar_coverage_node->getFloatValue();
    if (limit > 180)
        limit = 180;
    else if (limit < 0)
        limit = 0;
    limit *= SG_DEGREES_TO_RADIANS;

    int selected_id = fgGetInt("/instrumentation/radar/selected-id", -1);

    const SGPropertyNode *selected_ac = 0;
    const SGPropertyNode *ai = fgGetNode("/ai/models", true);

    for (int i = ai->nChildren() - 1; i >= -1; i--) {
        const SGPropertyNode *model;

        if (i < 0) { // last iteration: selected model
            model = selected_ac;
        } else {
            model = ai->getChild(i);
            if (!model->nChildren())
                continue;
            if ((model->getIntValue("id") == selected_id)&&
                (!draw_tcas)) {
                selected_ac = model;  // save selected model for last iteration
                continue;
            }
        }
        if (!model)
            continue;

        double echo_radius, sigma;
        const string name = model->getName();

        //cout << "name "<<name << endl;
        if (name == "aircraft" || name == "tanker")
            echo_radius = 1, sigma = 1;
        else if (name == "multiplayer" || name == "wingman" || name == "static")
            echo_radius = 1.5, sigma = 1;
        else if (name == "ship" || name == "carrier" || name == "escort" ||name == "storm")
            echo_radius = 1.5, sigma = 100;
        else if (name == "thermal")
            echo_radius = 2, sigma = 100;
        else if (name == "rocket")
            echo_radius = 0.1, sigma = 0.1;
        else if (name == "ballistic")
            echo_radius = 0.001, sigma = 0.001;
        else
            continue;

        double lat = model->getDoubleValue("position/latitude-deg");
        double lon = model->getDoubleValue("position/longitude-deg");
        double alt = model->getDoubleValue("position/altitude-ft");
        double heading = model->getDoubleValue("orientation/true-heading-deg");

        double range, bearing;
        calcRangeBearing(user_lat, user_lon, lat, lon, range, bearing);
        //cout << _antenna_ht << _interval<< endl;
        bool isVisible = withinRadarHorizon(user_alt, alt, range);

        if (!isVisible)
            continue;

        if (!inRadarRange(sigma, range))
            continue;

        bearing *= SG_DEGREES_TO_RADIANS;
        heading *= SG_DEGREES_TO_RADIANS;

        float radius = range * _scale;
        float angle = calcRelBearing(bearing, _view_heading);

        if (angle > limit || angle < -limit)
            continue;

        bearing += _angle_offset;
        heading += _angle_offset;

        bool is_tcas_contact = false;
        if (draw_tcas)
        {
            is_tcas_contact = update_tcas(model,range,user_alt,alt,bearing,radius,draw_absolute);
        }

        // pos mode
        if (draw_echoes && (!is_tcas_contact)) {
            float size = echo_radius * 120 * UNIT;

            const osg::Vec2f texBase(3 * UNIT, 3 * UNIT);
            osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
                * osg::Matrixf::translate(0.0f, radius, 0.0f)
                * wxRotate(bearing) * _centerTrans);
            addQuad(_vertices, _texCoords, m, texBase);
        }

        // data mode
        if (draw_symbols && (!draw_tcas)) {
            const osg::Vec2f texBase(0, 3 * UNIT);
            float size = 600 * UNIT;
            osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
                * wxRotate(heading - bearing)
                * osg::Matrixf::translate(0.0f, radius, 0.0f)
                * wxRotate(bearing) * _centerTrans);
            addQuad(_vertices, _texCoords, m, texBase);
        }

        if ((draw_data || i < 0)&&  // selected one (i == -1) is always drawn
            ((!draw_tcas)||(is_tcas_contact)||(draw_echoes)))
            update_data(model, alt, heading, radius, bearing, i < 0);
    }
}

/** Update TCAS display.
 * Return true when processed as TCAS contact, false otherwise. */
bool
wxRadarBg::update_tcas(const SGPropertyNode *model,double range,double user_alt,double alt,
                       double bearing,double radius,bool absMode)
{
    int threatLevel=0;
    {
        // update TCAS symbol
        osg::Vec2f texBase;
        threatLevel = model->getIntValue("tcas/threat-level",-1);
        if (threatLevel == -1)
        {
            // no TCAS information (i.e. no transponder) => not visible to TCAS
            return false;
        }
        int row = 7 - threatLevel;
        int col = 4;
        double vspeed = model->getDoubleValue("velocities/vertical-speed-fps");
        if (vspeed < -3.0) // descending
            col+=1;
        else
        if (vspeed > 3.0) // climbing
            col+=2;
        texBase = osg::Vec2f(col*UNIT,row * UNIT);
        float size = 200 * UNIT;
            osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
                * wxRotate(-bearing)
                * osg::Matrixf::translate(0.0f, radius, 0.0f)
                * wxRotate(bearing) * _centerTrans);
            addQuad(_vertices, _texCoords, m, texBase);
    }

    {
        // update TCAS data
        osgText::Text *altStr = new osgText::Text;
        altStr->setFont(_font.get());
        altStr->setFontResolution(12, 12);
        altStr->setCharacterSize(_font_size);
        altStr->setColor(_tcas_colors[threatLevel]);
        osg::Matrixf m(wxRotate(-bearing)
            * osg::Matrixf::translate(0.0f, radius, 0.0f)
            * wxRotate(bearing) * _centerTrans);
    
        osg::Vec3 pos = m.preMult(osg::Vec3(16, 16, 0));
        // cast to int's, otherwise text comes out ugly
        altStr->setLineSpacing(_font_spacing);
    
        stringstream text;
        altStr->setAlignment(osgText::Text::LEFT_CENTER);
        int altDif = (alt-user_alt+50)/100;
        char sign = 0;
        int dy=0;
        if (altDif>=0)
        {
            sign='+';
            dy=2;
        }
        else
        if (altDif<0)
        {
            sign='-';
            altDif = -altDif;
            dy=-30;
        }
        altStr->setPosition(osg::Vec3((int)pos.x()-30, (int)pos.y()+dy, 0));
        if (absMode)
        {
            // absolute altitude display
            text << setprecision(0) << fixed
                 << setw(3) << setfill('0') << alt/100 << endl;
        }
        else // relative altitude display
        if (sign)
        {
            text << sign
                 << setprecision(0) << fixed
                 << setw(2) << setfill('0') << altDif << endl;
        }
    
        altStr->setText(text.str());
        _textGeode->addDrawable(altStr);
    }

    return true;
}

void
wxRadarBg::update_tacan()
{
    // draw TACAN symbol
    int mode = _radar_mode_control_node->getIntValue();
    bool inRange = _tacan_in_range_node->getBoolValue();

    if (mode != 1 || !inRange)
        return;

    float size = 600 * UNIT;
    float radius = _tacan_distance_node->getFloatValue() * _scale;
    float angle = _tacan_bearing_node->getFloatValue() * SG_DEGREES_TO_RADIANS
        + _angle_offset;

    const osg::Vec2f texBase(1 * UNIT, 3 * UNIT);
    osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
        * wxRotate(-angle)
        * osg::Matrixf::translate(0.0f, radius, 0.0f)
        * wxRotate(angle) * _centerTrans);
    addQuad(_vertices, _texCoords, m, texBase);

    //SG_LOG(SG_INSTR, SG_DEBUG, "Radar:     drawing TACAN"
    //        << " dist=" << radius
    //        << " view_heading=" << _view_heading * SG_RADIANS_TO_DEGREES
    //        << " bearing=" << angle * SG_RADIANS_TO_DEGREES
    //        << " x=" << x << " y="<< y
    //        << " size=" << size);
}


void
wxRadarBg::update_heading_marker()
{
    if (!_radar_hdg_marker_node->getBoolValue())
        return;

    const osg::Vec2f texBase(2 * UNIT, 3 * UNIT);
    float size = 600 * UNIT;
    osg::Matrixf m(osg::Matrixf::scale(size, size, 1.0f)
        * wxRotate(_view_heading + _angle_offset));

    m *= _centerTrans;
    addQuad(_vertices, _texCoords, m, texBase);

    //SG_LOG(SG_INSTR, SG_DEBUG, "Radar:   drawing heading marker"
    //        << " x,y " << x <<","<< y
    //        << " dist" << dist
    //        << " view_heading" << _view_heading * SG_RADIANS_TO_DEGREES
    //        << " heading " << iradarEcho->heading * SG_RADIANS_TO_DEGREES
    //        << " angle " << angle * SG_RADIANS_TO_DEGREES);
}


void
wxRadarBg::center_map()
{
    _lat = _user_lat_node->getDoubleValue();
    _lon = _user_lon_node->getDoubleValue();
    _x_offset = _y_offset = 0;
}


void
wxRadarBg::apply_map_offset()
{
    double lat = _user_lat_node->getDoubleValue();
    double lon = _user_lon_node->getDoubleValue();
    double bearing, distance, az2;
    geo_inverse_wgs_84(_lat, _lon, lat, lon, &bearing, &az2, &distance);
    distance *= SG_METER_TO_NM * _scale;
    bearing *= SG_DEGREES_TO_RADIANS;
    _x_offset += sin(bearing) * distance;
    _y_offset += cos(bearing) * distance;
    _lat = lat;
    _lon = lon;
}


bool
wxRadarBg::withinRadarHorizon(double user_alt, double alt, double range_nm)
{
    // Radar Horizon  = 1.23(ht^1/2 + hr^1/2),
    //don't allow negative altitudes (an approximation - yes altitudes can be negative)
    // Allow antenna ht to be set, but only on ground
    _antenna_ht = _Instrument->getDoubleValue("antenna-ht-ft");

    if (user_alt <= 0)
        user_alt = _antenna_ht;

    if (alt <= 0)
        alt = 0; // to allow some vertical extent of target

    double radarhorizon = 1.23 * (sqrt(alt) + sqrt(user_alt));
//    SG_LOG(SG_INSTR, SG_ALERT, "Radar: radar horizon " << radarhorizon);
    return radarhorizon >= range_nm;
}


bool
wxRadarBg::inRadarRange(double sigma, double range_nm)
{
    //The Radar Equation:
    //
    // MaxRange^4 = (TxPower * AntGain^2 * lambda^2 * sigma)/((constant) * MDS)
    //
    // Where (constant) = (4*pi)3 and MDS is the Minimum Detectable Signal power.
    //
    // For a given radar we can assume that the only variable is sigma,
    // the target radar cross section.
    //
    // Here, we will use a normalised rcs (sigma) for a standard taget and assume that this
    // will provide a maximum range of 35nm;
    //
    // TODO - make the maximum range adjustable at runtime

    double constant = _radar_ref_rng;

    if (constant <= 0)
        constant = 35;

    double maxrange = constant * pow(sigma, 0.25);
    //SG_LOG(SG_INSTR, SG_DEBUG, "Radar: max range " << maxrange);
    return maxrange >= range_nm;
}


void
wxRadarBg::calcRangeBearing(double lat, double lon, double lat2, double lon2,
                            double &range, double &bearing) const
{
    // calculate the bearing and range of the second pos from the first
    double az2, distance;
    geo_inverse_wgs_84(lat, lon, lat2, lon2, &bearing, &az2, &distance);
    range = distance *= SG_METER_TO_NM;
}


float
wxRadarBg::calcRelBearing(float bearing, float heading)
{
    float angle = bearing - heading;

    if (angle >= SG_PI)
        angle -= 2.0 * SG_PI;

    if (angle < -SG_PI)
        angle += 2.0 * SG_PI;

    return angle;
}

float
wxRadarBg::calcRelBearingDeg(float bearing, float heading)
{
    float angle = bearing - heading;

    if (angle >= 180)
        return angle -= 360;

    if (angle < -180)
        return angle += 360;

    return angle;
}


void
wxRadarBg::updateFont()
{
    float red = _font_node->getFloatValue("color/red");
    float green = _font_node->getFloatValue("color/green");
    float blue = _font_node->getFloatValue("color/blue");
    float alpha = _font_node->getFloatValue("color/alpha");
    _font_color.set(red, green, blue, alpha);

    _font_size = _font_node->getFloatValue("size");
    _font_spacing = _font_size * _font_node->getFloatValue("line-spacing");
    string path = _font_node->getStringValue("name", DEFAULT_FONT);

    SGPath tpath;
    if (path[0] != '/') {
        tpath = globals->get_fg_root();
        tpath.append("Fonts");
        tpath.append(path);
    } else {
        tpath = path;
    }

#if (FG_OSG_VERSION >= 21000)
    osg::ref_ptr<osgDB::ReaderWriter::Options> fontOptions = new osgDB::ReaderWriter::Options("monochrome");
    osg::ref_ptr<osgText::Font> font = osgText::readFontFile(tpath.c_str(), fontOptions.get());
#else
    osg::ref_ptr<osgText::Font> font = osgText::readFontFile(tpath.c_str());
#endif

    if (font != 0) {
        _font = font;
        _font->setMinFilterHint(osg::Texture::NEAREST);
        _font->setMagFilterHint(osg::Texture::NEAREST);
        _font->setGlyphImageMargin(0);
        _font->setGlyphImageMarginRatio(0);
    }

    for (int i=0;i<4;i++)
    {
        const float defaultColors[4][3] = {{0,1,1},{0,1,1},{1,0.5,0},{1,0,0}};
        SGPropertyNode_ptr color_node = _font_node->getNode("tcas/color",i,true);
        float red   = color_node->getFloatValue("red",defaultColors[i][0]);
        float green = color_node->getFloatValue("green",defaultColors[i][1]);
        float blue  = color_node->getFloatValue("blue",defaultColors[i][2]);
        float alpha = color_node->getFloatValue("alpha",1);
        _tcas_colors[i]=osg::Vec4(red, green, blue, alpha);
    }
}

void
wxRadarBg::valueChanged(SGPropertyNode*)
{
    updateFont();
    _time = _interval;
}

