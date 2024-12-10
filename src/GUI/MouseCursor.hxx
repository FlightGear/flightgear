// MouseCursor.hxx - abstract inteface for  mouse cursor control

// Copyright (C) 2013 James Turner <zakalawe@mac.com>
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
//


#ifndef FG_GUI_MOUSE_CURSOR_HXX
#define FG_GUI_MOUSE_CURSOR_HXX 1

class SGPropertyNode;

class FGMouseCursor
{
public:
    static FGMouseCursor* instance();

    virtual void setAutoHideTimeMsec(unsigned int aMsec);

    enum Cursor {
        CURSOR_NONE = 0,
        CURSOR_ARROW,
        CURSOR_HAND, ///< the browser 'link' cursor
        CURSOR_CLOSED_HAND,
        CURSOR_CROSSHAIR,
        CURSOR_IBEAM,  ///< for editing text
        CURSOR_IN_OUT, ///< arrow pointing into / out of the screen
        CURSOR_LEFT_RIGHT,
        CURSOR_UP_DOWN,
        CURSOR_LEFT_SIDE,
        CURSOR_RIGHT_SIDE,
        CURSOR_TOP_SIDE,
        CURSOR_BOTTOM_SIDE,
        CURSOR_TOP_LEFT,
        CURSOR_TOP_RIGHT,
        CURSOR_BOTTOM_LEFT,
        CURSOR_BOTTOM_RIGHT,
        CURSOR_SPIN_CW,
        CURSOR_SPIN_CCW,
        CURSOR_WAIT
    };

    virtual void setCursor(Cursor aCursor) = 0;
    
    virtual void setCursorVisible(bool aVis) = 0;
    
    virtual void hideCursorUntilMouseMove() = 0;
    
    virtual void mouseMoved() = 0;
    
    static Cursor cursorFromString(const char* str);

    virtual Cursor getCursor() const;

protected:
    FGMouseCursor();
    
    bool setCursorCommand(const SGPropertyNode* arg, SGPropertyNode*);
    
    unsigned int mAutoHideTimeMsec;

    Cursor m_currentCursor = CURSOR_ARROW;
};

#endif // FG_GUI_MOUSE_CURSOR_HXX
