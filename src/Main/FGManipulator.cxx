#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <osg/Math>
#include <osgViewer/Viewer>
#include <plib/pu.h>
#include "FGManipulator.hxx"

#if !defined(X_DISPLAY_MISSING)
#define X_DOUBLE_SCROLL_BUG 1
#endif

// The manipulator is responsible for updating a Viewer's camera. It's
// event handling method is also a convenient place to run the the FG
// idle and draw handlers.

FGManipulator::FGManipulator() :
    idleHandler(0), drawHandler(0), windowResizeHandler(0), keyHandler(0),
    mouseClickHandler(0), mouseMotionHandler(0), currentModifiers(0),
    osgModifiers(0), resizable(true), mouseWarped(false),
    scrollButtonPressed(false)
{
    using namespace osgGA;
    
    keyMaskMap[GUIEventAdapter::KEY_Shift_L]
	= GUIEventAdapter::MODKEY_LEFT_SHIFT;
    keyMaskMap[GUIEventAdapter::KEY_Shift_R]
	= GUIEventAdapter::MODKEY_RIGHT_SHIFT;
    keyMaskMap[GUIEventAdapter::KEY_Control_L]
	= GUIEventAdapter::MODKEY_LEFT_CTRL;
    keyMaskMap[GUIEventAdapter::KEY_Control_R]
	= GUIEventAdapter::MODKEY_RIGHT_CTRL;
    keyMaskMap[GUIEventAdapter::KEY_Alt_L] = GUIEventAdapter::MODKEY_LEFT_ALT;
    keyMaskMap[GUIEventAdapter::KEY_Alt_R] = GUIEventAdapter::MODKEY_RIGHT_ALT;
    // We have to implement numlock too.
    numlockKeyMap[GUIEventAdapter::KEY_KP_Insert]  = '0';
    numlockKeyMap[GUIEventAdapter::KEY_KP_End] = '1';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Down] = '2';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Page_Down] = '3';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Left] = '4';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Begin] = '5';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Right] = '6';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Home] = '7';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Up] = '8';
    numlockKeyMap[GUIEventAdapter::KEY_KP_Page_Up] = '9';
}

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

// All the usual translation from window system to FG / plib
static int osgToFGModifiers(int modifiers)
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

void FGManipulator::init(const osgGA::GUIEventAdapter& ea,
			 osgGA::GUIActionAdapter& us)
{
    currentModifiers = osgToFGModifiers(ea.getModKeyMask());
    (void)handle(ea, us);
}

static bool
eventToViewport(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us,
                int& x, int& y)
{
  x = -1;
  y = -1;

  const osgViewer::Viewer* viewer;
  viewer = dynamic_cast<const osgViewer::Viewer*>(&us);
  if (!viewer)
      return false;

  float lx, ly;
  const osg::Camera* camera;
  camera = viewer->getCameraContainingPosition(ea.getX(), ea.getY(), lx, ly);

  if (!fgOSIsMainCamera(camera))
      return false;

  x = int(lx);
  y = int(camera->getViewport()->height() - ly);

  return true;
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
	mouseWarped = false;
	return true;
    case osgGA::GUIEventAdapter::KEYDOWN:
    case osgGA::GUIEventAdapter::KEYUP:
    {
	int key, modmask;
	handleKey(ea, key, modmask);
	eventToViewport(ea, us, x, y);
	if (keyHandler)
	    (*keyHandler)(key, modmask, x, y);
        return true;
    }
    case osgGA::GUIEventAdapter::PUSH:
    case osgGA::GUIEventAdapter::RELEASE:
    {
        bool mainWindow = eventToViewport(ea, us, x, y);
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
				  == osgGA::GUIEventAdapter::RELEASE), x, y, mainWindow, &ea);
	return true;
    }
    case osgGA::GUIEventAdapter::SCROLL:
    {
        bool mainWindow = eventToViewport(ea, us, x, y);
#ifdef X_DOUBLE_SCROLL_BUG
        scrollButtonPressed = !scrollButtonPressed;
        if (!scrollButtonPressed) // Drop the button release event
            return true;
#endif
        int button;
        if (ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP)
            button = 3;
        else
            button = 4;
        if (mouseClickHandler) {
            (*mouseClickHandler)(button, 0, x, y, mainWindow, &ea);
            (*mouseClickHandler)(button, 1, x, y, mainWindow, &ea);
        }
        return true;
    }
    case osgGA::GUIEventAdapter::MOVE:
    case osgGA::GUIEventAdapter::DRAG:
        // If we warped the mouse, then disregard all pointer motion
        // events for this frame. We really want to flush the event
        // queue of mouse events, but don't have the ability to do
        // that with osgViewer.
	if (mouseWarped)
	    return true;
	eventToViewport(ea, us, x, y);
	if (mouseMotionHandler)
	    (*mouseMotionHandler)(x, y);
	return true;
    case osgGA::GUIEventAdapter::RESIZE:
	if (resizable && windowResizeHandler)
	    (*windowResizeHandler)(ea.getWindowWidth(), ea.getWindowHeight());
	return true;
     case osgGA::GUIEventAdapter::CLOSE_WINDOW:
    case osgGA::GUIEventAdapter::QUIT_APPLICATION:
        fgOSExit(0);
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
    case osgGA::GUIEventAdapter::KEY_BackSpace: key = '\b'; break;
    case osgGA::GUIEventAdapter::KEY_Delete:   key = 0x7f; break;
    case osgGA::GUIEventAdapter::KEY_Tab:      key = '\t'; break;
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
    case osgGA::GUIEventAdapter::KEY_KP_Delete: key = '.'; break;
    case osgGA::GUIEventAdapter::KEY_KP_Enter: key = '\r'; break;
    case osgGA::GUIEventAdapter::KEY_KP_Add:   key = '+'; break;
    case osgGA::GUIEventAdapter::KEY_KP_Divide: key = '/'; break;
    case osgGA::GUIEventAdapter::KEY_KP_Multiply: key = '*'; break;
    case osgGA::GUIEventAdapter::KEY_KP_Subtract: key = '-'; break;
    }
    osgGA::GUIEventAdapter::EventType eventType = ea.getEventType();
    std::map<int, int>::iterator numPadIter = numlockKeyMap.find(key);

    if (numPadIter != numlockKeyMap.end()) {
	if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_NUM_LOCK) {
	    key = numPadIter->second;
	}
    } else {
	// Track the modifiers because OSG is currently (2.0) broken
	KeyMaskMap::iterator iter = keyMaskMap.find(key);
	if (iter != keyMaskMap.end()) {
	    int mask = iter->second;
	    if (eventType == osgGA::GUIEventAdapter::KEYUP)
		osgModifiers &= ~mask;
	    else
		osgModifiers |= mask;
	}
    }
    modifiers = osgToFGModifiers(osgModifiers);
    currentModifiers = modifiers;
    if (eventType == osgGA::GUIEventAdapter::KEYUP)
	modifiers |= KEYMOD_RELEASED;

    // Release the letter key, for which the keypress was reported
    if (key >= 0 && key < int(sizeof(release_keys))) {
        if (modifiers & KEYMOD_RELEASED) {
            key = release_keys[key];
        } else {
            release_keys[key] = key;
            if (key >= 1 && key <= 26) {
                release_keys[key + '@'] = key;
                release_keys[key + '`'] = key;
            } else if (key >= 'A' && key <= 'Z') {
                release_keys[key - '@'] = key;
                release_keys[tolower(key)] = key;
            } else if (key >= 'a' && key <= 'z') {
                release_keys[key - '`'] = key;
                release_keys[toupper(key)] = key;
            }
        }
    }
}

