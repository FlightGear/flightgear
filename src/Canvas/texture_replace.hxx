/*
 * SPDX-FileName: texture_replace.hxx
 * SPDX-FileCopyrightText: Copyright (C) 2012 Thomas Geymayer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <simgear/canvas/canvas_fwd.hxx>

namespace canvas {

/**
 * Replace an opengl texture name inside a given branch of the scene graph.
 * This is to replace a static texture by a dynamic one
 *
 * @param branch        Scene graph branch to use for search
 * @param name          PBR texture name
 * @param new_texture   dynamic texture to replace the old one
 * @return A list of groups which override the given texture
 */
simgear::canvas::Placements
set_texture( osg::Node* branch,
             const char * name,
             osg::Texture2D* new_texture );

/**
 * Replace an opengl texture name inside the aircraft scene graph.
 * This is to replace a static texture by a dynamic one
 *
 * @param branch        Scene graph branch to search for matching
 * @param name          PBR texture name
 * @param new_texture   dynamic texture to replace the old one
 * @return A list of groups which override the given texture
 */
simgear::canvas::Placements
set_aircraft_texture( const char * name,
                      osg::Texture2D* new_texture );

/**
 * Replace an opengl texture name inside a given branch of the scene graph.
 * This is to replace a static texture by a dynamic one. The replacement
 * is base on certain filtering criteria which have to be stored in string
 * value childs of the placement node. Recognized nodes are:
 *   - texture  Match the PBR texture name
 *   - node     Match the name of the object
 *   - parent   Match any of the object parents names (all the tree upwards)
 *
 * @param placement the node containing the replacement criteria
 * @param new_texture dynamic texture to replace the old one
 * @param an optional cull callback which will be installed on any matching
 *        object
 * @return A list of groups which override the given texture
 */
simgear::canvas::Placements
set_texture( osg::Node* branch,
             SGPropertyNode* placement,
             osg::Texture2D* new_texture,
             osg::NodeCallback* cull_callback = 0,
             const simgear::canvas::CanvasWeakPtr& canvas =
             simgear::canvas::CanvasWeakPtr() );

/**
 * Replace an opengl texture name inside the aircraft scene graph.
 *
 * @param placement the node containing the replacement criteria
 * @param new_texture dynamic texture to replace the old one
 * @param an optional cull callback which will be installed on any matching
 *        object
 * @return A list of groups which override the given texture
 */
simgear::canvas::Placements
set_aircraft_texture( SGPropertyNode* placement,
                      osg::Texture2D* new_texture,
                      osg::NodeCallback* cull_callback = 0,
                      const simgear::canvas::CanvasWeakPtr& canvas =
                      simgear::canvas::CanvasWeakPtr() );

} // namespace canvas
