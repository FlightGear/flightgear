// Fallback implementation of clipboard access for Nasal. Copy and edit for
// implementing support of other platforms
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

#include "NasalClipboard.hxx"

#include <simgear/debug/logstream.hxx>


/**
 * Provide a basic clipboard whose contents are only available to FlightGear
 * itself
 */
class ClipboardFallback:
  public NasalClipboard
{
  public:

    /**
     * Get clipboard contents as text
     */
    virtual std::string getText(Type type)
    {
      return type == CLIPBOARD ? _clipboard : _selection;
    }

    /**
     * Set clipboard contents as text
     */
    virtual bool setText(const std::string& text, Type type)
    {
      if( type == CLIPBOARD )
        _clipboard = text;
      else
        _selection = text;
      return true;
    }

  protected:

    std::string _clipboard,
                _selection;
};

//------------------------------------------------------------------------------
NasalClipboard::Ptr NasalClipboard::create()
{
  return NasalClipboard::Ptr(new ClipboardFallback);
}
