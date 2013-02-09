
// MouseCursor.hxx - abstract inteface for  mouse cursor control

#ifndef FG_GUI_MOUSE_CURSOR_HXX
#define FG_GUI_MOUSE_CURSOR_HXX 1

class SGPropertyNode;

class FGMouseCursor
{
public:
    static FGMouseCursor* instance();

    virtual void setAutoHideTimeMsec(unsigned int aMsec);
  
    enum Cursor
    {
        CURSOR_ARROW,
        CURSOR_HAND,
        CURSOR_CROSSHAIR,
        CURSOR_IBEAM, ///< for editing text
        CURSOR_IN_OUT, ///< arrow pointing into / out of the screen
        CURSOR_LEFT_RIGHT,
        CURSOR_UP_DOWN,
        CURSOR_SPIN_CW,
        CURSOR_SPIN_CCW
    };
  
    virtual void setCursor(Cursor aCursor) = 0;
    
    virtual void setCursorVisible(bool aVis) = 0;
    
    virtual void hideCursorUntilMouseMove() = 0;
    
    virtual void mouseMoved() = 0;
    
    static Cursor cursorFromString(const char* str);

protected:
    FGMouseCursor();
    
    bool setCursorCommand(const SGPropertyNode* arg);
    
    unsigned int mAutoHideTimeMsec;
};

#endif // FG_GUI_MOUSE_CURSOR_HXX
