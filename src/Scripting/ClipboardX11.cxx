// X11 implementation of clipboard access for Nasal
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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

class ClipboardX11:
  public NasalClipboard
{
  public:
    ClipboardX11():
      _display( XOpenDisplay(NULL) ),
      _window( XCreateSimpleWindow(
        _display,
        DefaultRootWindow(_display),
        0, 0, 1, 1, // dummy dimensions -> window will never be mapped
        0,
        0, 0
      ) ),
      _atom_targets( XInternAtom(_display, "TARGETS", False) ),
      _atom_primary( XInternAtom(_display, "PRIMARY", False) ),
      _atom_clipboard( XInternAtom(_display, "CLIPBOARD", False) )
    {
      assert(_display);
      assert(_atom_targets != None);
      assert(_atom_primary != None);
      assert(_atom_clipboard != None);
    }

    /**
     * Get clipboard contents as text
     */
    virtual std::string getText(Type type)
    {
      Atom atom_type = (type == CLIPBOARD ? _atom_clipboard : _atom_primary);

      //Request a list of possible conversions
      XConvertSelection( _display, atom_type, _atom_targets, atom_type,
                         _window, CurrentTime );
      XFlush(_display);

      Atom requested_type = None;
      bool sent_request = false;

      for(int cnt = 0; cnt < 5;)
      {
        XEvent event;
        XNextEvent(_display, &event);

        if( event.type == SelectionNotify )
        {
          Atom target = event.xselection.target;
          if(event.xselection.property == None)
          {
            if( target == _atom_targets )
              // If TARGETS can not be converted no selection is available
              break;

            SG_LOG
            (
              SG_NASAL,
              SG_WARN,
              "ClipboardX11::getText: Conversion failed: "
                                     "target=" << getAtomName(target)
            );
            break;
          }
          else
          {
            //If we're being given a list of targets (possible conversions)
            if(target == _atom_targets && !sent_request)
            {
              sent_request = true;
              requested_type = XA_STRING; // TODO select other type
              XConvertSelection( _display, atom_type, requested_type, atom_type,
                                 _window, CurrentTime );
            }
            else if(target == requested_type)
            {
              Property prop = readProperty(_window, atom_type);
              if( prop.format != 8 )
              {
                SG_LOG
                (
                  SG_NASAL,
                  SG_WARN,
                  "ClipboardX11::getText: can only handle 8-bit data (is "
                                       << prop.format << "-bit) -> retry "
                                       << cnt++
                );
                XFree(prop.data);
                continue;
              }

              std::string result((const char*)prop.data, prop.num_items);
              XFree(prop.data);

              return result;
            }
            else
            {
              SG_LOG
              (
                SG_NASAL,
                SG_WARN,
                "ClipboardX11::getText: wrong target: " << getAtomName(target)
              );
              break;
            }
          }
        }
        else
        {
          SG_LOG
          (
            SG_NASAL,
            SG_WARN,
            "ClipboardX11::getText: unexpected XEvent: " << event.type
          );
          break;
        }
      }

      return std::string();
    }

    /**
     * Set clipboard contents as text
     */
    virtual bool setText(const std::string& text, Type type)
    {
      SG_LOG
      (
        SG_NASAL,
        SG_ALERT,
        "ClipboardX11::setText: not yet implemented!"
      );
      return false;
    }

  protected:

    Display    *_display;
    Window      _window;
    Atom        _atom_targets,
                _atom_primary,
                _atom_clipboard;

    struct Property
    {
      unsigned char *data;
      int format, num_items;
      Atom type;
    };

    // Get all data from a property
    Property readProperty(Window w, Atom property)
    {
      Atom actual_type;
      int actual_format;
      unsigned long nitems;
      unsigned long bytes_after;
      unsigned char *ret=0;

      int read_bytes = 1024;

      //Keep trying to read the property until there are no
      //bytes unread.
      do
      {
        if( ret )
          XFree(ret);

        XGetWindowProperty
        (
          _display, w, property, 0, read_bytes, False, AnyPropertyType,
          &actual_type, &actual_format, &nitems, &bytes_after,
          &ret
        );

        read_bytes *= 2;
      } while( bytes_after );

      Property p = {ret, actual_format, nitems, actual_type};
      return p;
    }

    std::string getAtomName(Atom atom)
    {
      return atom == None ? "None" : XGetAtomName(_display, atom);
    }

};

//------------------------------------------------------------------------------
NasalClipboard::Ptr NasalClipboard::create()
{
  return NasalClipboard::Ptr(new ClipboardX11);
}
