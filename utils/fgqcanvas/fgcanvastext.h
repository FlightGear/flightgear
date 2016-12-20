#ifndef FGCANVASTEXT_H
#define FGCANVASTEXT_H

#include <QFont>
#include <QFontMetricsF>

#include "fgcanvaselement.h"

class FGCanvasText : public FGCanvasElement
{
public:
    FGCanvasText(FGCanvasGroup* pr, LocalProp* prop);


protected:
    virtual void doPaint(FGCanvasPaintContext* context) const override;

    virtual void markStyleDirty() override;
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
};


#endif // FGCANVASTEXT_H
