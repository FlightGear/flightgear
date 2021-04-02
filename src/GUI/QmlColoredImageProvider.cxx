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

#include "QmlColoredImageProvider.hxx"

#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>

static QmlColoredImageProvider* static_instance = nullptr;

QmlColoredImageProvider::QmlColoredImageProvider() : QQuickImageProvider(QQmlImageProviderBase::Image)
{
    static_instance = this;
}

void QmlColoredImageProvider::loadStyleColors(QQmlEngine* engine, int styleTypeId)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    QJSValue styleValue = engine->singletonInstance<QJSValue>(styleTypeId);
    if (styleValue.isNull() || !styleValue.isQObject()) {
        qWarning() << "Unable to load Style object";
        return;
    }

    QObject* styleObject = styleValue.toQObject();
#else
    // ugly version for Qt < 5.12 : parse and instantiate a dummy object to let
    // us access the Style singleton
    QQmlComponent comp(engine);
    comp.setData(R"(
               import QtQuick 2.0
               import FlightGear 1.0
               QtObject {
                 readonly property var styleObject: Style
               }
               )",
                 {});
    if (comp.isError()) {
        qWarning() << Q_FUNC_INFO << "Failed to create style accessor component" << comp.errors();
        return;
    }

    auto item = comp.create();
    if (!item) {
        qWarning() << Q_FUNC_INFO << "Failed to create component instance";
        return;
    }

    QObject* styleObject = item->property("styleObject").value<QObject*>();
    item->deleteLater();
#endif

    _themeColor = QColor{styleObject->property("themeColor").toString()};
    _textColor = QColor{styleObject->property("baseTextColor").toString()};
    _themeContrastColor = QColor{styleObject->property("themeContrastTextColor").toString()};
    _activeColor = QColor{styleObject->property("activeColor").toString()};
    _destructiveColor = QColor{styleObject->property("destructiveActionColor").toString()};
}

QImage QmlColoredImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    QString path = ":/icon/" + id;
    QColor c = _themeColor;

    auto queryPos = id.indexOf('?');
    if (queryPos >= 0) {
        path = ":/icon/" + id.left(queryPos); // without the query part
        const QString q = id.mid(queryPos + 1);
        if (q.startsWith('#')) {
            c = QColor(q); // allow directly specifying a color
        } else if (q == "text") {
            c = _textColor;
        } else if (q == "themeContrast") {
            c = _themeContrastColor;
        } else if (q == "active") {
            c = _activeColor;
        } else if (q == "destructive") {
            c = _destructiveColor;
        } else if (q == "theme") {
            // default already
        } else {
            qWarning() << Q_FUNC_INFO << "Unrecognized color specification:" << id;
        }
    }

    QImage originalImage = QImage{path};
    if (originalImage.isNull()) {
        qWarning() << Q_FUNC_INFO << "Failed to load image:" << path;
        return {};
    }

    if (!originalImage.isGrayscale()) {
        qWarning() << Q_FUNC_INFO << "Source image is not a greyscale mask:" << path;
    }

    const int baseRed = c.red();
    const int baseGreen = c.green();
    const int baseBlue = c.blue();
    *size = originalImage.size();

    // colorize it
    QImage colored{originalImage.size(), QImage::Format_ARGB32_Premultiplied};
    const int width = size->width();
    const int height = size->height();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // this is 0..255 ranged alpha/intensity value
            const int alpha = originalImage.pixel(x, y);
            colored.setPixel(x, y, qPremultiply(qRgba(baseRed, baseGreen, baseBlue, alpha)));
        }
    }

    return colored;
}

QPixmap QmlColoredImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize)
{
    QImage img = requestImage(id, size, requestedSize);
    return QPixmap::fromImage(img);
}

QmlColoredImageProvider* QmlColoredImageProvider::instance()
{
    return static_instance;
}
