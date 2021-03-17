#include "TipBackgroundBox.hxx"

#include <QPainter>
#include <QPainterPath>
#include <QDebug>

namespace {

void pathLineBy(QPainterPath& pp, double x, double y)
{
    const auto c = pp.currentPosition();
    pp.lineTo(c.x() + x, c.y() + y);
}

const double arrowDim1 = 20.0;
const double arrowEndOffset = 30;

QPainterPath pathFromArrowAndGeometry(GettingStartedTip::Arrow arrow, const QRectF& g)
{
    QPainterPath pp;

    switch (arrow) {
    case GettingStartedTip::Arrow::TopCenter:
        pp.moveTo(g.center().x(), 0.0);
        pp.lineTo(g.center().x() + arrowDim1, arrowDim1);
        pp.lineTo(g.right(), arrowDim1);
        pp.lineTo(g.right(), g.bottom());
        pp.lineTo(g.left(), g.bottom());
        pp.lineTo(g.left(), arrowDim1);
        pp.lineTo(g.center().x() - arrowDim1, arrowDim1);
        pp.closeSubpath();
        break;

    case GettingStartedTip::Arrow::BottomRight:
        pp.moveTo(g.right() - arrowEndOffset, g.bottom());
        pathLineBy(pp, arrowDim1, -arrowDim1);
        pp.lineTo(g.right(), g.bottom() - arrowDim1);
        pp.lineTo(g.right(), g.top());
        pp.lineTo(g.left(), g.top());
        pp.lineTo(g.left(), g.bottom() - arrowDim1);
        pp.lineTo(g.right() - arrowEndOffset - arrowDim1, g.bottom() - arrowDim1);
        pp.closeSubpath();
        break;

    case GettingStartedTip::Arrow::TopRight:
        pp.moveTo(g.right() - arrowEndOffset, 0.0);
        pathLineBy(pp, arrowDim1, arrowDim1);
        pp.lineTo(g.right(), arrowDim1);
        pp.lineTo(g.right(), g.bottom());
        pp.lineTo(g.left(), g.bottom());
        pp.lineTo(g.left(), arrowDim1);
        pp.lineTo(g.right() - (arrowEndOffset + arrowDim1), arrowDim1);
        pp.closeSubpath();
        break;

    case GettingStartedTip::Arrow::TopLeft:
        pp.moveTo(arrowEndOffset, 0.0);
        pathLineBy(pp, arrowDim1, arrowDim1);
        pp.lineTo(g.right(), arrowDim1);
        pp.lineTo(g.right(), g.bottom());
        pp.lineTo(g.left(), g.bottom());
        pp.lineTo(g.left(), arrowDim1);
        pp.lineTo(arrowEndOffset - arrowDim1, arrowDim1);
        pp.closeSubpath();
        break;

    case GettingStartedTip::Arrow::LeftCenter:
        pp.moveTo(0.0, g.center().y());
        pathLineBy(pp, arrowDim1, -arrowDim1);
        pp.lineTo(arrowDim1, g.top());
        pp.lineTo(g.right(), g.top());
        pp.lineTo(g.right(), g.bottom());
        pp.lineTo(arrowDim1, g.bottom());
        pp.lineTo(arrowDim1, g.center().y() + arrowDim1);
        pp.closeSubpath();
        break;

    case GettingStartedTip::Arrow::RightCenter:
        pp.moveTo(g.right(), g.center().y());
        pathLineBy(pp, -arrowDim1, arrowDim1);
        pp.lineTo(g.right() - arrowDim1, g.bottom());
        pp.lineTo(g.left(), g.bottom());
        pp.lineTo(g.left(), g.top());
        pp.lineTo(g.right() - arrowDim1, g.top());
        pp.lineTo(g.right() - arrowDim1, g.center().y() - arrowDim1);
        pp.closeSubpath();
        break;


    case GettingStartedTip::Arrow::LeftTop:
        pp.moveTo(0.0, g.top() + arrowDim1);
        pathLineBy(pp, arrowDim1, -arrowDim1);
        pp.lineTo(g.right(), g.top());
        pp.lineTo(g.right(), g.bottom());
        pp.lineTo(g.left() + arrowDim1, g.bottom());
        pp.lineTo(g.left() + arrowDim1, -g.left() + arrowDim1);
        pp.closeSubpath();
        break;

    case GettingStartedTip::Arrow::NoArrow:
        pp.moveTo(0.0, g.top());
        pp.lineTo(g.right(), g.top());
        pp.lineTo(g.right(), g.bottom());
        pp.lineTo(0.0, g.bottom());
        pp.closeSubpath();
        break;


    default:
        qWarning() << Q_FUNC_INFO << "unhandled:" << arrow;
        break;
    }

    return pp;
}


} // of anonymous

int TipBackgroundBox::arrowSideOffset()
{
    return static_cast<int>(arrowEndOffset);
}

int TipBackgroundBox::arrowHeight()
{
    return static_cast<int>(arrowDim1);
}

TipBackgroundBox::TipBackgroundBox(QQuickItem* pr) :
    QQuickPaintedItem(pr)
{
}

GettingStartedTip::Arrow TipBackgroundBox::arrowPosition() const
{
    return _arrow;
}

QColor TipBackgroundBox::borderColor() const
{
    return _borderColor;
}

int TipBackgroundBox::borderWidth() const
{
    return _borderWidth;
}

QColor TipBackgroundBox::fill() const
{
    return _fill;
}

void TipBackgroundBox::setArrowPosition(GettingStartedTip::Arrow arrow)
{
    if (_arrow == arrow)
        return;

    _arrow = arrow;
    emit arrowPositionChanged(_arrow);
    update();
}

void TipBackgroundBox::setBorderColor(QColor borderColor)
{
    if (_borderColor == borderColor)
        return;

    _borderColor = borderColor;
    emit borderColorChanged(_borderColor);
    update();
}

void TipBackgroundBox::setBorderWidth(int borderWidth)
{
    if (_borderWidth == borderWidth)
        return;

    _borderWidth = borderWidth;
    emit borderWidthChanged(_borderWidth);
    update();
}

void TipBackgroundBox::setFill(QColor fill)
{
    if (_fill == fill)
        return;

    _fill = fill;
    emit fillChanged(_fill);
    update();
}

void TipBackgroundBox::paint(QPainter *painter)
{
    QPainterPath pp = pathFromArrowAndGeometry(_arrow, QRectF{0.0,0.0,width(), height()});
    painter->setBrush(_fill);
    painter->setPen(QPen{_borderColor, static_cast<double>(_borderWidth)});
    painter->drawPath(pp);
}
