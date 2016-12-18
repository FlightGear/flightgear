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

    void rebuildPathFromCommands(const std::vector<int>& commands, const std::vector<float>& coords) const;
    bool rebuildFromSVGData(std::vector<int>& commands, std::vector<float>& coords) const;
    bool rebuildFromRect(std::vector<int> &commands, std::vector<float> &coords) const;
private:
    enum PaintType
    {
        Path,
        Rect,
        RoundRect
    };

    mutable bool _pathDirty = true;
    mutable QPainterPath _painterPath;
    mutable bool _penDirty = true;
    mutable QPen _stroke;
    bool _isRect = false;

    mutable PaintType _paintType = Path;
    mutable QRectF _rect;
    mutable QSizeF _roundRectRadius;
};

#endif // FGCANVASPATH_H
