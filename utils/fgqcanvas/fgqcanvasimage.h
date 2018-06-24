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

#ifndef FGQCANVASIMAGE_H
#define FGQCANVASIMAGE_H

#include <QPixmap>
#include <QQmlEngine>

#include "fgcanvaselement.h"

class ImageQuickItem;

class FGQCanvasImage : public FGCanvasElement
{
    Q_OBJECT
public:
    FGQCanvasImage(FGCanvasGroup* pr, LocalProp* prop);

    CanvasItem* createQuickItem(QQuickItem *parent) override;
    CanvasItem* quickItem() const override;

    void dumpElement() override;
protected:
    virtual void doPaint(FGCanvasPaintContext* context) const override;

    virtual void markStyleDirty() override;

    void doDestroy() override;

    void doPolish() override;
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

    ImageQuickItem* _quickItem = nullptr;
};

#endif // FGQCANVASIMAGE_H
