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

#include "fgqcanvasimage.h"

#include <QPainter>
#include <QDebug>
#include <QQmlComponent>

#include "fgcanvaspaintcontext.h"
#include "localprop.h"
#include "fgqcanvasimageloader.h"
#include "canvasitem.h"
#include "canvasconnection.h"


#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#include <QSGTexture>
#include <QSGSimpleTextureNode>
#include <QQuickWindow>

class ImageQuickItem : public CanvasItem
{
    Q_OBJECT

public:
    ImageQuickItem(QQuickItem* parent)
        : CanvasItem(parent)
    {
        setFlag(ItemHasContents);
    }

    void setSourceRect(const QRectF& sourceRect)
    {
        m_sourceRect = sourceRect;
        update();
    }

    void setSize(const QSizeF &size)
    {
        m_size = size;
        setImplicitSize(size.width(), size.height());
        update();
    }

    void setPixmap(QPixmap pixmap)
    {
        m_pixmap = pixmap;
         update();
    }

    QSGNode* updateRealPaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData *data) override
    {
        if (m_pixmap.isNull()) {
            return nullptr;
        }

        QSGSimpleTextureNode* texNode = static_cast<QSGSimpleTextureNode*>(oldNode);
        if (!texNode) {
            texNode = new QSGSimpleTextureNode;
            texNode->setOwnsTexture(true);
        }

        texNode->setRect(QRectF(QPointF(), m_size));
        texNode->setSourceRect(m_sourceRect);

        if (m_texture) {
            delete m_texture;
        }
        m_texture = window()->createTextureFromImage(m_pixmap.toImage(), QQuickWindow::TextureCanUseAtlas);
        texNode->setTexture(m_texture);
        return texNode;
    }

protected:
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override
    {
        QQuickItem::geometryChanged(newGeometry, oldGeometry);
        update();
    }

    QRectF boundingRect() const override
    {
        if (!widthValid() || !heightValid()) {
            return QRectF(QPointF(), m_size);
        }

        return QQuickItem::boundingRect();
    }

private:
    QRectF m_sourceRect;
    QSGTexture* m_texture = nullptr;
    QSizeF m_size;
    QPixmap m_pixmap;
};

FGQCanvasImage::FGQCanvasImage(FGCanvasGroup* pr, LocalProp* prop) :
    FGCanvasElement(pr, prop)
{
}

void FGQCanvasImage::doPolish()
{
    if (_imageDirty) {
        rebuildImage();
        _imageDirty = false;
    }

    if (_sourceRectDirty) {
        recomputeSourceRect();
    }
}

void FGQCanvasImage::doPaint(FGCanvasPaintContext *context) const
{
    QRectF dstRect(0.0, 0.0, _destSize.width(), _destSize.height());
    context->painter()->drawPixmap(dstRect, _image, _sourceRect);
}

bool FGQCanvasImage::onChildAdded(LocalProp *prop)
{
    if (FGCanvasElement::onChildAdded(prop)) {
        return true;
    }

    const QByteArray nm = prop->name();
    if ((nm == "src") || (nm == "size") || (nm == "file")) {
        connect(prop, &LocalProp::valueChanged, this, &FGQCanvasImage::markImageDirty);
        return true;
    }

    if (nm == "source") {
        FGQCanvasImage* self = this;
        connect(prop, &LocalProp::childAdded, [self](LocalProp* newChild) {
            connect(newChild, &LocalProp::valueChanged, self, &FGQCanvasImage::markSourceDirty);
        });
        return true;
    }

    return false;
}

void FGQCanvasImage::markImageDirty()
{
    _imageDirty = true;
    requestPolish();
}

void FGQCanvasImage::markSourceDirty()
{
    _sourceRectDirty = true;
    requestPolish();
}

void FGQCanvasImage::recomputeSourceRect() const
{
    const float imageWidth = _image.width();
    const float imageHeight = _image.height();
    _sourceRect = QRectF(0, 0, imageWidth, imageHeight);
    if (!_propertyRoot->hasChild("source")) {
        return;
    }

    const bool normalized = _propertyRoot->value("source/normalized", true).toBool();
    float left =  _propertyRoot->value("source/left", 0.0).toFloat();
    float top =  _propertyRoot->value("source/top", 0.0).toFloat();
    float right =  _propertyRoot->value("source/right", 1.0).toFloat();
    float bottom =  _propertyRoot->value("source/bottom", 1.0).toFloat();

    if (normalized) {
        left *= imageWidth;
        right *= imageWidth;
        top *= imageHeight;
        bottom *= imageHeight;
    }

    _sourceRect =  QRectF(left, top, right - left, bottom - top);
    _sourceRectDirty = false;

    if (_quickItem) {
        _quickItem->setSourceRect(_sourceRect);
    }
}

void FGQCanvasImage::rebuildImage() const
{
    QByteArray file = _propertyRoot->value("file", QByteArray()).toByteArray();
    auto loader = connection()->imageLoader();
    if (!file.isEmpty()) {
         _image = loader->getImage(file);


        if (_image.isNull()) {
            // get notified when the image loads
            loader->connectToImageLoaded(file,
                                         const_cast<FGQCanvasImage*>(this),
                                         SLOT(markImageDirty()));
        } else {
            // loaded image ok!
        }

     } else {
        qDebug() << "src" << _propertyRoot->value("src", QString());
    }

    _destSize = QSizeF(_propertyRoot->value("size[0]", 0.0).toFloat(),
                       _propertyRoot->value("size[1]", 0.0).toFloat());

    _imageDirty = false;

    if (_quickItem) {
        _quickItem->setSize(_destSize);
        _quickItem->setPixmap(_image);
    }
}

void FGQCanvasImage::markStyleDirty()
{
}

void FGQCanvasImage::doDestroy()
{
    delete _quickItem;
}

CanvasItem *FGQCanvasImage::createQuickItem(QQuickItem *parent)
{
    _quickItem = new ImageQuickItem(parent);

    _quickItem->setSourceRect(_sourceRect);
    _quickItem->setSize(_destSize);
    _quickItem->setPixmap(_image);

    return _quickItem;
}

CanvasItem *FGQCanvasImage::quickItem() const
{
    return _quickItem;
}

void FGQCanvasImage::dumpElement()
{
    qDebug() << "Image: " << _source << " at " << _propertyRoot->path();
}

#include "fgqcanvasimage.moc"
