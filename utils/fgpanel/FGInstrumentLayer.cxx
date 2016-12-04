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

#include <math.h>
#include <simgear/props/props.hxx>

#include "GL_utils.hxx"
#include "FGInstrumentLayer.hxx"

FGInstrumentLayer::FGInstrumentLayer (const int w, const int h) :
  m_w (w), m_h (h) {
}

FGInstrumentLayer::~FGInstrumentLayer () {
  for (transformation_list::iterator it = m_transformations.begin ();
       it != m_transformations.end ();
       ++it) {
    delete *it;
    *it = 0;
  }
}

void
FGInstrumentLayer::transform () const {
  for (transformation_list::const_iterator it = m_transformations.begin ();
       it != m_transformations.end ();
       ++it) {
    FGPanelTransformation *t = *it;
    if (t->test ()) {
      float val (t->node == 0 ? 0.0 : t->node->getFloatValue ());
      if (t->has_mod) {
          val = fmod (val, t->mod);
      }
      if (val < t->min) {
        val = t->min;
      } else if (val > t->max) {
        val = t->max;
      }

      if (t->table == 0) {
        val = val * t->factor + t->offset;
      } else {
        val = t->table->interpolate (val) * t->factor + t->offset;
      }

      switch (t->type) {
      case FGPanelTransformation::XSHIFT:
        GL_utils::instance ().glTranslatef (val, 0.0, 0.0);
        break;
      case FGPanelTransformation::YSHIFT:
        GL_utils::instance ().glTranslatef (0.0, val, 0.0);
        break;
      case FGPanelTransformation::ROTATION:
        GL_utils::instance ().glRotatef (-val, 0.0, 0.0, 1.0);
        break;
      }
    }
  }
}

int
FGInstrumentLayer::getWidth () const {
  return m_w;
}

int
FGInstrumentLayer::getHeight () const {
  return m_h;
}

void
FGInstrumentLayer::setWidth (const int w) {
  m_w = w;
}

void
FGInstrumentLayer::setHeight (const int h) {
  m_h = h;
}

void
FGInstrumentLayer::addTransformation (FGPanelTransformation * const transformation) {
  m_transformations.push_back (transformation);
}
