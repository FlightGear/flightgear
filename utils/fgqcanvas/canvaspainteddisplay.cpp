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

#include "canvaspainteddisplay.h"

#include <QDebug>

#include "canvasconnection.h"
#include "fgcanvasgroup.h"
#include "fgcanvaspaintcontext.h"
#include "localprop.h"

CanvasPaintedDisplay::CanvasPaintedDisplay(QQuickItem* parent) :
    QQuickPaintedItem(parent)
{
    setTransformOrigin(QQuickItem::TopLeft);
    setAntialiasing(true);
}

CanvasPaintedDisplay::~CanvasPaintedDisplay()
{
}

void CanvasPaintedDisplay::paint(QPainter *painter)
{
    if (!m_rootElement)
        return;

    const double xScaleFactor = width() / m_sourceSize.width();
    const double yScaleFactor =  height() / m_sourceSize.height();
    const double f = std::min(xScaleFactor, yScaleFactor);
    painter->scale(f, f);

    FGCanvasPaintContext context(painter);
    m_rootElement->paint(&context);

}

void CanvasPaintedDisplay::geometryChanged(const QRectF &newGeometry, const QRectF &)
{
    Q_UNUSED(newGeometry);
    update();
}

void CanvasPaintedDisplay::setCanvas(CanvasConnection *canvas)
{
    if (m_connection == canvas)
        return;

    if (m_connection) {
        disconnect(m_connection, nullptr, this, nullptr);
        delete m_rootElement;
    }

    m_connection = canvas;
    emit canvasChanged(m_connection);

    if (m_connection) {
        connect(m_connection, &QObject::destroyed,
                this, &CanvasPaintedDisplay::onConnectionDestroyed);
        connect(m_connection, &CanvasConnection::statusChanged,
                this, &CanvasPaintedDisplay::onConnectionStatusChanged);
        connect(m_connection, &CanvasConnection::updated,
                this, &CanvasPaintedDisplay::onConnectionUpdated);

        onConnectionStatusChanged();
    }
}

void CanvasPaintedDisplay::onConnectionDestroyed()
{
    qDebug() << Q_FUNC_INFO << "saw connection destroyed";
    m_connection = nullptr;
    delete m_rootElement;

    emit canvasChanged(m_connection);
}

void CanvasPaintedDisplay::onConnectionStatusChanged()
{
    if ((m_connection->status() == CanvasConnection::Connected) ||
        (m_connection->status() == CanvasConnection::Snapshot))
    {
        buildElements();
    } else {
        if (m_rootElement) {
            qDebug() << Q_FUNC_INFO << "clearing root element";
            delete m_rootElement;
            m_rootElement.clear();
        }
        update();
    }
}

void CanvasPaintedDisplay::buildElements()
{
    m_rootElement = new FGCanvasGroup(nullptr, m_connection->propertyRoot());
    // this is important to elements can discover their connection
    // by walking their parent chain
    m_rootElement->setParent(m_connection);

    connect(m_rootElement.data(), &FGCanvasGroup::canvasSizeChanged,
            this, &CanvasPaintedDisplay::onCanvasSizeChanged);

    onCanvasSizeChanged();

    m_connection->propertyRoot()->recursiveNotifyRestored();
    m_rootElement->polish();
    update();
}

void CanvasPaintedDisplay::onConnectionUpdated()
{
    if (m_rootElement) {
        m_rootElement->polish();
        update();
    }
}

void CanvasPaintedDisplay::onCanvasSizeChanged()
{
    m_sourceSize = QSizeF(m_connection->propertyRoot()->value("size", 256).toDouble(),
                          m_connection->propertyRoot()->value("size[1]", 256).toDouble());
    setImplicitSize(m_sourceSize.width(), m_sourceSize.height());
    update();
}

