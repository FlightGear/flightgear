#include "fgqcanvasimage.h"

#include <QPainter>
#include <QDebug>

#include "fgcanvaspaintcontext.h"
#include "localprop.h"
#include "fgqcanvasimageloader.h"

FGQCanvasImage::FGQCanvasImage(FGCanvasGroup* pr, LocalProp* prop) :
    FGCanvasElement(pr, prop)
{

}

void FGQCanvasImage::doPaint(FGCanvasPaintContext *context) const
{
    if (_imageDirty) {
        rebuildImage();
        _imageDirty = false;
    }

    if (_sourceRectDirty) {
        recomputeSourceRect();
    }

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

    qDebug() << "image saw child:" << prop->name();
    return false;
}

void FGQCanvasImage::markImageDirty()
{
    _imageDirty = true;
}

void FGQCanvasImage::markSourceDirty()
{
    _sourceRectDirty = true;
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
}

void FGQCanvasImage::rebuildImage() const
{
    QByteArray file = _propertyRoot->value("file", QByteArray()).toByteArray();
    if (!file.isEmpty()) {
         _image = FGQCanvasImageLoader::instance()->getImage(file);


        if (_image.isNull()) {
            // get notified when the image loads
            FGQCanvasImageLoader::instance()->connectToImageLoaded(file,
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
}

void FGQCanvasImage::markStyleDirty()
{
}
