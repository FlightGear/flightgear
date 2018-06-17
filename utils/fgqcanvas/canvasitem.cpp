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
    void applyTo(QMatrix4x4 *matrix) const override
    {
        *matrix *= transform;
    }
private:
    QMatrix4x4 transform;
};

CanvasItem::CanvasItem(QQuickItem* pr)
    : QQuickItem(pr)
    , m_localTransform(new LocalTransform(this))
{
    setFlag(ItemHasContents);
    m_localTransform->prependToItem(this);
}

void CanvasItem::setTransform(const QMatrix4x4 &mat)
{
    m_localTransform->setTransform(mat);
}

void CanvasItem::setClip(const QRectF &clip, ReferenceFrame rf)
{
    if (m_hasClip && (clip == m_clipRect) && (rf == m_clipReferenceFrame)) {
        return;
    }

    m_hasClip = true;
    m_clipRect = clip;
    m_clipReferenceFrame = rf;
    update();
}

void CanvasItem::setClipReferenceFrameItem(QQuickItem *refItem)
{
    m_clipReferenceFrameItem = refItem;
}

void CanvasItem::clearClip()
{
    m_hasClip = false;
    update();
}

QSGNode *CanvasItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *d)
{
    QSGNode* realOldNode = oldNode;
    QSGClipNode* oldClip = nullptr;

    if (oldNode && (oldNode->type() == QSGNode::ClipNodeType)) {
        Q_ASSERT(oldNode->childCount() == 1);
        realOldNode = oldNode->childAtIndex(0);
        oldClip = static_cast<QSGClipNode*>(oldNode);
    }

    QSGNode* contentNode = updateRealPaintNode(realOldNode, d);
    if (!contentNode) {
        return nullptr;
    }

    QSGNode* clipNode = updateClipNode(oldClip, contentNode);
    return clipNode ? clipNode : contentNode;
}

QSGNode *CanvasItem::updateRealPaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *d)
{
    if (oldNode) {
        return oldNode;
    }

    return new QSGNode();
}

QRectF checkRectangularClip(QPointF* vertices)
{
    // order is TL / BL / TR / BR to match updateRectGeometry
    const double top = vertices[0].y();
    const double left = vertices[0].x();
    const double bottom = vertices[1].y();
    const double right = vertices[2].x();

    if (vertices[1].x() != left) return {};
    if (vertices[2].y() != top) return {};
    if ((vertices[3].x() != right) || (vertices[3].y() != bottom))
        return {};

    return QRectF(vertices[0], vertices[3]);
}

QSGClipNode* CanvasItem::updateClipNode(QSGClipNode* oldClipNode, QSGNode* contentNode)
{
    Q_ASSERT(contentNode);
    if (!m_hasClip) {
        return nullptr;
    }

    QSGGeometry* clipGeometry = nullptr;
    QSGClipNode* clipNode = oldClipNode;

    if (!clipNode) {
        clipNode = new QSGClipNode();
        clipGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
        clipGeometry->setDrawingMode(GL_TRIANGLE_STRIP);
        clipNode->setGeometry(clipGeometry);
        clipNode->setFlag(QSGNode::OwnsGeometry);
        clipNode->appendChildNode(contentNode);
    } else {
        if (clipNode->childCount() == 1) {
            const auto existingChild = clipNode->childAtIndex(0);
            if (existingChild == contentNode) {
                qInfo() << "optimise for this case!";
            }
        }

        clipNode->removeAllChildNodes();
        clipNode->appendChildNode(contentNode);
        clipGeometry = clipNode->geometry();
        Q_ASSERT(clipGeometry);
    }

    QPointF clipVertices[4],
            inVertices[4] = {m_clipRect.topLeft(), m_clipRect.bottomLeft(),
                             m_clipRect.topRight(), m_clipRect.bottomRight()};
    QRectF rectClip;

    switch (m_clipReferenceFrame) {
    case ReferenceFrame::GLOBAL:
    case ReferenceFrame::PARENT:
        Q_ASSERT(m_clipReferenceFrameItem);
        for (int i=0; i<4; ++i) {
            clipVertices[i] = mapFromItem(m_clipReferenceFrameItem, inVertices[i]);
        }
        rectClip = checkRectangularClip(clipVertices);
        break;

    case ReferenceFrame::LOCAL:
        // local ref-frame clip is always rectangular
        rectClip = m_clipRect;
        for (int i=0; i<4; ++i) {
            clipVertices[i] = inVertices[i];
        }
        break;
    }

    clipNode->setIsRectangular(!rectClip.isNull());
    qInfo() << "\nobj:" << objectName();
    if (!rectClip.isNull()) {
        qInfo() << "have rectangular clip for:" << m_clipRect << (int) m_clipReferenceFrame << rectClip;
        clipNode->setClipRect(rectClip);
    } else {
        qInfo() << "haved rotated clip" << m_clipRect << (int) m_clipReferenceFrame;
        qInfo() << "final local clip points:" << clipVertices[0] << clipVertices[1]
                << clipVertices[2] << clipVertices[3];
    }

    QSGGeometry::Point2D *v = clipGeometry->vertexDataAsPoint2D();
    for (int i=0; i<4; ++i) {
        v[i].x = clipVertices[i].x();
        v[i].y = clipVertices[i].y();
    }
    clipGeometry->markVertexDataDirty();
    clipNode->markDirty(QSGNode::DirtyGeometry);
    return clipNode;
}

#include "canvasitem.moc"
