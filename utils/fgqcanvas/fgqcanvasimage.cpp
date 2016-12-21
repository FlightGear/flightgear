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

    context->painter()->drawPixmap(0, 0, _image);
}

bool FGQCanvasImage::onChildAdded(LocalProp *prop)
{
    if (FGCanvasElement::onChildAdded(prop)) {
        return true;
    }

    const QByteArray nm = prop->name();
    if ((nm == "source") || (nm == "src") || (nm == "size") || (nm == "file")) {
        connect(prop, &LocalProp::valueChanged, this, &FGQCanvasImage::markImageDirty);
        return true;
    }

    qDebug() << "image saw child:" << prop->name();
    return false;
}

void FGQCanvasImage::markImageDirty()
{
    _imageDirty = true;
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
            qDebug() << "have image" << _image.size();
        }

     } else {
        qDebug() << "source" << _propertyRoot->value("source", QString());
        qDebug() << "src" << _propertyRoot->value("src", QString());
    }

    _destSize = QSizeF(_propertyRoot->value("size[0]", 0.0).toFloat(),
                       _propertyRoot->value("size[1]", 0.0).toFloat());
    qDebug() << "dest-size" << _destSize;

    _imageDirty = false;
}

void FGQCanvasImage::markStyleDirty()
{
}
