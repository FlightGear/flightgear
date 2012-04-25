// Copyright (C) 2008  Tim Moore
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

#ifndef FLIGHTGEAR_WINDOWBUILDER_HXX
#define FLIGHTGEAR_WINDOWBUILDER_HXX 1

#include <osg/ref_ptr>
#include <osg/Referenced>
#include <osg/GraphicsContext>

#include <string>

class SGPropertyNode;

namespace flightgear
{
class GraphicsWindow;
/** Singleton Builder class for creating a GraphicsWindow from property
 * nodes. This involves initializing an osg::GraphicsContext::Traits
 * structure from the property node values and creating an
 * osgViewer::GraphicsWindow.
 */
class WindowBuilder : public osg::Referenced
{
public:
    /** Initialize the singleton window builder.
     * @param stencil whether windows should allocate stencil planes
     */
    static void initWindowBuilder(bool stencil);
    /** Get the singleton window builder
     */
    static WindowBuilder* getWindowBuilder() { return windowBuilder.get(); }
    /** Create a window from its property node description.
     * @param winNode The window's root property node
     * @return a graphics window.
     */
    GraphicsWindow* buildWindow(const SGPropertyNode* winNode);
    /** Get a window whose properties come from FlightGear's
     * command line arguments and their defaults. The window is opened
     * if it has not been already.
     * @return the default graphics window
     */
    GraphicsWindow* getDefaultWindow();
    /** Get the name used to look up the default window.
     */
    const std::string& getDefaultWindowName() { return defaultWindowName; }
protected:
    WindowBuilder(bool stencil);
    static osg::GraphicsContext::Traits* makeDefaultTraits(bool stencil);
    osg::ref_ptr<osg::GraphicsContext::Traits> defaultTraits;
    int defaultCounter;
    static osg::ref_ptr<WindowBuilder> windowBuilder;
    static const std::string defaultWindowName;
};

/** Silly function for making the default window and camera
 * names. This concatenates a string with in integer.
 */
std::string makeName(const std::string& prefix, int num);

}
#endif
