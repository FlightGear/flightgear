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

#include "canvasconnection.h"
#include "fgcanvasgroup.h"
#include "fgcanvaspaintcontext.h"
#include "canvasitem.h"

CanvasDisplay::CanvasDisplay(QQuickItem* parent) :
    QQuickItem(parent)
{
    setSize(QSizeF(400, 400));
    qDebug() << "created a canvas display";

    setFlag(ItemHasContents);
}

CanvasDisplay::~CanvasDisplay()
{

}

void CanvasDisplay::updatePolish()
{
    m_rootElement->polish();
}

void CanvasDisplay::setCanvas(CanvasConnection *canvas)
{
    if (m_connection == canvas)
        return;

    if (m_connection) {
        disconnect(m_connection, nullptr, this, nullptr);
    }

    m_connection = canvas;
    emit canvasChanged(m_connection);

    // delete existing children

    connect(m_connection, &CanvasConnection::statusChanged,
            this, &CanvasDisplay::onConnectionStatusChanged);
    connect(m_connection, &CanvasConnection::updated,
            this, &CanvasDisplay::onConnectionUpdated);

}

void CanvasDisplay::onConnectionStatusChanged()
{
    if (m_connection->status() == CanvasConnection::Connected) {
        m_rootElement.reset(new FGCanvasGroup(nullptr, m_connection->propertyRoot()));
        auto qq = m_rootElement->createQuickItem(this);
        qq->setSize(QSizeF(400, 400));
    }
}

void CanvasDisplay::onConnectionUpdated()
{
    m_rootElement->polish();
    update();
}
