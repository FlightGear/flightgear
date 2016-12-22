#ifndef FGCANVASELEMENT_H
#define FGCANVASELEMENT_H

#include <QObject>
#include <QTransform>
#include <QColor>
#include <QVariant>

#include <vector>

class LocalProp;
class FGCanvasPaintContext;
class FGCanvasGroup;

class FGCanvasElement : public QObject
{
    Q_OBJECT
public:
    explicit FGCanvasElement(FGCanvasGroup* pr, LocalProp* prop);

    void paint(FGCanvasPaintContext* context) const;

    QTransform combinedTransform() const;

    bool isVisible() const;

    int zIndex() const;

    const FGCanvasGroup* parentGroup() const;

    static bool isStyleProperty(QByteArray name);

    LocalProp* property() const;
protected:
    virtual void doPaint(FGCanvasPaintContext* context) const;

    virtual bool onChildAdded(LocalProp* prop);
    virtual bool onChildRemoved(LocalProp* prop);

    const LocalProp* _propertyRoot;
    const FGCanvasGroup* _parent;

    QColor fillColor() const;

    QColor parseColorValue(QVariant value) const;

    virtual void markStyleDirty();

    QVariant getCascadedStyle(const char* name, QVariant defaultValue = QVariant()) const;
private:

    void onCenterChanged(QVariant value);

    void markTransformsDirty();

    void markZIndexDirty(QVariant value);

    void onVisibleChanged(QVariant value);

    void markClipDirty();

private:
    friend class FGCanvasGroup;

    bool _visible = true;
    mutable bool _transformsDirty = true;
    mutable bool _styleDirty = true;

    mutable QTransform _combinedTransform;

    QPointF _center;

    mutable QColor _fillColor;
    int _zIndex = 0;
    QByteArray _svgElementId;

    mutable bool _clipDirty = true;
    mutable bool _hasClip = false;
    mutable QRectF _clipRect;

    void parseCSSClip(QByteArray value) const;
    float parseCSSValue(QByteArray value) const;
};

using FGCanvasElementVec = std::vector<FGCanvasElement*>;

#endif // FGCANVASELEMENT_H
