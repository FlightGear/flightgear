// CocoaMouseCursor.cxx - mouse cursor using Cocoa APIs

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

#include "CocoaMouseCursor.hxx"
#include "MouseCursor.hxx"

#include <Foundation/NSPathUtilities.h>
#include <AppKit/NSImage.h>
#include <AppKit/NSCursor.h>

#include <map>

#include <Main/globals.hxx>
#include <GUI/CocoaHelpers_private.h>

class CocoaMouseCursor::CocoaMouseCursorPrivate
{
public:
    Cursor activeCursorKey;
    
    typedef std::map<Cursor, NSCursor*> CursorMap;
    CursorMap cursors;
};

NSCursor* cocoaCursorForKey(FGMouseCursor::Cursor aKey)
{
    NSImage* img = nil;

    std::string p = globals->get_fg_root().utf8Str();
    NSString* path = [NSString stringWithCString:p.c_str() encoding:NSUTF8StringEncoding];
    path = [path stringByAppendingPathComponent:@"gui"];
    
    switch (aKey) {
    case FGMouseCursor::CURSOR_HAND: return [NSCursor pointingHandCursor];
    case FGMouseCursor::CURSOR_CROSSHAIR: return [NSCursor crosshairCursor];
    case FGMouseCursor::CURSOR_IBEAM: return [NSCursor IBeamCursor];
    case FGMouseCursor::CURSOR_CLOSED_HAND: return [NSCursor closedHandCursor];
            
    // FIXME - use a proper left-right cursor here.
    case FGMouseCursor::CURSOR_LEFT_RIGHT: return [NSCursor resizeLeftRightCursor];
    case FGMouseCursor::CURSOR_UP_DOWN: return [NSCursor resizeUpDownCursor];

    case FGMouseCursor::CURSOR_LEFT_SIDE:
      return [NSCursor resizeLeftCursor];
    case FGMouseCursor::CURSOR_RIGHT_SIDE:
      return [NSCursor resizeRightCursor];
    case FGMouseCursor::CURSOR_TOP_SIDE:
      return [NSCursor resizeUpCursor];
    case FGMouseCursor::CURSOR_BOTTOM_SIDE:
      return [NSCursor resizeDownCursor];

    case FGMouseCursor::CURSOR_TOP_LEFT:
      return [NSCursor _topLeftResizeCursor];
    case FGMouseCursor::CURSOR_TOP_RIGHT:
      return [NSCursor _topRightResizeCursor];
    case FGMouseCursor::CURSOR_BOTTOM_LEFT:
      return [NSCursor _bottomLeftResizeCursor];
    case FGMouseCursor::CURSOR_BOTTOM_RIGHT:
      return [NSCursor _bottomRightResizeCursor];

    case FGMouseCursor::CURSOR_WAIT:
      return [NSCursor _waitCursor];

    case FGMouseCursor::CURSOR_SPIN_CW:
        path = [path stringByAppendingPathComponent:@"cursor-spin-cw.png"];
        img = [[NSImage alloc] initWithContentsOfFile:path];
        return [[NSCursor alloc] initWithImage:img hotSpot:NSMakePoint(16,16)];
            
    case FGMouseCursor::CURSOR_SPIN_CCW:
        path = [path stringByAppendingPathComponent:@"cursor-spin-cw.png"];
        img = [[NSImage alloc] initWithContentsOfFile:path];
        return [[NSCursor alloc] initWithImage:img hotSpot:NSMakePoint(16,16)];
            
    default: return [NSCursor arrowCursor];
    }

}

CocoaMouseCursor::CocoaMouseCursor() :
    d(new CocoaMouseCursorPrivate)
{
    
}

CocoaMouseCursor::~CocoaMouseCursor()
{
    
    
}


void CocoaMouseCursor::setCursor(Cursor aCursor)
{
    if (aCursor == d->activeCursorKey) {
        return;
    }

    m_currentCursor = aCursor;

    CocoaAutoreleasePool pool;   
    d->activeCursorKey = aCursor;
    if (d->cursors.find(aCursor) == d->cursors.end()) {
        d->cursors[aCursor] = cocoaCursorForKey(aCursor);
        [d->cursors[aCursor] retain];
    }
    
    [d->cursors[aCursor] set];
}

void CocoaMouseCursor::setCursorVisible(bool aVis)
{
    CocoaAutoreleasePool pool;
    if (aVis) {
        [NSCursor unhide];
    } else {
        [NSCursor hide];
    }
}

void CocoaMouseCursor::hideCursorUntilMouseMove()
{
    [NSCursor setHiddenUntilMouseMoves:YES];
}

void CocoaMouseCursor::mouseMoved()
{
    // no-op
}
