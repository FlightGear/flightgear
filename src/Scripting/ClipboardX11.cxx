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

/*
 * See the following links for more information on X11 clipboard:
 *
 * http://www.jwz.org/doc/x-cut-and-paste.html
 * http://michael.toren.net/mirrors/doc/X-copy+paste.txt
 * https://github.com/kfish/xsel/blob/master/xsel.c
 */

#include "NasalClipboard.hxx"

#include <simgear/debug/logstream.hxx>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <cassert>

class ClipboardX11:
  public NasalClipboard
{
  public:
    ClipboardX11():
      // REVIEW: Memory Leak - 55,397 (104 direct, 55,293 indirect) bytes in 1 blocks are definitely lost
      _display( XOpenDisplay(NULL) ),
      _window( XCreateSimpleWindow(
        _display,
        DefaultRootWindow(_display),
        0, 0, 1, 1, // dummy dimensions -> window will never be mapped
        0,
        0, 0
      ) ),
      _atom_targets( XInternAtom(_display, "TARGETS", False) ),
      _atom_text( XInternAtom(_display, "TEXT", False) ),
      _atom_utf8( XInternAtom(_display, "UTF8_STRING", False) ),
      _atom_primary( XInternAtom(_display, "PRIMARY", False) ),
      _atom_clipboard( XInternAtom(_display, "CLIPBOARD", False) )
    {
      assert(_display);
    }

    virtual ~ClipboardX11()
    {
      // Ensure we get rid of any selection ownership
      if( XGetSelectionOwner(_display, _atom_primary) )
        XSetSelectionOwner(_display, _atom_primary, None, CurrentTime);
      if( XGetSelectionOwner(_display, _atom_clipboard) )
        XSetSelectionOwner(_display, _atom_clipboard, None, CurrentTime);
    }

    /**
     * We need to run an event queue to check for selection request
     */
    virtual void update()
    {
      while( XPending(_display) )
      {
        XEvent event;
        XNextEvent(_display, &event);
        handleEvent(event);
      }
    }

    /**
     * Get clipboard contents as text
     */
    virtual std::string getText(Type type)
    {
      Atom atom_type = typeToAtom(type);

      //Request a list of possible conversions
      XConvertSelection( _display, atom_type, _atom_targets, atom_type,
                         _window, CurrentTime );

      Atom requested_type = None;
      bool sent_request = false;

      for(int cnt = 0; cnt < 5;)
      {
        XEvent event;
        XNextEvent(_display, &event);

        if( event.type != SelectionNotify )
        {
          handleEvent(event);
          continue;
        }

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

      return std::string();
    }

    /**
     * Set clipboard contents as text
     */
    virtual bool setText(const std::string& text, Type type)
    {
      Atom atom_type = typeToAtom(type);
      XSetSelectionOwner(_display, atom_type, _window, CurrentTime);
      if( XGetSelectionOwner(_display, atom_type) != _window )
      {
        SG_LOG
        (
          SG_NASAL,
          SG_ALERT,
          "ClipboardX11::setText: failed to get selection owner!"
        );
        return false;
      }

      // We need to store the text for sending it to another application upon
      // request
      if( type == CLIPBOARD )
        _clipboard = text;
      else
        _selection = text;

      return true;
    }

  protected:

    Display    *_display;
    Window      _window;
    Atom        _atom_targets,
                _atom_text,
                _atom_utf8,
                _atom_primary,
                _atom_clipboard;

    std::string _clipboard,
                _selection;

    void handleEvent(const XEvent& event)
    {
      switch( event.type )
      {
        case SelectionRequest:
          handleSelectionRequest(event.xselectionrequest);
          break;
        case SelectionClear:
          if( event.xselectionclear.selection == _atom_clipboard )
            _clipboard.clear();
          else
            _selection.clear();
          break;
        default:
          SG_LOG
          (
            SG_NASAL,
            SG_WARN,
            "ClipboardX11: unexpected XEvent: " << event.type
          );
          break;
      }
    }

    void handleSelectionRequest(const XSelectionRequestEvent& sel_req)
    {
      SG_LOG
      (
        SG_NASAL,
        SG_DEBUG,
        "ClipboardX11: handle selection request: "
                      "selection=" << getAtomName(sel_req.selection) << ", "
                      "target=" << getAtomName(sel_req.target)
      );

      const std::string& buf = sel_req.selection == _atom_clipboard
                             ? _clipboard
                             : _selection;

      // Prepare response to notify whether we have written to the property or
      // are unable to do the conversion
      XSelectionEvent response;
      response.type = SelectionNotify;
      response.display = sel_req.display;
      response.requestor = sel_req.requestor;
      response.selection = sel_req.selection;
      response.target = sel_req.target;
      response.property = sel_req.property;
      response.time = sel_req.time;

      if( sel_req.target == _atom_targets )
      {
        static Atom supported[] = {
          XA_STRING,
          _atom_text,
          _atom_utf8
        };

        changeProperty
        (
          sel_req.requestor,
          sel_req.property,
          sel_req.target,
          &supported,
          sizeof(supported)
        );
      }
      else if(    sel_req.target == XA_STRING
               || sel_req.target == _atom_text
               || sel_req.target == _atom_utf8 )
      {
        changeProperty
        (
          sel_req.requestor,
          sel_req.property,
          sel_req.target,
          buf.data(),
          buf.size()
        );
      }
      else
      {
        // Notify requestor that we are unable to perform requested conversion
        response.property = None;
      }

      XSendEvent(_display, sel_req.requestor, False, 0, (XEvent*)&response);
    }

    void changeProperty( Window w,
                         Atom prop,
                         Atom type,
                         const void *data,
                         size_t size )
    {
      XChangeProperty
      (
        _display, w, prop, type, 8, PropModeReplace,
        static_cast<const unsigned char*>(data), size
      );
    }

    struct Property
    {
      unsigned char *data;
      int format;
      unsigned long num_items;
      Atom type;
    };

    // Get all data from a property
    Property readProperty(Window w, Atom property)
    {
      Atom actual_type;
      int actual_format;
      unsigned long nitems;
      unsigned long bytes_after;
      unsigned char *ret = 0;

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

    std::string getAtomName(Atom atom) const
    {
      return atom == None ? "None" : XGetAtomName(_display, atom);
    }

    Atom typeToAtom(Type type) const
    {
      return type == CLIPBOARD ? _atom_clipboard : _atom_primary;
    }

};

//------------------------------------------------------------------------------
NasalClipboard::Ptr NasalClipboard::create()
{
  return NasalClipboard::Ptr(new ClipboardX11);
}
