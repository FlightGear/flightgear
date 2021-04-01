// Copyright (C) 2021 James Turner <james@flightgear.org>
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

#pragma once

#include <QColor>
#include <QQuickImageProvider>

class QQmlEngine;

/**
 * @brief Heper image provider to allow re-colorizing
 * images based on the active style
 * 
 */
class QmlColoredImageProvider : public QQuickImageProvider
{
public:
    QmlColoredImageProvider();

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

    void loadStyleColors(QQmlEngine* engine, int styleTypeId);

    static QmlColoredImageProvider* instance();

private:
    QColor _themeColor, _textColor, _themeContrastColor, _activeColor;
    QColor _destructiveColor;
};
