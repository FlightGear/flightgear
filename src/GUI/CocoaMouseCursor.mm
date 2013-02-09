#include "CocoaMouseCursor.hxx"

#include <Cocoa/Cocoa.h>
#include <map>

#include <Main/globals.hxx>

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
    
    NSString* path = [NSString stringWithCString:globals->get_fg_root().c_str()
                                            encoding:NSUTF8StringEncoding];
    path = [path stringByAppendingPathComponent:@"gui"];
    
    switch (aKey) {
    case FGMouseCursor::CURSOR_HAND: return [NSCursor pointingHandCursor];
    case FGMouseCursor::CURSOR_CROSSHAIR: return [NSCursor crosshairCursor];
    case FGMouseCursor::CURSOR_IBEAM: return [NSCursor IBeamCursor];
    
    // FIXME - use a proper left-right cursor here.
    case FGMouseCursor::CURSOR_LEFT_RIGHT: return [NSCursor resizeLeftRightCursor];
            
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
    
    d->activeCursorKey = aCursor;
    if (d->cursors.find(aCursor) == d->cursors.end()) {
        d->cursors[aCursor] = cocoaCursorForKey(aCursor);
        [d->cursors[aCursor] retain];
    }
    
    [d->cursors[aCursor] set];
}

void CocoaMouseCursor::setCursorVisible(bool aVis)
{
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
