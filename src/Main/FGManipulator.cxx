#include <osg/Math>

#include <osgViewer/Viewer>

#include <plib/pu.h>
#include "FGManipulator.hxx"

// The manipulator is responsible for updating a Viewer's camera. It's
// event handling method is also a convenient place to run the the FG
// idle and draw handlers.

void FGManipulator::setByMatrix(const osg::Matrixd& matrix)
{
    // Yuck
    position = matrix.getTrans();
    attitude = matrix.getRotate();
}

osg::Matrixd FGManipulator::getMatrix() const
{
    return osg::Matrixd::rotate(attitude) * osg::Matrixd::translate(position);
}

osg::Matrixd FGManipulator::getInverseMatrix() const
{
    return (osg::Matrixd::translate(-position)
	    * osg::Matrixd::rotate(attitude.inverse())) ;
}

// Not used, but part of the interface.
void FGManipulator::setNode(osg::Node* node)
{
    _node = node;
}
    
const osg::Node* FGManipulator::getNode() const
{
    return _node.get();
}

osg::Node* FGManipulator::getNode()
{
    return _node.get();
}

namespace {
// All the usual translation from window system to FG / plib
int osgToFGModifiers(int modifiers)
{
    int result = 0;
    if (modifiers & (osgGA::GUIEventAdapter::MODKEY_LEFT_SHIFT |
		     osgGA::GUIEventAdapter::MODKEY_RIGHT_SHIFT))
	result |= KEYMOD_SHIFT;
    if (modifiers & (osgGA::GUIEventAdapter::MODKEY_LEFT_CTRL |
		     osgGA::GUIEventAdapter::MODKEY_RIGHT_CTRL))
	result |= KEYMOD_CTRL;
    if (modifiers & (osgGA::GUIEventAdapter::MODKEY_LEFT_ALT |
		     osgGA::GUIEventAdapter::MODKEY_RIGHT_ALT))
	result |= KEYMOD_ALT;
    return result;
}
} // namespace

void FGManipulator::init(const osgGA::GUIEventAdapter& ea,
			 osgGA::GUIActionAdapter& us)
{
    currentModifiers = osgToFGModifiers(ea.getModKeyMask());
    (void)handle(ea, us);
}

namespace {
void eventToViewport(const osgGA::GUIEventAdapter& ea,
		     osgGA::GUIActionAdapter& us,
		     int& x, int& y)
{
    osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(&us);
    osg::Viewport* viewport = viewer->getCamera()->getViewport();
    const osg::GraphicsContext::Traits* traits
	= viewer->getCamera()->getGraphicsContext()->getTraits();
    x = (int)ea.getX();
    y = (int)ea.getY();
    if (ea.getMouseYOrientation()
	== osgGA::GUIEventAdapter::Y_INCREASING_UPWARDS)
	y = (int)traits->height - y;
}
}

bool FGManipulator::handle(const osgGA::GUIEventAdapter& ea,
			   osgGA::GUIActionAdapter& us)
{
    int x, y;
    switch (ea.getEventType()) {
    case osgGA::GUIEventAdapter::FRAME:
	if (idleHandler)
	    (*idleHandler)();
	if (drawHandler)
	    (*drawHandler)();
	return true;
    case osgGA::GUIEventAdapter::KEYDOWN:
    case osgGA::GUIEventAdapter::KEYUP:
    {
	int key, modmask;
	handleKey(ea, key, modmask);
	eventToViewport(ea, us, x, y);
	if (keyHandler)
	    (*keyHandler)(key, modmask, x, y);
    }
    return true;
    case osgGA::GUIEventAdapter::PUSH:
    case osgGA::GUIEventAdapter::RELEASE:
    {
	eventToViewport(ea, us, x, y);
	int button = 0;
	switch (ea.getButton()) {
	case osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON:
	    button = 0;
	    break;
	case osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON:
	    button = 1;
	    break;
	case osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON:
	    button = 2;
	    break;
	}
	if (mouseClickHandler)
	    (*mouseClickHandler)(button,
				 (ea.getEventType()
				  == osgGA::GUIEventAdapter::RELEASE), x, y);
	return true;
    }
    case osgGA::GUIEventAdapter::MOVE:
    case osgGA::GUIEventAdapter::DRAG:
	eventToViewport(ea, us, x, y);
	if (mouseMotionHandler)
	    (*mouseMotionHandler)(x, y);
	return true;
    case osgGA::GUIEventAdapter::RESIZE:
	if (windowResizeHandler)
	    (*windowResizeHandler)(ea.getWindowWidth(), ea.getWindowHeight());
	return true;
    default:
	return false;
    }
}
	    
void FGManipulator::handleKey(const osgGA::GUIEventAdapter& ea, int& key,
			      int& modifiers)
{
    key = ea.getKey();
    // XXX Probably other translations are needed too.
    switch (key) {
    case osgGA::GUIEventAdapter::KEY_Escape: key = 0x1b; break;
    case osgGA::GUIEventAdapter::KEY_Return: key = '\n'; break;
    case osgGA::GUIEventAdapter::KEY_Left:     key = PU_KEY_LEFT;      break;
    case osgGA::GUIEventAdapter::KEY_Up:       key = PU_KEY_UP;        break;
    case osgGA::GUIEventAdapter::KEY_Right:    key = PU_KEY_RIGHT;     break;
    case osgGA::GUIEventAdapter::KEY_Down:     key = PU_KEY_DOWN;      break;
    case osgGA::GUIEventAdapter::KEY_Page_Up:   key = PU_KEY_PAGE_UP;   break;
    case osgGA::GUIEventAdapter::KEY_Page_Down: key = PU_KEY_PAGE_DOWN; break;
    case osgGA::GUIEventAdapter::KEY_Home:     key = PU_KEY_HOME;      break;
    case osgGA::GUIEventAdapter::KEY_End:      key = PU_KEY_END;       break;
    case osgGA::GUIEventAdapter::KEY_Insert:   key = PU_KEY_INSERT;    break;
    case osgGA::GUIEventAdapter::KEY_F1:       key = PU_KEY_F1;        break;
    case osgGA::GUIEventAdapter::KEY_F2:       key = PU_KEY_F2;        break;
    case osgGA::GUIEventAdapter::KEY_F3:       key = PU_KEY_F3;        break;
    case osgGA::GUIEventAdapter::KEY_F4:       key = PU_KEY_F4;        break;
    case osgGA::GUIEventAdapter::KEY_F5:       key = PU_KEY_F5;        break;
    case osgGA::GUIEventAdapter::KEY_F6:       key = PU_KEY_F6;        break;
    case osgGA::GUIEventAdapter::KEY_F7:       key = PU_KEY_F7;        break;
    case osgGA::GUIEventAdapter::KEY_F8:       key = PU_KEY_F8;        break;
    case osgGA::GUIEventAdapter::KEY_F9:       key = PU_KEY_F9;        break;
    case osgGA::GUIEventAdapter::KEY_F10:      key = PU_KEY_F10;       break;
    case osgGA::GUIEventAdapter::KEY_F11:      key = PU_KEY_F11;       break;
    case osgGA::GUIEventAdapter::KEY_F12:      key = PU_KEY_F12;       break;
    }
    modifiers = osgToFGModifiers(ea.getModKeyMask());
    currentModifiers = modifiers;
    if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
	modifiers |= KEYMOD_RELEASED;
}

