//
//  Written by David Megginson, started January 2000.
//  Adopted for standalone fgpanel application by Torsten Dreyer, August 2009
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef FGPANELINSTRUMENT_HXX
#define FGPANELINSTRUMENT_HXX

#include <simgear/props/condition.hxx>

using namespace std;

/**
 * Abstract base class for a panel instrument.
 *
 * A panel instrument consists of zero or more actions, associated
 * with mouse clicks in rectangular areas.  Currently, the only
 * concrete class derived from this is FGLayeredInstrument, but others
 * may show up in the future (some complex instruments could be
 * entirely hand-coded, for example).
 */
class FGPanelInstrument : public SGConditional {
public:
  FGPanelInstrument ();
  FGPanelInstrument (const int x, const int y, const int w, const int h);
  virtual ~FGPanelInstrument ();

  virtual void draw () = 0;

  virtual void setPosition (const int x, const int y);
  virtual void setSize (const int w, const int h);

  virtual int getXPos () const;
  virtual int getYPos () const;
  virtual int getWidth () const;
  virtual int getHeight () const;

private:
  int m_x, m_y, m_w, m_h;
};

#endif
