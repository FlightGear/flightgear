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

#include "canvasitem.h"

#include <QMatrix4x4>
#include <QSGClipNode>

class LocalTransform : public QQuickTransform
{
    Q_OBJECT
public:
    LocalTransform(QObject *parent) : QQuickTransform(parent) {}

    void setTransform(const QMatrix4x4 &t) {
        transform = t;
        update();
    }
    void applyTo(QMatrix4x4 *matrix) const Q_DECL_OVERRIDE {
        *matrix *= transform;
    }
private:
    QMatrix4x4 transform;
};

CanvasItem::CanvasItem(QQuickItem* pr)
    : QQuickItem(pr)
    , m_localTransform(new LocalTransform(this))
{
    m_localTransform->prependToItem(this);
}

void CanvasItem::setTransform(const QMatrix4x4 &mat)
{
    m_localTransform->setTransform(mat);
}

void CanvasItem::setGlobalClip(const QRectF &clip)
{
    m_hasClip = true;
    update();
}

QSGClipNode *CanvasItem::getClipNode()
{
    if (!m_hasClip) {
        return nullptr;
    }

    if (!m_clipNode) {
        m_clipNode = new QSGClipNode();
    }

    // transform global rect to local
    QRectF localRect(mapFromGlobal(m_globalClipRect.topLeft()),
                     mapFromGlobal(m_globalClipRect.bottomRight()));
    m_clipNode->setClipRect(localRect);
    return m_clipNode;
}

#include "canvasitem.moc"
