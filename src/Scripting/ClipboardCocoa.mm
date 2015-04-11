// Cocoa implementation of clipboard access for Nasal
//
// Copyright (C) 2012  James Turner <zakalawe@mac.com>
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



#include "NasalClipboard.hxx"

#include <simgear/debug/logstream.hxx>

#include <AppKit/NSPasteboard.h>
#include <Foundation/NSArray.h>

#include <GUI/CocoaHelpers_private.h>

/**
 */
class ClipboardCocoa: public NasalClipboard
{
  public:

    /**
     * Get clipboard contents as text
     */
    virtual std::string getText(Type type)
    {
      CocoaAutoreleasePool pool;
      
      if( type == CLIPBOARD )
      {
       NSPasteboard* pboard = [NSPasteboard generalPasteboard];
       #if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
         NSString* nstext = [pboard stringForType:NSStringPboardType];
       #else // > 10.5
         NSString* nstext = [pboard stringForType:NSPasteboardTypeString];
       #endif // MAC_OS_X_VERSION_MIN_REQUIRED
       return stdStringFromCocoa(nstext);
      }
      
      return "";
    }

    /**
     * Set clipboard contents as text
     */
    virtual bool setText(const std::string& text, Type type)
    {
      CocoaAutoreleasePool pool;
      
      if( type == CLIPBOARD )
      {
        NSPasteboard* pboard = [NSPasteboard generalPasteboard];
        NSString* nstext = stdStringToCocoa(text);
        #if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
          NSString* type = NSStringPboardType;
          NSArray* types = [NSArray arrayWithObjects: type, nil];
          [pboard declareTypes:types owner:nil];
          [pboard setString:nstext forType: NSStringPboardType];
        #else // > 10.5
          NSString* type = NSPasteboardTypeString;
          [pboard clearContents];
          [pboard setString:nstext forType:NSPasteboardTypeString];
        #endif // MAC_OS_X_VERSION_MIN_REQUIRED
        return true;
      }
      
      return false;
    }

  protected:

    std::string _selection;
};

//------------------------------------------------------------------------------
NasalClipboard::Ptr NasalClipboard::create()
{
  return NasalClipboard::Ptr(new ClipboardCocoa);
}
