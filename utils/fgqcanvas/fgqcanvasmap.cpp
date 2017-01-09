//
// Copyright (C) 2017 James Turner  zakalawe@mac.com
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

#include "fgqcanvasmap.h"

#include <QDebug>

#include "localprop.h"
#include "fgcanvaspaintcontext.h"

FGQCanvasMap::FGQCanvasMap(FGCanvasGroup* pr, LocalProp* prop) :
    FGCanvasGroup(pr, prop)
{

}

void FGQCanvasMap::doPaint(FGCanvasPaintContext *context) const
{

    FGCanvasGroup::doPaint(context);
}

bool FGQCanvasMap::onChildAdded(LocalProp *prop)
{
    const QByteArray nm = prop->name();
    if ((nm == "ref-lon") || (nm == "ref-lat") || (nm == "hdg") || (nm == "range")
        || (nm == "screen-range")) {
        connect(prop, &LocalProp::valueChanged, this, &FGQCanvasMap::markProjectionDirty);
        return true;
    }

    if (FGCanvasGroup::onChildAdded(prop)) {
        return true;
    }

    qDebug() << Q_FUNC_INFO << "deal with:" << prop->name();
    return false;
}

void FGQCanvasMap::markProjectionDirty()
{
    _projectionChanged = true;
}
