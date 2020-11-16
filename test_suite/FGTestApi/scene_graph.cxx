/*
 * Copyright (C) 2018 Edward d'Auvergne
 *
 * This file is part of the program FlightGear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "scene_graph.hxx"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/locale.hxx>
#include <Scenery/scenery.hxx>
#include <Viewer/renderer.hxx>
#include <Viewer/FGEventHandler.hxx>


namespace FGTestApi {
namespace setUp {

void initScenery()
{
    // Read the global defaults from $FG_ROOT/defaults.xml (needed by the renderer).
    SGPath defaultsXML = globals->get_fg_root() / "defaults.xml";
    if (!defaultsXML.exists())
        SG_LOG(SG_GENERAL, SG_ALERT, "Cannot read the global defaults from \"" << defaultsXML.utf8Str() << "\".");
    fgLoadProps("defaults.xml", globals->get_props());

    // otherwise fgSplashProgress will assert
    globals->get_locale()->selectLanguage({});
    
    // Set up the renderer.
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    FGRenderer* render = globals->get_renderer();
    render->init();
    render->setView(viewer.get());

    // Start up the scenery subsystem.
    globals->add_new_subsystem<FGScenery>(SGSubsystemMgr::DISPLAY);
    globals->get_scenery()->init();
    globals->get_scenery()->bind();
}

} // End of namespace setUp.
} // End of namespace FGTestApi.
