// PixmapImageItem.hxx - display a QPixmap/QImage directly
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


#ifndef PIXMAPIMAGEITEM_HXX
#define PIXMAPIMAGEITEM_HXX

#include <QQuickPaintedItem>
#include <QImage>

class PixmapImageItem : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)
public:
    PixmapImageItem(QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    QImage image() const
    { return _image; }

    void setImage(QImage img);

signals:
    void imageChanged();

private:
    QImage _image;
};

#endif // PIXMAPIMAGEITEM_HXX
