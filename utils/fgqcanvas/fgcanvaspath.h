#ifndef FGCANVASPATH_H
#define FGCANVASPATH_H

#include "fgcanvaselement.h"

#include <QPainterPath>
#include <QPen>

class FGCanvasPath : public FGCanvasElement
{
public:
    FGCanvasPath(FGCanvasGroup* pr, LocalProp* prop);

protected:
    virtual void doPaint(FGCanvasPaintContext* context) const override;

    virtual void markStyleDirty() override;
private:
    void markPathDirty();
    void markStrokeDirty();
private:
    bool onChildAdded(LocalProp *prop) override;

    void rebuildPath() const;
    void rebuildPen() const;

private:
    mutable bool _pathDirty = true;
    mutable QPainterPath _painterPath;
    mutable bool _penDirty = true;
    mutable QPen _stroke;
};

#endif // FGCANVASPATH_H
