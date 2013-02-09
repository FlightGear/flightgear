#ifndef FG_GUI_COCOA_MOUSE_CURSOR_HXX
#define FG_GUI_COCOA_MOUSE_CURSOR_HXX

#include <memory> // for auto_ptr

#include "MouseCursor.hxx"

class CocoaMouseCursor : public FGMouseCursor
{
public:
    CocoaMouseCursor();
    virtual ~CocoaMouseCursor();
    
    virtual void setCursor(Cursor aCursor);
    
    virtual void setCursorVisible(bool aVis);
    
    virtual void hideCursorUntilMouseMove();
    
    virtual void mouseMoved();

private:
    class CocoaMouseCursorPrivate;
    std::auto_ptr<CocoaMouseCursorPrivate> d;
};


#endif
