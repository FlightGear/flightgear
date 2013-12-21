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

#include "CocoaHelpers.h"
#include "CocoaHelpers_private.h"

// standard
#include <string>

// OS
#include <Cocoa/Cocoa.h>
#include <Foundation/NSAutoreleasePool.h>

// simgear
#include <simgear/misc/sg_path.hxx>

// flightgear
#include <GUI/MessageBox.hxx>
#include <Main/options.hxx>
#include <Main/locale.hxx>

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
    return [NSURL fileURLWithPath:stdStringToCocoa(aPath.str())];
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
                         informativeTextWithFormat:@"%@",stdStringToCocoa(text)];
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
    
std::string Options::platformDefaultRoot() const
{
    CocoaAutoreleasePool ap;
    
    NSURL* url = [[NSBundle mainBundle] resourceURL];
    SGPath dataDir(URLToPath(url));
    dataDir.append("data");
    return dataDir.str();
}
    
} // of namespace flightgear

string_list FGLocale::getUserLanguage()
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
