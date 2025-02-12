#ifndef FGEVENTHANDLER_H
#define FGEVENTHANDLER_H 1

#include <map>
#include <osg/Quat>
#include <osgGA/GUIEventHandler>
#include <osgViewer/ViewerEventHandlers>

#include <Main/fg_os.hxx>

namespace flightgear
{
    class FGStatsHandler : public osgViewer::StatsHandler
    {
        public:
            FGStatsHandler()
            {
                // Adjust font type/size for >=OSG3.
                // OSG defaults aren't working/available for FG.
                _font = "Fonts/helvetica_medium.txf";
                _characterSize = 12.0f;
            }
    };

class FGEventHandler : public osgGA::GUIEventHandler {
public:
    FGEventHandler();
    
    virtual ~FGEventHandler() {}
    
    virtual const char* className() const {return "FGEventHandler"; }
#if 0
    virtual void init(const osgGA::GUIEventAdapter& ea,
		      osgGA::GUIActionAdapter& us);
#endif
    bool handle(const osgGA::GUIEventAdapter& ea,
			osgGA::GUIActionAdapter& us) override;

    void setIdleHandler(fgIdleHandler idleHandler)
	{
	    this->idleHandler = idleHandler;
	}

    fgIdleHandler getIdleHandler() const
	{
	    return idleHandler;
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

    void setChangeStatsCameraRenderOrder(bool c)
    {
        changeStatsCameraRenderOrder = c;
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

    void reset();
    void clear();
    
    // Wrapper for osgViewer::GraphicsWindow::setWindowRectangle() that takes
    // coordinates excluding window furniture.
    //
    void setWindowRectangleInteriorWithCorrection(osgViewer::GraphicsWindow* window, int x, int y, int width, int height);

    static int translateKey(const osgGA::GUIEventAdapter& ea);
    static int translateModifiers(const osgGA::GUIEventAdapter& ea);
protected:
    osg::ref_ptr<osg::Node> _node;
    fgIdleHandler idleHandler;
    fgKeyHandler keyHandler;
    fgMouseClickHandler mouseClickHandler;
    fgMouseMotionHandler mouseMotionHandler;
    osg::ref_ptr<FGStatsHandler> statsHandler;
    osg::ref_ptr<osgGA::GUIEventAdapter> statsEvent;
    int statsType;
    int currentModifiers;
    
    void handleKey(const osgGA::GUIEventAdapter& ea, int& key, int& modifiers);
    bool resizable;
    bool mouseWarped;
    // workaround for osgViewer double scroll events
    bool scrollButtonPressed;
    int release_keys[128];
    void handleStats(osgGA::GUIActionAdapter& us);
    bool changeStatsCameraRenderOrder;
    SGPropertyNode_ptr _display, _print;
private:
    bool m_setWindowRectangle_called = false;
    int m_setWindowRectangle_called_x = 0;
    int m_setWindowRectangle_called_y = 0;
    int m_setWindowRectangle_delta_x = 0;
    int m_setWindowRectangle_delta_y = 0;

    enum WindowType
    {
        WindowType_NONE,
        WindowType_MAIN,
        WindowType_SVIEW
    };
    WindowType
    eventToViewport(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us,
                    int& x, int& y);

    bool isMainWindow(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);
    
};

bool eventToWindowCoords(const osgGA::GUIEventAdapter* ea, double& x, double& y);
}
#endif
