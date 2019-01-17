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
class CanvasItem;
class QQuickItem;
class CanvasConnection;

/**
 * Coordinate reference frame (eg. "clip" property)
 */
enum class ReferenceFrame
{
  GLOBAL, ///< Global coordinates
  PARENT, ///< Coordinates relative to parent coordinate frame
  LOCAL   ///< Coordinates relative to local coordinates (parent
          ///  coordinates with local transformations applied)
};

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

    const FGCanvasGroup* rootGroup() const;

    CanvasConnection* connection() const;

    static bool isStyleProperty(QByteArray name);

    LocalProp* property() const;

    void setHighlighted(bool hilighted);
    bool isHighlighted() const;

    virtual CanvasItem* quickItem() const;

    virtual CanvasItem* createQuickItem(QQuickItem* parent);

    void requestPolish();

    void polish();

    virtual void dumpElement() = 0;
protected:

    virtual void doPaint(FGCanvasPaintContext* context) const;
    virtual void doPolish();

    virtual bool onChildAdded(LocalProp* prop);
    virtual bool onChildRemoved(LocalProp* prop);

    virtual void doDestroy();

    const LocalProp* _propertyRoot;
    const FGCanvasGroup* _parent;

    QColor fillColor() const;

    QColor parseColorValue(QVariant value) const;

    virtual void markStyleDirty();

    QVariant getCascadedStyle(const char* name, QVariant defaultValue = QVariant()) const;

private slots:
    void onPropDestroyed();

private:

    void onCenterChanged(QVariant value);

    void markTransformsDirty();

    void markZIndexDirty(QVariant value);

    void onVisibleChanged(QVariant value);

    void markClipDirty();
    void markSVGIDDirty(QVariant value);

private:
    friend class FGCanvasGroup;

    bool _polishRequired = false;
    bool _visible = true;
    bool _highlighted = false;

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
    mutable ReferenceFrame _clipFrame = ReferenceFrame::GLOBAL;

    void parseCSSClip(QByteArray value);
    double parseCSSValue(QByteArray value) const;
};

using FGCanvasElementVec = std::vector<FGCanvasElement*>;

#endif // FGCANVASELEMENT_H
