// Copyright (C) 2008 Tim Moore
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

#ifndef FLIGHTGEAR_WINDOWSYSTEMADAPTER_HXX
#define FLIGHTGEAR_WINDOWSYSTEMADAPTER_HXX 1

#include <functional>
#include <string>

#include <osg/Referenced>
#include <osg/Camera>
#include <osg/GraphicsThread>
#include <osg/ref_ptr>

#include <simgear/structure/SGAtomic.hxx>

namespace osg
{
class GraphicsContext;
}

// Flexible window support
namespace flightgear
{
/** A window with a graphics context and an integer ID
 */
class GraphicsWindow : public osg::Referenced
{
public:
    GraphicsWindow(osg::GraphicsContext* gc_, const std::string& name_,
                   int id_, unsigned flags_ = 0) :
        gc(gc_), name(name_), id(id_), flags(flags_)
    {
    }
    /** The OSG graphics context for this window.
     */
    osg::ref_ptr<osg::GraphicsContext> gc;
    /** The window's internal name.
     */
    std::string name;
    /** A unique ID for the window.
     */
    int id;
    enum Flags {
        GUI = 1 /**< The GUI (and 2D cockpit) will be drawn on this window. */
    };
    /** Flags for the window.
     */
    unsigned flags;
};

typedef std::vector<osg::ref_ptr<GraphicsWindow> >  WindowVector;

/**
 * An operation that is run once with a particular GraphicsContext
 * current. It will probably be deferred and may run in a different
 * thread.
 */
class GraphicsContextOperation : public osg::GraphicsOperation
{
public:
    GraphicsContextOperation(const std::string& name) :
        osg::GraphicsOperation(name, false)
    {
    }
    /** Don't override this!
     */
    virtual void operator()(osg::GraphicsContext* gc);
    /** The body of the operation.
     */
    virtual void run(osg::GraphicsContext* gc) = 0;
    /** Test if the operation has completed.
     * @return true if the run() method has finished.
     */
    bool isFinished() const { return done != 0; }
private:
    SGAtomic done;
};

/** Adapter from windows system / graphics context management API to
 * functions used by flightgear. This papers over the difference
 * between osgViewer::Viewer, which handles multiple windows, graphics
 * threads, etc., and the embedded viewer used with GLUT and SDL.
 */
class WindowSystemAdapter : public osg::Referenced
{
public:
    WindowSystemAdapter();
    virtual ~WindowSystemAdapter() {}
    /** Vector of all the registered windows.
     */
    WindowVector windows;
    /** Register a window, assigning  it an ID.
     * @param gc graphics context
     * @param windowName internal name (not displayed)
     * @return a graphics window
     */
    GraphicsWindow* registerWindow(osg::GraphicsContext* gc,
                                   const std::string& windowName);
    /** Initialize the plib pui interface library. This might happen
     *in another thread and may be deferred.
     */
    virtual void puInitialize();
    /** Find a window by name.
     * @param name the window name
     * @return the window or 0
     */
    GraphicsWindow* findWindow(const std::string& name);
    /** Get the global WindowSystemAdapter
     * @return the adapter
     */
    static WindowSystemAdapter* getWSA() { return _wsa.get(); }
    /** Set the global adapter
     * @param wsa the adapter
     */
    static void setWSA(WindowSystemAdapter* wsa) { _wsa = wsa; }
protected:
    int _nextWindowID;
    osg::ref_ptr<GraphicsContextOperation> _puInitOp;
    bool _isPuInitialized;
    static osg::ref_ptr<WindowSystemAdapter> _wsa;
    // Default callbacks for plib
    static int puGetWindow();
    static void puGetWindowSize(int* width, int* height);

};

/**
 * Class for testing if flags are set in an object with a flags member.
 */
template<typename T>
class FlagTester : public std::unary_function<osg::ref_ptr<T>, bool>
{
public:
    /** Initialize with flags to test for.
     * @param flags logical or of flags to test.
     */
    FlagTester(unsigned flags_) : flags(flags_) {}
    /** test operator
     * @param obj An object with a flags member
     * @return true if flags member of obj contains any of the flags
     * (bitwise and with flags is nonzero).
     */
    bool operator() (const osg::ref_ptr<T>& obj)
    {
        return (obj->flags & flags) != 0;
    }
    unsigned flags;
};

}
#endif
