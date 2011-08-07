#ifndef FGEVENTHANDLER_H
#define FGEVENTHANDLER_H 1

#include <map>
#include <osg/Quat>
#include <osgGA/GUIEventHandler>
#include <osgViewer/ViewerEventHandlers>

#include "fg_os.hxx"

namespace flightgear
{
class FGEventHandler : public osgGA::GUIEventHandler {
public:
    FGEventHandler();
    
    virtual ~FGEventHandler() {}
    
    virtual const char* className() const {return "FGEventHandler"; }
#if 0
    virtual void init(const osgGA::GUIEventAdapter& ea,
		      osgGA::GUIActionAdapter& us);
#endif
    virtual bool handle(const osgGA::GUIEventAdapter& ea,
			osgGA::GUIActionAdapter& us);

    void setIdleHandler(fgIdleHandler idleHandler)
	{
	    this->idleHandler = idleHandler;
	}

    fgIdleHandler getIdleHandler() const
	{
	    return idleHandler;
	}

    void setDrawHandler(fgDrawHandler drawHandler)
	{
	    this->drawHandler = drawHandler;
	}

    fgDrawHandler getDrawHandler() const
	{
	    return drawHandler;
	}

    void setWindowResizeHandler(fgWindowResizeHandler windowResizeHandler)
	{
	    this->windowResizeHandler = windowResizeHandler;
	}
    
    fgWindowResizeHandler getWindowResizeHandler() const
	{
	    return windowResizeHandler;
	}

    void setKeyHandler(fgKeyHandler keyHandler)
	{
	    this->keyHandler = keyHandler;
	}

    fgKeyHandler getKeyHandler() const
	{
	    return keyHandler;
	}

    void setMouseClickHandler(fgMouseClickHandler mouseClickHandler)
	{
	    this->mouseClickHandler = mouseClickHandler;
	}
    
    fgMouseClickHandler getMouseClickHandler()
	{
	    return mouseClickHandler;
	}

    void setMouseMotionHandler(fgMouseMotionHandler mouseMotionHandler)
	{
	    this->mouseMotionHandler = mouseMotionHandler;
	}
    
    fgMouseMotionHandler getMouseMotionHandler()
	{
	    return mouseMotionHandler;
	}

    int getCurrentModifiers() const
	{
	    return currentModifiers;
	}

    void setMouseWarped()
	{
	    mouseWarped = true;
	}

    /** Whether or not resizing is supported. It might not be when
     * using multiple displays.
     */
    bool getResizable() { return resizable; }
    void setResizable(bool _resizable) { resizable = _resizable; }

protected:
    osg::ref_ptr<osg::Node> _node;
    fgIdleHandler idleHandler;
    fgDrawHandler drawHandler;
    fgWindowResizeHandler windowResizeHandler;
    fgKeyHandler keyHandler;
    fgMouseClickHandler mouseClickHandler;
    fgMouseMotionHandler mouseMotionHandler;
    osg::ref_ptr<osgViewer::StatsHandler> statsHandler;
    osg::ref_ptr<osgGA::GUIEventAdapter> statsEvent;
    int statsType;
    int currentModifiers;
    std::map<int, int> numlockKeyMap;
    std::map<int, int> noNumlockKeyMap;
    void handleKey(const osgGA::GUIEventAdapter& ea, int& key, int& modifiers);
    bool resizable;
    bool mouseWarped;
    // workaround for osgViewer double scroll events
    bool scrollButtonPressed;
    int release_keys[128];
    void handleStats(osgGA::GUIActionAdapter& us);
};

void eventToWindowCoords(const osgGA::GUIEventAdapter* ea, double& x, double& y);
void eventToWindowCoordsYDown(const osgGA::GUIEventAdapter* ea,
                              double& x, double& y);
}
#endif
