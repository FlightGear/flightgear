// Windows implementation of clipboard access for Nasal
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
#include <windows.h>

/**
 * Windows does only support on clipboard and no selection. We fake also the X11
 * selection buffer - at least inside FlightGear
 */
class ClipboardWindows:
  public NasalClipboard
{
  public:

    /**
     * Get clipboard contents as text
     */
    virtual std::string getText(Type type)
    {
      if( type == CLIPBOARD )
      {
        std::string data;

        if( !OpenClipboard(NULL) )
          return data;

        HANDLE hData = GetClipboardData( CF_TEXT );
        char* buff = (char*)GlobalLock( hData );
        if (buff)
          data = buff;
        GlobalUnlock( hData );
        CloseClipboard();

        return data;
      }
      else
        return _selection;
    }

    /**
     * Set clipboard contents as text
     */
    virtual bool setText(const std::string& text, Type type)
    {
      if( type == CLIPBOARD )
      {
        if( !OpenClipboard(NULL) )
          return false;

        bool ret = true;
        if( !EmptyClipboard() )
          ret = false;
        else if( !text.empty() )
        {
          HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
          if( !hGlob )
            ret = false;
          else
          {
            memcpy(GlobalLock(hGlob), (char*)&text[0], text.size() + 1);
            GlobalUnlock(hGlob);

            if( !SetClipboardData(CF_TEXT, hGlob) )
            {
              GlobalFree(hGlob);
              ret = false;
            }
          }
        }

        CloseClipboard();
        return ret;
      }
      else
      {
        _selection = text;
        return true;
      }
    }

  protected:

    std::string _selection;
};

//------------------------------------------------------------------------------
NasalClipboard::Ptr NasalClipboard::create()
{
  return NasalClipboard::Ptr(new ClipboardWindows);
}
