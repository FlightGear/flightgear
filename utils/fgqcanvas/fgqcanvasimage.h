#ifndef FGQCANVASIMAGE_H
#define FGQCANVASIMAGE_H

#include <QPixmap>

#include "fgcanvaselement.h"

class FGQCanvasImage : public FGCanvasElement
{
    Q_OBJECT
public:
    FGQCanvasImage(FGCanvasGroup* pr, LocalProp* prop);

protected:
    virtual void doPaint(FGCanvasPaintContext* context) const override;

    virtual void markStyleDirty() override;

private slots:
    void markImageDirty();
    void markSourceDirty();

private:
    bool onChildAdded(LocalProp *prop) override;

    void rebuildImage() const;
    void recomputeSourceRect() const;

private:
    mutable bool _imageDirty;
    mutable bool _sourceRectDirty = true;
    mutable QPixmap _image;
    QString _source;
    mutable QSizeF _destSize;
    mutable QRectF _sourceRect;
};

#endif // FGQCANVASIMAGE_H
