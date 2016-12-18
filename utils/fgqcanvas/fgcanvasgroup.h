#ifndef FGCANVASGROUP_H
#define FGCANVASGROUP_H

#include "fgcanvaselement.h"

class FGCanvasGroup : public FGCanvasElement
{
    Q_OBJECT
public:
    explicit FGCanvasGroup(FGCanvasGroup* pr, LocalProp* prop);

    const FGCanvasElementVec& children() const;

    void markChildZIndicesDirty() const;

    bool hasChilden() const;

    unsigned int childCount() const;

    FGCanvasElement* childAt(unsigned int index) const;

    unsigned int indexOfChild(const FGCanvasElement* e) const;

signals:
    void childAdded();
    void childRemoved(int index);

protected:
    virtual void doPaint(FGCanvasPaintContext* context) const override;

    bool onChildAdded(LocalProp *prop) override;
    bool onChildRemoved(LocalProp *prop) override;

    virtual void markStyleDirty() override;
private:
    void markCachedSymbolDirty();
    int indexOfChildWithProp(LocalProp *prop) const;

private:
    mutable FGCanvasElementVec _children;
    mutable bool _zIndicesDirty = false;
    mutable bool _cachedSymbolDirty = false;
};

#endif // FGCANVASGROUP_H
