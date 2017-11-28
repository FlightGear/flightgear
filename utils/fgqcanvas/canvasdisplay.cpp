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

#include "canvasdisplay.h"

#include <QDebug>
#include <QQuickItem>

#include "canvasconnection.h"
#include "fgcanvasgroup.h"
#include "fgcanvaspaintcontext.h"
#include "canvasitem.h"
#include "localprop.h"

CanvasDisplay::CanvasDisplay(QQuickItem* parent) :
    QQuickItem(parent)
{
    setTransformOrigin(QQuickItem::TopLeft);
    setFlag(ItemHasContents);
}

CanvasDisplay::~CanvasDisplay()
{
    delete m_rootItem;
}

void CanvasDisplay::updatePolish()
{
    m_rootElement->polish();
}

void CanvasDisplay::geometryChanged(const QRectF &newGeometry, const QRectF &)
{
    Q_UNUSED(newGeometry);
    recomputeScaling();
}

void CanvasDisplay::setCanvas(CanvasConnection *canvas)
{
    if (m_connection == canvas)
        return;

    if (m_connection) {
        disconnect(m_connection, nullptr, this, nullptr);


        qDebug() << "deleting items";
        delete m_rootItem;

        qDebug() << "deleting elements";
        m_rootElement.reset();

        qDebug() << "done";
    }

    m_connection = canvas;
    emit canvasChanged(m_connection);

    if (m_connection) {
        connect(m_connection, &QObject::destroyed,
                this, &CanvasDisplay::onConnectionDestroyed);
        connect(m_connection, &CanvasConnection::statusChanged,
                this, &CanvasDisplay::onConnectionStatusChanged);
        connect(m_connection, &CanvasConnection::updated,
                this, &CanvasDisplay::onConnectionUpdated);

        onConnectionStatusChanged();
    }

}

void CanvasDisplay::onConnectionDestroyed()
{
    m_connection = nullptr;
    emit canvasChanged(m_connection);

    m_rootElement.reset();
}

void CanvasDisplay::onConnectionStatusChanged()
{
    if ((m_connection->status() == CanvasConnection::Connected) ||
            (m_connection->status() == CanvasConnection::Snapshot))
    {
        m_rootElement.reset(new FGCanvasGroup(nullptr, m_connection->propertyRoot()));
        // this is important to elements can discover their connection
        // by walking their parent chain
        m_rootElement->setParent(m_connection);

        connect(m_rootElement.get(), &FGCanvasGroup::canvasSizeChanged,
                this, &CanvasDisplay::onCanvasSizeChanged);

        m_rootItem = m_rootElement->createQuickItem(this);
        onCanvasSizeChanged();

        if (m_connection->status() == CanvasConnection::Snapshot) {
            m_connection->propertyRoot()->recursiveNotifyRestored();
            m_rootElement->polish();
            update();
        }
    }
}

void CanvasDisplay::onConnectionUpdated()
{
    if (m_rootElement) {
        m_rootElement->polish();
        update();
    }
}

void CanvasDisplay::onCanvasSizeChanged()
{
    m_sourceSize = QSizeF(m_connection->propertyRoot()->value("size", 256).toDouble(),
                          m_connection->propertyRoot()->value("size[1]", 256).toDouble());
    setImplicitSize(m_sourceSize.width(), m_sourceSize.height());
    recomputeScaling();
}

void CanvasDisplay::recomputeScaling()
{
    const double xScaleFactor = width() / m_sourceSize.width();
    const double yScaleFactor =  height() / m_sourceSize.height();
    setScale(std::min(xScaleFactor, yScaleFactor));
}
