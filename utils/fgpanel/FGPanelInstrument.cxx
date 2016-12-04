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

#include "FGPanelInstrument.hxx"

FGPanelInstrument::FGPanelInstrument () {
  setPosition (0, 0);
  setSize (0, 0);
}

FGPanelInstrument::FGPanelInstrument (const int x, const int y, const int w, const int h) {
  setPosition (x, y);
  setSize (w, h);
}

FGPanelInstrument::~FGPanelInstrument () {
}

void
FGPanelInstrument::setPosition (const int x, const int y) {
  m_x = x;
  m_y = y;
}

void
FGPanelInstrument::setSize (const int w, const int h) {
  m_w = w;
  m_h = h;
}

int
FGPanelInstrument::getXPos () const {
  return m_x;
}

int
FGPanelInstrument::getYPos () const {
  return m_y;
}

int
FGPanelInstrument::getWidth () const {
  return m_w;
}

int
FGPanelInstrument::getHeight () const {
  return m_h;
}
