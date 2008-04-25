#ifndef FGMANIPULATOR_H
#define FGMANIPULATOR_H 1

#include <map>
#include <osg/Quat>
#include <osgGA/MatrixManipulator>
#include <osgViewer/ViewerEventHandlers>

#include "fg_os.hxx"

class FGManipulator : public osgGA::MatrixManipulator {
public:
    FGManipulator();
    
    virtual ~FGManipulator() {}
    
    virtual const char* className() const {return "FGManipulator"; }

    /** set the position of the matrix manipulator using a 4x4 Matrix.*/
    virtual void setByMatrix(const osg::Matrixd& matrix);

    virtual void setByInverseMatrix(const osg::Matrixd& matrix)
	{ setByMatrix(osg::Matrixd::inverse(matrix)); }

    /** get the position of the manipulator as 4x4 Matrix.*/
    virtual osg::Matrixd getMatrix() const;

    /** get the position of the manipulator as a inverse matrix of the manipulator, typically used as a model view matrix.*/
    virtual osg::Matrixd getInverseMatrix() const;

    virtual void setNode(osg::Node* node);
    
    const osg::Node* getNode() const;

    osg::Node* getNode();

    virtual void init(const osgGA::GUIEventAdapter& ea,
		      osgGA::GUIActionAdapter& us);

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

    void setPosition(const osg::Vec3d position) { this->position = position; }
    void setAttitude(const osg::Quat attitude) { this->attitude = attitude; }

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
#if 0
    std::map<int, int> numlockKeyMap;
#endif
    osg::Vec3d position;
    osg::Quat attitude;
    void handleKey(const osgGA::GUIEventAdapter& ea, int& key, int& modifiers);
    bool resizable;
    bool mouseWarped;
    // workaround for osgViewer double scroll events
    bool scrollButtonPressed;
    int release_keys[128];
    void handleStats(osgGA::GUIActionAdapter& us);
};
#endif
