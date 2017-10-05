//
//  QtQuickFGCanvasItem.hxx
//
//  Copyright (C) 2017 James Turner <zakalawe@mac.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef QtQuickFGCanvasItem_hpp
#define QtQuickFGCanvasItem_hpp

#include <QQuickItem>

#include <simgear/canvas/canvas_fwd.hxx>
#include <simgear/props/props.hxx>

#include <Canvas/canvas_mgr.hxx>

/**
 * Custom QtQuick item for displaying a FlightGear canvas (and interacting with it)
 *
 * Note this unrelated to the QtQuick 'native' canvas (HTML5 canvas 2D implementation)!
 */
class QtQuickFGCanvasItem : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QString canvas READ canvas WRITE setCanvas NOTIFY canvasChanged)
public:
    QtQuickFGCanvasItem(QQuickItem* parent = nullptr);
    virtual ~QtQuickFGCanvasItem();

    QString canvas() const
    {
        return _canvasName;
    }

public slots:
    void setCanvas(QString canvas);

signals:
    void canvasChanged(QString canvas);

protected:

    QSGNode* updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void initCanvasNasalModules();
    void updateCanvasGeometry();

    simgear::canvas::MouseEventPtr convertMouseEvent(QMouseEvent *event);

    simgear::canvas::CanvasPtr _canvas;
    QString _canvasName;
    QPointF _lastMouseEventScreenPosition;
};

#endif /* QtQuickFGCanvasItem_hpp */
