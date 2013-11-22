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

#include <boost/algorithm/string/case_conv.hpp>
#include <cstddef>

/*
 *  Nasal wrappers for setting/getting clipboard text
 */
//------------------------------------------------------------------------------
static NasalClipboard::Type parseType(naContext c, int argc, naRef* args, int i)
{
  if( argc > i )
  {
    if( naIsNum(args[i]) )
    {
      if( static_cast<int>(args[i].num) == NasalClipboard::CLIPBOARD )
        return NasalClipboard::CLIPBOARD;
      if( static_cast<int>(args[i].num) == NasalClipboard::PRIMARY )
        return NasalClipboard::PRIMARY;
    }

    naRuntimeError
    (
      c,
      "clipboard: invalid arg "
      "(expected clipboard.CLIPBOARD or clipboard.SELECTION)"
    );
  }

  return NasalClipboard::CLIPBOARD;
}

//------------------------------------------------------------------------------
static naRef f_setClipboardText(naContext c, naRef me, int argc, naRef* args)
{
  if( argc < 1 || argc > 2 )
    naRuntimeError( c, "clipboard.setText() expects 1 or 2 arguments: "
                       "text, [, type = clipboard.CLIPBOARD]" );

  if( !naIsString(args[0]) )
    naRuntimeError(c, "clipboard.setText() invalid arg (arg 0 not a string)");

  return
    naNum
    (
      NasalClipboard::getInstance()->setText( naStr_data(args[0]),
                                              parseType(c, argc, args, 1) )
    );
}

//------------------------------------------------------------------------------
static naRef f_getClipboardText(naContext c, naRef me, int argc, naRef* args)
{
  if( argc > 1 )
    naRuntimeError(c, "clipboard.getText() accepts max 1 arg: "
                      "[type = clipboard.CLIPBOARD]" );

  const std::string& text =
    NasalClipboard::getInstance()->getText(parseType(c, argc, args, 0));

  // TODO create some nasal helper functions (eg. stringToNasal)
  //      some functions are available spread over different files (eg.
  //      NasalPositioned.cxx)
  return naStr_fromdata(naNewString(c), text.c_str(), text.length());
}

//------------------------------------------------------------------------------
// Table of extension functions
static struct {const char* name; naCFunction func; } funcs[] = {
  { "setText", f_setClipboardText },
  { "getText", f_getClipboardText }
};

// Table of extension symbols
static struct {const char* name; naRef val; } symbols[] = {
  { "CLIPBOARD", naNum(NasalClipboard::CLIPBOARD) },
  { "SELECTION", naNum(NasalClipboard::PRIMARY) }
};

//------------------------------------------------------------------------------
NasalClipboard::Ptr NasalClipboard::_clipboard;
naRef NasalClipboard::_clipboard_hash;

//------------------------------------------------------------------------------
NasalClipboard::~NasalClipboard()
{

}

//------------------------------------------------------------------------------
void NasalClipboard::init(FGNasalSys *nasal)
{
    naContext ctx = naNewContext();
    
  _clipboard = create();
  _clipboard_hash = naNewHash(ctx);

  nasal->globalsSet("clipboard", _clipboard_hash);

  for( size_t i = 0; i < sizeof(funcs)/sizeof(funcs[0]); ++i )
  {
    nasal->hashset
    (
      _clipboard_hash,
      funcs[i].name,
      naNewFunc(ctx, naNewCCode(ctx, funcs[i].func))
    );

    SG_LOG(SG_NASAL, SG_DEBUG, "Adding clipboard function: " << funcs[i].name);
  }

  for( size_t i = 0; i < sizeof(symbols)/sizeof(symbols[0]); ++i )
  {
    nasal->hashset(_clipboard_hash, symbols[i].name, symbols[i].val);

    SG_LOG(SG_NASAL, SG_DEBUG, "Adding clipboard symbol: " << symbols[i].name);
  }
    
    naFreeContext(ctx);
}

//------------------------------------------------------------------------------
NasalClipboard::Ptr NasalClipboard::getInstance()
{
  return _clipboard;
}
