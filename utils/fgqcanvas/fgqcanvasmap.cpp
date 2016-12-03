#include "fgqcanvasmap.h"

#include <QDebug>

#include "localprop.h"
#include "fgcanvaspaintcontext.h"

FGQCanvasMap::FGQCanvasMap(FGCanvasGroup* pr, LocalProp* prop) :
    FGCanvasGroup(pr, prop)
{

}

void FGQCanvasMap::doPaint(FGCanvasPaintContext *context) const
{

    FGCanvasGroup::doPaint(context);
}

bool FGQCanvasMap::onChildAdded(LocalProp *prop)
{
    const QByteArray nm = prop->name();
    if ((nm == "ref-lon") || (nm == "ref-lat") || (nm == "hdg") || (nm == "range")
        || (nm == "screen-range")) {
        connect(prop, &LocalProp::valueChanged, this, &FGQCanvasMap::markProjectionDirty);
        return true;
    }

    if (FGCanvasGroup::onChildAdded(prop)) {
        return true;
    }

    qDebug() << Q_FUNC_INFO << "deal with:" << prop->name();
    return false;
}

void FGQCanvasMap::markProjectionDirty()
{
    _projectionChanged = true;
}
