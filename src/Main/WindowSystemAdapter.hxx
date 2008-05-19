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
#include <osg/GraphicsContext>
#include <osg/GraphicsThread>

#include <simgear/structure/SGAtomic.hxx>

// Flexible Camera and window support
namespace flightgear
{
/** A window opened by default or via rendering properties
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
    enum Flags {
        /** The GUI (and 2D cockpit) will be drawn on this window.
         */
        GUI = 1
    };
    int id;
    unsigned flags;
};

/** Camera associated with a 3d view. The camera might occupy an
 * entire window or share one with other cameras.
 */
class Camera3D : public osg::Referenced
{
public:
    Camera3D(GraphicsWindow* window_, osg::Camera* camera_, const std::string& name_,
             unsigned flags_ = 0) :
        window(window_), camera(camera_), name(name_), flags(flags_)
    {
    }
    osg::ref_ptr<GraphicsWindow> window;
    osg::ref_ptr<osg::Camera> camera;
    std::string name;
    enum Flags {
        SHARES_WINDOW = 1,      /**< Camera shares window with other cameras*/
        MASTER = 2              /**< Camera has same view as master camera*/
    };
    unsigned flags;
};

typedef std::vector<osg::ref_ptr<GraphicsWindow> >  WindowVector;
typedef std::vector<osg::ref_ptr<Camera3D> >  Camera3DVector;

/**
 * An operation that is run once with a particular GraphicsContext
 * current.
 */
class GraphicsContextOperation : public osg::GraphicsOperation
{
public:
    GraphicsContextOperation(const std::string& name) :
        osg::GraphicsOperation(name, false)
    {
    }
    virtual void operator()(osg::GraphicsContext* gc);
    virtual void run(osg::GraphicsContext* gc) = 0;
    bool isFinished() const { return done != 0; }
private:
    SGAtomic done;
};

/** Adapter from windows system / graphics context management API to
 * functions used by flightgear. This papers over the difference
 * between osgViewer Viewer, which handles multiple windows, graphics
 * threads, etc., and the embedded viewer used with GLUT and SDL.
 */
class WindowSystemAdapter : public osg::Referenced
{
public:
    WindowSystemAdapter();
    virtual ~WindowSystemAdapter() {}
    WindowVector windows;
    Camera3DVector cameras;
    GraphicsWindow* registerWindow(osg::GraphicsContext* gc,
                                   const std::string& windowName);
    Camera3D* registerCamera3D(GraphicsWindow* gw, osg::Camera* camera,
                               const std::string& cameraName);
    GraphicsWindow* getGUIWindow();
    int getGUIWindowID();
    osg::GraphicsContext* getGUIGraphicsContext();
    /** Initialize the plib pui interface library. This might happen
     *in another thread and may be deferred.
     */
    virtual bool puInitialize();
    /** Returns true if pui initialization has finished.
     */
    template<typename T>
    class FlagTester : public std::unary_function<osg::ref_ptr<T>, bool>
    {
    public:
        FlagTester(unsigned flags_) : flags(flags_) {}
        bool operator() (const osg::ref_ptr<T>& obj)
        {
            return (obj->flags & flags) != 0;
        }
        unsigned flags;
    };
    static WindowSystemAdapter* getWSA() { return _wsa.get(); }
    static void setWSA(WindowSystemAdapter* wsa) { _wsa = wsa; }
protected:
    int _nextWindowID;
    int _nextCameraID;
    osg::ref_ptr<GraphicsContextOperation> _puInitOp;
    bool _isPuInitialized;
    static osg::ref_ptr<WindowSystemAdapter> _wsa;
    // Default callbacks for plib
    static int puGetWindow();
    static void puGetWindowSize(int* width, int* height);

};
}
#endif
