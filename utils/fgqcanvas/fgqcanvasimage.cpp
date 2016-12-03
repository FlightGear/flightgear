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

    qDebug() << "image saw child:" << prop->name();
    return false;
}

void FGQCanvasImage::rebuildImage() const
{

}

void FGQCanvasImage::markStyleDirty()
{
}
