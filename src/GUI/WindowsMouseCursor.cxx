// WindowsMouseCursor.cxx - mouse cursor using Windows APIs

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

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#include "WindowsMouseCursor.hxx"

#include "windows.h"

#include <map>

#include <Main/globals.hxx>

class WindowsMouseCursor::WindowsMouseCursorPrivate
{
public:
    Cursor activeCursorKey;
    bool hideUntilMove;

    typedef std::map<Cursor, HCURSOR> CursorMap;
    CursorMap cursors;
};

HCURSOR windowsCursorForKey(FGMouseCursor::Cursor aKey)
{
    switch (aKey) {
    case FGMouseCursor::CURSOR_HAND: return LoadCursor(NULL, IDC_HAND);
    case FGMouseCursor::CURSOR_CROSSHAIR: return LoadCursor(NULL, IDC_CROSS);
    case FGMouseCursor::CURSOR_IBEAM: return LoadCursor(NULL, IDC_IBEAM);
	case FGMouseCursor::CURSOR_LEFT_RIGHT: return LoadCursor( NULL, IDC_SIZEWE );
    
#if 0       
    case FGMouseCursor::CURSOR_SPIN_CW:
        path = [path stringByAppendingPathComponent:@"cursor-spin-cw.png"];
        img = [[NSImage alloc] initWithContentsOfFile:path];
        return [[NSCursor alloc] initWithImage:img hotSpot:NSMakePoint(16,16)];
            
    case FGMouseCursor::CURSOR_SPIN_CCW:
        path = [path stringByAppendingPathComponent:@"cursor-spin-cw.png"];
        img = [[NSImage alloc] initWithContentsOfFile:path];
        return [[NSCursor alloc] initWithImage:img hotSpot:NSMakePoint(16,16)];
#endif       
    default: return LoadCursor(NULL, IDC_ARROW);
    }
    
}

WindowsMouseCursor::WindowsMouseCursor() :
    d(new WindowsMouseCursorPrivate)
{
    d->hideUntilMove = false;
	d->activeCursorKey = CURSOR_ARROW;
}

WindowsMouseCursor::~WindowsMouseCursor()
{
    
    
}


void WindowsMouseCursor::setCursor(Cursor aCursor)
{
    if (aCursor == d->activeCursorKey) {
        return;
    }
    
    d->activeCursorKey = aCursor;
    if (d->cursors.find(aCursor) == d->cursors.end()) {
        d->cursors[aCursor] = windowsCursorForKey(aCursor);
    }

    SetCursor(d->cursors[aCursor]);
}

void WindowsMouseCursor::setCursorVisible(bool aVis)
{
    d->hideUntilMove = false; // cancel this
    ShowCursor(aVis);
}

void WindowsMouseCursor::hideCursorUntilMouseMove()
{
    if (d->hideUntilMove) {
        return;
    }
    
    d->hideUntilMove = true;
    ShowCursor(false);
}

void WindowsMouseCursor::mouseMoved()
{
    if (d->hideUntilMove) {
        d->hideUntilMove = false;
        ShowCursor(true);
    }
}
