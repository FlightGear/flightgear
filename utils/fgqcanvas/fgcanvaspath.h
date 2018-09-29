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

#ifndef FGCANVASPATH_H
#define FGCANVASPATH_H

#include "fgcanvaselement.h"

#include <QPainterPath>
#include <QPen>

class PathQuickItem;

class FGCanvasPath : public FGCanvasElement
{
public:
    FGCanvasPath(FGCanvasGroup* pr, LocalProp* prop);

    void dumpElement() override;
protected:
    virtual void doPaint(FGCanvasPaintContext* context) const override;

    void doPolish() override;

    virtual void markStyleDirty() override;

    CanvasItem* createQuickItem(QQuickItem *parent) override;
    CanvasItem* quickItem() const override;

    void doDestroy() override;

private:
    void markPathDirty();
    void markStrokeDirty();
private:
    bool onChildAdded(LocalProp *prop) override;
    bool onChildRemoved(LocalProp* prop) override;

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

    PathQuickItem* _quickPath = nullptr;
};

#endif // FGCANVASPATH_H
