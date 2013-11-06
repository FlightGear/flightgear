#ifndef FG_GUI_COCOA_AUTORELEASE_POOL_HXX
#define FG_GUI_COCOA_AUTORELEASE_POOL_HXX

#include <Foundation/NSAutoreleasePool.h>

class CocoaAutoreleasePool
{
public:
    CocoaAutoreleasePool()
    {
      pool = [[NSAutoreleasePool alloc] init];
    }

    ~CocoaAutoreleasePool()
    {
      [pool release];
    }

private:
    NSAutoreleasePool* pool;
};
  
  
#endif // of FG_GUI_COCOA_AUTORELEASE_POOL_HXX