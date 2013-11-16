// CocoaHelpers_private.h - common Objective-C/Cocoa helper functions

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


#ifndef FG_GUI_COCOA_HELPERS_PRIVATE_H
#define FG_GUI_COCOA_HELPERS_PRIVATE_H

#include <string>

// forward decls
class SGPath;

@class NSString;
@class NSURL;
@class NSAutoreleasePool;

NSString* stdStringToCocoa(const std::string& s);
NSURL* pathToNSURL(const SGPath& aPath);
SGPath URLToPath(NSURL* url);
std::string stdStringFromCocoa(NSString* s);

class CocoaAutoreleasePool
{
public:
    CocoaAutoreleasePool();

    ~CocoaAutoreleasePool();

private:
    NSAutoreleasePool* pool;
};
  
#endif