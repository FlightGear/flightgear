// Clipboard access for Nasal
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef NASAL_CLIPOARD_HXX_
#define NASAL_CLIPOARD_HXX_

#include <simgear/nasal/nasal.h>
#include <memory>
#include <string>

class FGNasalSys;
class NasalClipboard
{
  public:

    enum Type
    {
      /// Standard clipboard as supported by nearly all operating systems
      CLIPBOARD,

      /// X11 platforms support also a mode called PRIMARY selection which
      /// contains the current (mouse) selection and can typically be inserted
      /// via a press on the middle mouse button
      PRIMARY
    };

    typedef std::shared_ptr<NasalClipboard> Ptr;

    virtual void update() {}
    virtual std::string getText(Type type = CLIPBOARD) = 0;
    virtual bool setText( const std::string& text,
                          Type type = CLIPBOARD ) = 0;

    /**
     * Sets up the clipboard and puts all the extension functions into a new
     * "clipboard" namespace.
     */
    static void init(FGNasalSys *nasal);

    /**
     * Get clipboard platform specific instance
     */
    static Ptr getInstance();

  protected:

    static Ptr      _clipboard;

    /**
     * Implementation supplied by actual platform implementation
     */
    static Ptr create();

    virtual ~NasalClipboard() = 0;
};

#endif /* NASAL_CLIPOARD_HXX_ */
