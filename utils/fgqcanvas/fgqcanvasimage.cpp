#include "fgqcanvasimage.h"

#include <QPainter>
#include <QDebug>

#include "fgcanvaspaintcontext.h"
#include "localprop.h"

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
    qDebug() << "source" << _propertyRoot->value("source", QString());
    qDebug() << "src" << _propertyRoot->value("src", QString());
    qDebug() << "file" << _propertyRoot->value("file", QString());

}

void FGQCanvasImage::markStyleDirty()
{
}
