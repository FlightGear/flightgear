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

#ifndef CANVAS_PAINTED_DISPLAY_H
#define CANVAS_PAINTED_DISPLAY_H

#include <memory>

#include <QQuickPaintedItem>
#include <QPointer>

class CanvasConnection;
class FGCanvasGroup;
class QQuickItem;

class CanvasPaintedDisplay : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(CanvasConnection* canvas READ canvas WRITE setCanvas NOTIFY canvasChanged)

public:
    CanvasPaintedDisplay(QQuickItem* parent = nullptr);
    ~CanvasPaintedDisplay();

    CanvasConnection* canvas() const
    {
        return m_connection;
    }

    void paint(QPainter *painter) override;
signals:

    void canvasChanged(CanvasConnection* canvas);

public slots:

    void setCanvas(CanvasConnection* canvas);

protected:

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private slots:
    void onConnectionStatusChanged();

    void onConnectionUpdated();

    void onCanvasSizeChanged();
    void onConnectionDestroyed();

private:
    void recomputeScaling();
    void buildElements();

    CanvasConnection* m_connection = nullptr;
    QPointer<FGCanvasGroup> m_rootElement;
   // QQuickItem* m_rootItem = nullptr;
    QSizeF m_sourceSize;
};

#endif // CANVAS_PAINTED_DISPLAY_H
