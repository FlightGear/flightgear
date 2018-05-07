// QmlNavCacheWrapper.hxx - Expose NavData to Qml
//
// Written by James Turner, started April 2018.
//
// Copyright (C) 2018 James Turner <james@flightgear.org>
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

#ifndef QMLNAVCACHEWRAPPER_HXX
#define QMLNAVCACHEWRAPPER_HXX

#include <QObject>

/**
 * @brief QmlNavCacheWrapper wraps and exposes NavData to Qml as singleton
 * service
 */
class QmlNavCacheWrapper : public QObject
{
    Q_OBJECT
public:

    QmlNavCacheWrapper();
};

#endif // QMLNAVCACHEWRAPPER_HXX
