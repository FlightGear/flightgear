#include "fgcanvaspath.h"

#include <QPainter>
#include <QDebug>
#include <QtMath>

#include "fgcanvaspaintcontext.h"
#include "localprop.h"

static void pathArcSegment(QPainterPath &path,
                           qreal xc, qreal yc,
                           qreal th0, qreal th1,
                           qreal rx, qreal ry, qreal xAxisRotation)
{
    qreal sinTh, cosTh;
    qreal a00, a01, a10, a11;
    qreal x1, y1, x2, y2, x3, y3;
    qreal t;
    qreal thHalf;

    sinTh = qSin(xAxisRotation * (M_PI / 180.0));
    cosTh = qCos(xAxisRotation * (M_PI / 180.0));

    a00 =  cosTh * rx;
    a01 = -sinTh * ry;
    a10 =  sinTh * rx;
    a11 =  cosTh * ry;

    thHalf = 0.5 * (th1 - th0);
    t = (8.0 / 3.0) * qSin(thHalf * 0.5) * qSin(thHalf * 0.5) / qSin(thHalf);
    x1 = xc + qCos(th0) - t * qSin(th0);
    y1 = yc + qSin(th0) + t * qCos(th0);
    x3 = xc + qCos(th1);
    y3 = yc + qSin(th1);
    x2 = x3 + t * qSin(th1);
    y2 = y3 - t * qCos(th1);

    path.cubicTo(a00 * x1 + a01 * y1, a10 * x1 + a11 * y1,
                 a00 * x2 + a01 * y2, a10 * x2 + a11 * y2,
                 a00 * x3 + a01 * y3, a10 * x3 + a11 * y3);
}

// the arc handling code underneath is from XSVG (BSD license)
/*
 * Copyright  2002 USC/Information Sciences Institute
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Information Sciences Institute not be used in advertising or
 * publicity pertaining to distribution of the software without
 * specific, written prior permission.  Information Sciences Institute
 * makes no representations about the suitability of this software for
 * any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * INFORMATION SCIENCES INSTITUTE DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL INFORMATION SCIENCES
 * INSTITUTE BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
static void pathArc(QPainterPath &path,
                    qreal               rx,
                    qreal               ry,
                    qreal               x_axis_rotation,
                    int         large_arc_flag,
                    int         sweep_flag,
                    qreal               x,
                    qreal               y,
                    qreal curx, qreal cury)
{
    qreal sin_th, cos_th;
    qreal a00, a01, a10, a11;
    qreal x0, y0, x1, y1, xc, yc;
    qreal d, sfactor, sfactor_sq;
    qreal th0, th1, th_arc;
    int i, n_segs;
    qreal dx, dy, dx1, dy1, Pr1, Pr2, Px, Py, check;

    rx = qAbs(rx);
    ry = qAbs(ry);

    sin_th = qSin(x_axis_rotation * (M_PI / 180.0));
    cos_th = qCos(x_axis_rotation * (M_PI / 180.0));

    dx = (curx - x) / 2.0;
    dy = (cury - y) / 2.0;
    dx1 =  cos_th * dx + sin_th * dy;
    dy1 = -sin_th * dx + cos_th * dy;
    Pr1 = rx * rx;
    Pr2 = ry * ry;
    Px = dx1 * dx1;
    Py = dy1 * dy1;
    /* Spec : check if radii are large enough */
    check = Px / Pr1 + Py / Pr2;
    if (check > 1) {
        rx = rx * qSqrt(check);
        ry = ry * qSqrt(check);
    }

    a00 =  cos_th / rx;
    a01 =  sin_th / rx;
    a10 = -sin_th / ry;
    a11 =  cos_th / ry;
    x0 = a00 * curx + a01 * cury;
    y0 = a10 * curx + a11 * cury;
    x1 = a00 * x + a01 * y;
    y1 = a10 * x + a11 * y;
    /* (x0, y0) is current point in transformed coordinate space.
       (x1, y1) is new point in transformed coordinate space.

       The arc fits a unit-radius circle in this space.
    */
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0) sfactor_sq = 0;
    sfactor = qSqrt(sfactor_sq);
    if (sweep_flag == large_arc_flag) sfactor = -sfactor;
    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    /* (xc, yc) is center of the circle. */

    th0 = qAtan2(y0 - yc, x0 - xc);
    th1 = qAtan2(y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep_flag)
        th_arc += 2 * M_PI;
    else if (th_arc > 0 && !sweep_flag)
        th_arc -= 2 * M_PI;

    n_segs = qCeil(qAbs(th_arc / (M_PI * 0.5 + 0.001)));

    for (i = 0; i < n_segs; i++) {
        pathArcSegment(path, xc, yc,
                       th0 + i * th_arc / n_segs,
                       th0 + (i + 1) * th_arc / n_segs,
                       rx, ry, x_axis_rotation);
    }
}

///////////////////////////////////////////////////////////////////////////////


FGCanvasPath::FGCanvasPath(FGCanvasGroup* pr, LocalProp* prop) :
    FGCanvasElement(pr, prop)
{

}

void FGCanvasPath::doPaint(FGCanvasPaintContext *context) const
{
    if (_pathDirty) {
        rebuildPath();
        _pathDirty = false;
    }

    if (_penDirty) {
        rebuildPen();
        _penDirty = false;
    }

    context->painter()->setPen(_stroke);
    context->painter()->drawPath(_painterPath);
}

void FGCanvasPath::markStyleDirty()
{
    _penDirty = true;
}

void FGCanvasPath::markPathDirty()
{
    _pathDirty = true;
}

void FGCanvasPath::markStrokeDirty()
{
    _penDirty = true;
}

bool FGCanvasPath::onChildAdded(LocalProp *prop)
{
    if (FGCanvasElement::onChildAdded(prop)) {
        return true;
    }

    if ((prop->name() == "cmd") || (prop->name() == "coord")) {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasPath::markPathDirty);
        return true;
    }

    if ((prop->name() == "cmd-geo") || (prop->name() == "coord-geo")) {
        // ignore for now, we let the server-side transform down to cartesian.
        // if we move that work to client side we could skip sending the cmd/coord data
        return true;
    }

    if (prop->name().startsWith("stroke")) {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasPath::markStrokeDirty);
        return true;
    }

    qDebug() << "path saw child:" << prop->name() << prop->index();
    return false;
}

typedef enum
{
    PathClose                               = ( 0 << 1),
    PathMoveTo                              = ( 1 << 1),
    PathLineTo                              = ( 2 << 1),
    PathHLineTo                             = ( 3 << 1),
    PathVLineTo                             = ( 4 << 1),
    PathQuadTo                              = ( 5 << 1),
    PathCubicTo                             = ( 6 << 1),
    PathSmoothQuadTo                        = ( 7 << 1),
    PathSmoothCubicTo                       = ( 8 << 1),
    PathShortCCWArc                         = ( 9 << 1),
    PathShortCWArc                          = (10 << 1),
    PathLongCCWArc                          = (11 << 1),
    PathLongCWArc                           = (12 << 1)
} PathCommands;

static const quint8 CoordsPerCommand[] = {
    0, /* VG_CLOSE_PATH */
    2, /* VG_MOVE_TO */
    2, /* VG_LINE_TO */
    1, /* VG_HLINE_TO */
    1, /* VG_VLINE_TO */
    4, /* VG_QUAD_TO */
    6, /* VG_CUBIC_TO */
    2, /* VG_SQUAD_TO */
    4, /* VG_SCUBIC_TO */
    5, /* VG_SCCWARC_TO */
    5, /* VG_SCWARC_TO */
    5, /* VG_LCCWARC_TO */
    5  /* VG_LCWARC_TO */
};

void FGCanvasPath::rebuildPath() const
{
    std::vector<float> coords;
    std::vector<int> commands;

    for (QVariant v : _propertyRoot->valuesOfChildren("coord")) {
        coords.push_back(v.toFloat());
    }

    for (QVariant v : _propertyRoot->valuesOfChildren("cmd")) {
        commands.push_back(v.toInt());
    }

    QPainterPath newPath;
    float* coord = coords.data();
    QPointF lastControlPoint; // for smooth cubics / quadric
    size_t currentCoord = 0;

    for (int cmd : commands) {
        bool isRelative = cmd & 0x1;
        const int op = cmd & ~0x1;
        const int cmdIndex = op >> 1;
        const float baseX = isRelative ? newPath.currentPosition().x() : 0.0f;
        const float baseY = isRelative ? newPath.currentPosition().y() : 0.0f;

        if ((currentCoord + CoordsPerCommand[cmdIndex]) > coords.size()) {
            qWarning() << "insufficient path data";
            break;
        }

        switch (op) {
        case PathClose:
            newPath.closeSubpath();
            break;
        case PathMoveTo:
            newPath.moveTo(coord[0] + baseX, coord[1] + baseY);
            break;
        case PathLineTo:
            newPath.lineTo(coord[0] + baseX, coord[1] + baseY);
            break;
        case PathHLineTo:
            newPath.lineTo(coord[0] + baseX, newPath.currentPosition().y());
            break;
        case PathVLineTo:
            newPath.lineTo(newPath.currentPosition().x(), coord[0] + baseY);
            break;
        case PathQuadTo:
            newPath.quadTo(coord[0] + baseX, coord[1] + baseY,
                           coord[2] + baseX, coord[3] + baseY);
            lastControlPoint = QPointF(coord[0] + baseX, coord[1] + baseY);
            break;
        case PathCubicTo:
            newPath.cubicTo(coord[0] + baseX, coord[1] + baseY,
                            coord[2] + baseX, coord[3] + baseY,
                            coord[4] + baseX, coord[5] + baseY);
            lastControlPoint = QPointF(coord[2] + baseX, coord[3] + baseY);
            break;

        case PathSmoothQuadTo: {
            QPointF smoothControlPoint = (newPath.currentPosition() - lastControlPoint) * 2.0;
            newPath.quadTo(smoothControlPoint.x(), smoothControlPoint.y(),
                           coord[0] + baseX, coord[1] + baseY);
            lastControlPoint = smoothControlPoint;
            break;
        }

        case PathSmoothCubicTo: {
            QPointF smoothControlPoint = (newPath.currentPosition() - lastControlPoint) * 2.0;
            newPath.cubicTo(smoothControlPoint.x(), smoothControlPoint.y(),
                           coord[0] + baseX, coord[1] + baseY,
                           coord[2] + baseX, coord[3] + baseY);
            lastControlPoint = QPointF(coord[0] + baseX, coord[1] + baseY);
            break;
        }

#if 0
            qreal               rx,
            qreal               ry,
            qreal               x_axis_rotation,
            int         large_arc_flag,
            int         sweep_flag,
            qreal               x,
            qreal               y,
            qreal curx, qreal cury)
#endif
        case PathLongCCWArc:
        case PathLongCWArc:
            pathArc(newPath, coord[0], coord[1], coord[2],
                    true, (op == PathLongCCWArc),
                    coord[3] + baseX, coord[4] + baseY,
                    newPath.currentPosition().x(), newPath.currentPosition().y());
            break;

        case PathShortCCWArc:
        case PathShortCWArc:
            pathArc(newPath, coord[0], coord[1], coord[2],
                    false, (op == PathShortCCWArc),
                    coord[3] + baseX, coord[4] + baseY,
                    newPath.currentPosition().x(), newPath.currentPosition().y());
            break;

        default:
            qWarning() << "uhandled path command type:" << cmdIndex;
        }

        if ((op < PathQuadTo) || (op > PathSmoothCubicTo)) {
            lastControlPoint = newPath.currentPosition();
        }

        coord += CoordsPerCommand[cmdIndex];
        currentCoord += CoordsPerCommand[cmdIndex];
    } // of commands iteration

//    qDebug() << _propertyRoot->path() << "path" << newPath;
    _painterPath = newPath;
}

static Qt::PenCapStyle qtCapFromCanvas(QString s)
{
    if (s.isEmpty() || (s == "butt")) {
        return Qt::FlatCap;
    } else if (s == "round") {
        return Qt::RoundCap;
    } else {
        qDebug() << Q_FUNC_INFO << s;
    }
    return Qt::FlatCap;
}

static Qt::PenJoinStyle qtJoinFromCanvas(QString s)
{
    if (s.isEmpty() || (s == "miter")) {
        return Qt::MiterJoin;
    } else if (s == "round") {
        return Qt::RoundJoin;
    } else {
        qDebug() << Q_FUNC_INFO << s;
    }
    return Qt::MiterJoin;
}

static QVector<qreal> qtPenDashesFromCanvas(QString s, double penWidth)
{
    QVector<qreal> result;
    Q_FOREACH(QString v, s.split(',')) {
        result.push_back(v.toFloat() / penWidth);
    }

    // https://developer.mozilla.org/en/docs/Web/SVG/Attribute/stroke-dasharray
    // odd number = double it
    if ((result.size() % 2) == 1) {
        result += result;
    }
    return result;
}

void FGCanvasPath::rebuildPen() const
{
    QPen p;

    QVariant strokeColor = getCascadedStyle("stroke");
    p.setColor(parseColorValue(strokeColor));

    p.setWidthF(getCascadedStyle("stroke-width", 1.0).toFloat());
    p.setCapStyle(qtCapFromCanvas(_propertyRoot->value("stroke-linecap", QString()).toString()));
    p.setJoinStyle(qtJoinFromCanvas(_propertyRoot->value("stroke-linejoin", QString()).toString()));

    QString dashArray = _propertyRoot->value("stroke-dasharray", QVariant()).toString();
    if (!dashArray.isEmpty() && (dashArray != "none")) {
        p.setDashPattern(qtPenDashesFromCanvas(dashArray, p.widthF()));
    }

    _stroke = p;
}