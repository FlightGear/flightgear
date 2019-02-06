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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "NasalClipboard.hxx"
#include "NasalSys.hxx"
#include <simgear/nasal/cppbind/NasalCallContext.hxx>

#include <cstddef>

/*
 *  Nasal wrappers for setting/getting clipboard text
 */
//------------------------------------------------------------------------------
static NasalClipboard::Type parseType(const nasal::CallContext& ctx, size_t i)
{
  if( ctx.argc > i )
  {
    if( ctx.isNumeric(i) )
    {
      if( ctx.requireArg<int>(i) == NasalClipboard::CLIPBOARD )
        return NasalClipboard::CLIPBOARD;
      if( ctx.requireArg<int>(i) == NasalClipboard::PRIMARY )
        return NasalClipboard::PRIMARY;
    }

    ctx.runtimeError("clipboard: invalid arg "
                     "(expected clipboard.CLIPBOARD or clipboard.SELECTION)");
  }

  return NasalClipboard::CLIPBOARD;
}

//------------------------------------------------------------------------------
static naRef f_setClipboardText(const nasal::CallContext& ctx)
{
  if( ctx.argc < 1 || ctx.argc > 2 )
    ctx.runtimeError("clipboard.setText() expects 1 or 2 arguments: "
                     "text, [, type = clipboard.CLIPBOARD]");

  return
    naNum
    (
      NasalClipboard::getInstance()->setText( ctx.requireArg<std::string>(0),
                                              parseType(ctx, 1) )
    );
}

//------------------------------------------------------------------------------
static naRef f_getClipboardText(const nasal::CallContext& ctx)
{
  if( ctx.argc > 1 )
    ctx.runtimeError("clipboard.getText() accepts max 1 arg: "
                     "[type = clipboard.CLIPBOARD]");

  return ctx.to_nasal
  (
    NasalClipboard::getInstance()->getText(parseType(ctx, 0))
  );
}

//------------------------------------------------------------------------------
NasalClipboard::Ptr NasalClipboard::_clipboard;

//------------------------------------------------------------------------------
NasalClipboard::~NasalClipboard()
{

}

//------------------------------------------------------------------------------
void NasalClipboard::init(FGNasalSys *nasal)
{
  _clipboard = create();

  nasal::Hash clipboard = nasal->getGlobals().createHash("clipboard");

  clipboard.set("setText", f_setClipboardText);
  clipboard.set("getText", f_getClipboardText);
  clipboard.set("CLIPBOARD", NasalClipboard::CLIPBOARD);
  clipboard.set("SELECTION", NasalClipboard::PRIMARY);
}

//------------------------------------------------------------------------------
NasalClipboard::Ptr NasalClipboard::getInstance()
{
  return _clipboard;
}
