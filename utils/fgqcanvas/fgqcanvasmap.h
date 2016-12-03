#ifndef FGQCANVASMAP_H
#define FGQCANVASMAP_H

#include "fgcanvasgroup.h"

class FGQCanvasMap : public FGCanvasGroup
{
public:
    FGQCanvasMap(FGCanvasGroup* pr, LocalProp* prop);

protected:
    virtual void doPaint(FGCanvasPaintContext* context) const;

    virtual bool onChildAdded(LocalProp* prop);

private:
    void markProjectionDirty();

private:
    double _projectionCenterLat;
    double _projectionCenterLon;
    double _range;

    mutable bool _projectionChanged;
};

#endif // FGQCANVASMAP_H
