// CocoaHelpers.mm - C++ implementation of Cocoa/AppKit helpers

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

#include <config.h>

#include "CocoaHelpers.h"
#include "CocoaHelpers_private.h"

// standard
#include <string>
#include <dlfcn.h>

// OS

#include <Foundation/NSAutoreleasePool.h>
#include <Foundation/NSURL.h>
#include <Foundation/NSFileManager.h>
#include <Foundation/NSBundle.h>
#include <Foundation/NSLocale.h>
#include <AppKit/NSAlert.h>

// simgear
#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/commands.hxx>

// flightgear
#include <GUI/MessageBox.hxx>
#include <Main/options.hxx>
#include <Main/locale.hxx>
#include <Main/globals.hxx>

#if defined(HAVE_QT)
#  include <GUI/QtLauncher.hxx>
#endif

NSString* stdStringToCocoa(const std::string& s)
{
    return [NSString stringWithUTF8String:s.c_str()];
}

std::string stdStringFromCocoa(NSString* s)
{
    return std::string([s UTF8String]);
}

NSURL* pathToNSURL(const SGPath& aPath)
{
    return [NSURL fileURLWithPath:stdStringToCocoa(aPath.utf8Str())];
}

SGPath URLToPath(NSURL* url)
{
    if (!url) {
        return SGPath();
    }
    
    return SGPath([[url path] UTF8String]);
}

flightgear::MessageBoxResult cocoaMessageBox(const std::string& msg,
                                             const std::string& text)
{
    CocoaAutoreleasePool pool;
    NSAlert* alert = [NSAlert alertWithMessageText:stdStringToCocoa(msg)
                                     defaultButton:nil /* localized 'ok' */
                                   alternateButton:nil
                                       otherButton:nil
                         informativeTextWithFormat:@" %@",stdStringToCocoa(text)];
    [[alert retain] autorelease];
    [alert runModal];
    return flightgear::MSG_BOX_OK;
}



flightgear::MessageBoxResult cocoaFatalMessage(const std::string& msg,
                                               const std::string& text)
{
    CocoaAutoreleasePool pool;
    NSAlert* alert = [NSAlert alertWithMessageText:stdStringToCocoa(msg)
                                     defaultButton:@"Quit FlightGear"
                                   alternateButton:nil
                                       otherButton:nil
                         informativeTextWithFormat:@"%@", stdStringToCocoa(text)];
    [[alert retain] autorelease];
    [alert runModal];
    return flightgear::MSG_BOX_OK;
}

void cocoaOpenUrl(const std::string& url)
{
  CocoaAutoreleasePool pool;
  NSURL* nsu = [NSURL URLWithString:stdStringToCocoa(url)];
  [[NSWorkspace sharedWorkspace] openURL:nsu];
}

CocoaAutoreleasePool::CocoaAutoreleasePool()
{
    pool = [[NSAutoreleasePool alloc] init];
}

CocoaAutoreleasePool::~CocoaAutoreleasePool()
{
    [pool release];
}

SGPath platformDefaultDataPath()
{
    CocoaAutoreleasePool ap;
    NSFileManager* fm = [NSFileManager defaultManager];
    
    NSURL* appSupportUrl = [fm URLForDirectory:NSApplicationSupportDirectory
                                     inDomain:NSUserDomainMask
                             appropriateForURL:Nil
                                       create:YES
                                         error:nil];
    if (!appSupportUrl) {
        return SGPath();
    }
    
    SGPath appData(URLToPath(appSupportUrl));
    appData.append("FlightGear");
    return appData;
}

namespace flightgear
{
    
SGPath Options::platformDefaultRoot() const
{
    CocoaAutoreleasePool ap;
    
    NSURL* url = [[NSBundle mainBundle] resourceURL];
    SGPath dataDir(URLToPath(url));
    dataDir.append("data");
    return dataDir;
}
    
} // of namespace flightgear

string_list FGLocale::getUserLanguages()
{
    CocoaAutoreleasePool ap;
    string_list result;
    
    for (NSString* lang in [NSLocale preferredLanguages]) {
        result.push_back(stdStringFromCocoa(lang));
    }
    
    return result;
}

void transformToForegroundApp()
{
    ProcessSerialNumber sn = { 0, kCurrentProcess };
    TransformProcessType(&sn,kProcessTransformToForegroundApplication);

    
    [[NSApplication sharedApplication] activateIgnoringOtherApps: YES];
}

@interface FlightGearNSAppDelegate  : NSObject <NSApplicationDelegate>
@end

@implementation FlightGearNSAppDelegate


// it would be lovely to do the following, but the return code doesn't
// seem to work as documented - maybe it's ignored due to newer process
// management
#if 0
- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *)sender
{    
    SGCommandMgr* mgr = SGCommandMgr::instance();
    SGPropertyNode_ptr propArgs(new SGPropertyNode);
    propArgs->setStringValue("dialog-name", "exit");
    mgr->execute("dialog-show", propArgs, globals->get_props());
    
    return NSTerminateCancel;
}
#endif

- (void) applicationWillTerminate:(NSNotification *)notification
{
    SG_LOG(SG_GENERAL, SG_INFO, "macOS quit occuring");
#if defined(HAVE_QT)
    flightgear::shutdownQtApp();
#endif
}

@end

void cocoaRegisterTerminateHandler()
{
    FlightGearNSAppDelegate* delegate = [[FlightGearNSAppDelegate alloc] init];
    
    auto *app = [NSApplication sharedApplication];
    [[NSNotificationCenter defaultCenter]
     addObserver:delegate
     selector:@selector(applicationWillTerminate:)
     name:NSApplicationWillTerminateNotification object:app];
#if 9
    [[NSNotificationCenter defaultCenter]
     addObserver:delegate
     selector:@selector(applicationShouldTerminate:)
     name:NSApplicationWillTerminateNotification object:app];
#endif
}



bool cocoaIsRunningTranslocated()
{
    // #define NSAppKitVersionNumber10_11 1404
    if (floor(NSAppKitVersionNumber) <= 1404) {
        return false;
    }

    void *handle = dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_LAZY);
    if (handle == NULL) {
        return false;
    }

    bool isTranslocated = false;
    using SecIsTranslocatedFunc = Boolean (*)(CFURLRef path, bool *isTranslocated, CFErrorRef * __nullable error);

    SecIsTranslocatedFunc f = reinterpret_cast<SecIsTranslocatedFunc>(dlsym(handle, "SecTranslocateIsTranslocatedURL"));
    if (f != NULL) {
        auto url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
        f(url, &isTranslocated, NULL);

        CFRelease(url);
    }

    dlclose(handle);
    return isTranslocated;
}
