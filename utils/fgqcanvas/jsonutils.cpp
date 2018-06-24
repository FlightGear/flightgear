//
// Copyright (C) 2018 James Turner  <james@flightgear.org>
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

#include "jsonutils.h"

QJsonArray rectToJsonArray(const QRect& r)
{
    return QJsonArray{r.x(), r.y(), r.width(), r.height()};
}


QRect jsonArrayToRect(QJsonArray a)
{
    if (a.size() < 4) {
        return {};
    }

    return QRect(a[0].toInt(), a[1].toInt(),
            a[2].toInt(), a[3].toInt());
}
