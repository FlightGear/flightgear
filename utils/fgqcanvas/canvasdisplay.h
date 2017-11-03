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

#ifndef CANVASDISPLAY_H
#define CANVASDISPLAY_H

#include <memory>

#include <QQuickItem>

class CanvasConnection;
class FGCanvasGroup;
class QQuickItem;

class CanvasDisplay : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(CanvasConnection* canvas READ canvas WRITE setCanvas NOTIFY canvasChanged)

public:
    CanvasDisplay(QQuickItem* parent = nullptr);
    ~CanvasDisplay();

    CanvasConnection* canvas() const
    {
        return m_connection;
    }

signals:

    void canvasChanged(CanvasConnection* canvas);

public slots:

void setCanvas(CanvasConnection* canvas);

protected:
    void updatePolish() override;

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private slots:
    void onConnectionStatusChanged();

    void onConnectionUpdated();

    void onCanvasSizeChanged();
    void onConnectionDestroyed();

private:
    void recomputeScaling();

    CanvasConnection* m_connection = nullptr;
    std::unique_ptr<FGCanvasGroup> m_rootElement;
    QQuickItem* m_rootItem = nullptr;
    QSizeF m_sourceSize;
};

#endif // CANVASDISPLAY_H
