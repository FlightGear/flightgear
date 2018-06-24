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

#ifndef FGCANVASTEXT_H
#define FGCANVASTEXT_H

#include <QFont>
#include <QFontMetricsF>

#include "fgcanvaselement.h"

class TextCanvasItem;

class FGCanvasText : public FGCanvasElement
{
public:
    FGCanvasText(FGCanvasGroup* pr, LocalProp* prop);

    CanvasItem* createQuickItem(QQuickItem *parent) override;
    CanvasItem* quickItem() const override;

    void dumpElement() override;
protected:
    virtual void doPaint(FGCanvasPaintContext* context) const override;
    void doPolish() override;

    virtual void markStyleDirty() override;

    void doDestroy() override;

private:
    bool onChildAdded(LocalProp *prop) override;

    void onTextChanged(QVariant var);

    void setDrawMode(QVariant var);

    void markFontDirty();

    void onFontLoaded(QByteArray name);
private:
    void rebuildFont() const;
    void rebuildAlignment(QVariant var) const;

    QString _text;

    mutable Qt::Alignment _alignment;
    mutable bool _fontDirty = true;
    mutable QFont _font;
    mutable QFontMetricsF _metrics;

    TextCanvasItem* _quickItem = nullptr;
};


#endif // FGCANVASTEXT_H
