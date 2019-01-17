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

#include "fgcanvaspath.h"

#include <cctype>

#include <QByteArrayList>
#include <QPainter>
#include <QDebug>
#include <QtMath>
#include <QPen>

#include "fgcanvaspaintcontext.h"
#include "localprop.h"
#include "canvasitem.h"

#include "private/qtriangulator_p.h" // private QtGui header
#include "private/qtriangulatingstroker_p.h" // private QtGui header
#include "private/qvectorpath_p.h" // private QtGui header

#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>

class PathQuickItem : public CanvasItem
{
    Q_OBJECT

    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(QPen stroke READ stroke WRITE setStroke NOTIFY strokeChanged)

public:
    PathQuickItem(QQuickItem* parent)
        : CanvasItem(parent)
    {
        setFlag(ItemHasContents);
    }

    void setPath(QPainterPath pp)
    {
        m_path = pp;

        QRectF pathBounds = pp.boundingRect();
        setImplicitSize(pathBounds.width(), pathBounds.height());

        update(); // request a paint node update
    }

    QSGNode* updateRealPaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData *) override
    {
        if (m_path.isEmpty()) {
            return nullptr;
        }

        delete oldNode;
        QSGGeometryNode* fillGeom = nullptr;
        QSGGeometryNode* strokeGeom = nullptr;

        if (m_fillColor.isValid()) {
            // TODO: compute LOD for qTriangulate based on world transform
            QTransform transform;
            QTriangleSet triangles = qTriangulate(m_path, transform);

            int indexType = GL_UNSIGNED_SHORT;
            if (triangles.indices.type() == QVertexIndexVector::UnsignedShort) {
                // default is fine
            } else if (triangles.indices.type() == QVertexIndexVector::UnsignedInt) {
                indexType = GL_UNSIGNED_INT;
            } else {
                qFatal("Unsupported triangle index type");
            }

            QSGGeometry* sgGeom = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(),
                                                  triangles.vertices.size() >> 1,
                                                  triangles.indices.size(),
                                                  indexType);

            sgGeom->setIndexDataPattern(QSGGeometry::StaticPattern);
            sgGeom->setDrawingMode(GL_TRIANGLES);

            //
            QSGGeometry::Point2D *points = sgGeom->vertexDataAsPoint2D();
            for (int v=0; v < triangles.vertices.size(); ) {
                const float vx = triangles.vertices.at(v++);
                const float vy = triangles.vertices.at(v++);
                (points++)->set(vx, vy);
            }

            if (triangles.indices.type() == QVertexIndexVector::UnsignedShort) {
                quint16* indices = sgGeom->indexDataAsUShort();
                ::memcpy(indices, triangles.indices.data(), sizeof(unsigned short) * triangles.indices.size());
            } else {
                quint32* indices = sgGeom->indexDataAsUInt();
                ::memcpy(indices, triangles.indices.data(), sizeof(quint32) * triangles.indices.size());
            }

            // create the node now, pretty trivial
            fillGeom = new QSGGeometryNode;
            fillGeom->setGeometry(sgGeom);
            fillGeom->setFlag(QSGNode::OwnsGeometry);

            QSGFlatColorMaterial* mat = new QSGFlatColorMaterial();
            mat->setColor(m_fillColor);
            fillGeom->setMaterial(mat);
            fillGeom->setFlag(QSGNode::OwnsMaterial);
        }

        if (m_stroke.style() != Qt::NoPen) {
            const QVectorPath& vp = qtVectorPathForPath(m_path);
            QRectF clipBounds;
            QTriangulatingStroker ts;
            QPainter::RenderHints renderHints;

            if (m_stroke.style() == Qt::SolidLine) {
                ts.process(vp, m_stroke, clipBounds, renderHints);
    #if 0
                inline int vertexCount() const { return m_vertices.size(); }
                inline const float *vertices() const { return m_vertices.data(); }

    #endif
            } else {
                QDashedStrokeProcessor dasher;
                dasher.process(vp, m_stroke, clipBounds, renderHints);

                QVectorPath dashStroke(dasher.points(),
                                       dasher.elementCount(),
                                       dasher.elementTypes(),
                                       renderHints);

                ts.process(dashStroke, m_stroke, clipBounds, renderHints);
            }

            QSGGeometry* sgGeom = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(),
                                                  ts.vertexCount() >> 1);
            sgGeom->setVertexDataPattern(QSGGeometry::StaticPattern);
            sgGeom->setDrawingMode(GL_TRIANGLE_STRIP);

            QSGGeometry::Point2D *points = sgGeom->vertexDataAsPoint2D();
            const float* vPtr = ts.vertices();
            for (int v=0; v < ts.vertexCount(); v += 2) {
                const float vx = *vPtr++;
                const float vy = *vPtr++;
                (points++)->set(vx, vy);
            }

            // create the node now, pretty trivial
            strokeGeom = new QSGGeometryNode;
            strokeGeom->setGeometry(sgGeom);
            strokeGeom->setFlag(QSGNode::OwnsGeometry);

            QSGFlatColorMaterial* mat = new QSGFlatColorMaterial();
            mat->setColor(m_stroke.color());
            strokeGeom->setMaterial(mat);
            strokeGeom->setFlag(QSGNode::OwnsMaterial);
        }

        if (fillGeom && strokeGeom) {
            QSGNode* groupNode = new QSGNode;
            groupNode->appendChildNode(fillGeom);
            groupNode->appendChildNode(strokeGeom);
            return groupNode;
        } else if (fillGeom) {
            return fillGeom;
        }

        return strokeGeom;
    }

    QColor fillColor() const
    {
        return m_fillColor;
    }

    QPen stroke() const
    {
        return m_stroke;
    }

public slots:
    void setFillColor(QColor fillColor)
    {
        if (m_fillColor == fillColor)
            return;

        m_fillColor = fillColor;
        emit fillColorChanged(fillColor);
        update();
    }

    void setStroke(QPen stroke)
    {
        if (m_stroke == stroke)
            return;

        m_stroke = stroke;
        emit strokeChanged(stroke);
        update();
    }

signals:
    void fillColorChanged(QColor fillColor);

    void strokeChanged(QPen stroke);

protected:
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override
    {
        QQuickItem::geometryChanged(newGeometry, oldGeometry);
        update();
    }

    QRectF boundingRect() const override
    {
        if ((width() == 0.0) || (height() == 0.0)) {
            return QRectF(0.0, 0.0, implicitWidth(), implicitHeight());
        }

        return QQuickItem::boundingRect();
    }

private:
    QPainterPath m_path;
    QColor m_fillColor;
    QPen m_stroke;
};

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

void FGCanvasPath::dumpElement()
{
    qDebug() << "Path: at " << _propertyRoot->path();
}

void FGCanvasPath::doPaint(FGCanvasPaintContext *context) const
{
    context->painter()->setPen(_stroke);

    switch (_paintType) {
    case Rect:
        context->painter()->drawRect(_rect);
        break;
    case RoundRect:
        context->painter()->drawRoundRect(_rect, _roundRectRadius.width(), _roundRectRadius.height());
        break;

    case Path:
        context->painter()->drawPath(_painterPath);
        break;
    }

    if (isHighlighted()) {
        context->painter()->setPen(QPen(Qt::red, 1));
        context->painter()->setBrush(Qt::NoBrush);
        switch (_paintType) {
        case Rect:
        case RoundRect:
            context->painter()->drawRect(_rect);
            break;

        case Path:
            context->painter()->drawRect(_painterPath.boundingRect());
            break;
        }
    }

}

void FGCanvasPath::doPolish()
{
    if (_pathDirty) {
        rebuildPath();
        if (_quickPath) {
            _quickPath->setPath(_painterPath);
        }
        _pathDirty = false;
    }

    if (_penDirty) {
        rebuildPen();
        if (_quickPath) {
            _quickPath->setStroke(_stroke);
        }
        _penDirty = false;
    }

    if (_quickPath) {
        _quickPath->setFillColor(fillColor());
    }
}

void FGCanvasPath::markStyleDirty()
{
    _penDirty = true;
}

CanvasItem *FGCanvasPath::createQuickItem(QQuickItem *parent)
{
    _quickPath = new PathQuickItem(parent);
    _quickPath->setPath(_painterPath);
    _quickPath->setStroke(_stroke);
    _quickPath->setAntialiasing(true);
    return _quickPath;
}

CanvasItem *FGCanvasPath::quickItem() const
{
    return _quickPath;
}

void FGCanvasPath::doDestroy()
{
    delete _quickPath;
}

void FGCanvasPath::markPathDirty()
{
    _pathDirty = true;
    requestPolish();
}

void FGCanvasPath::markStrokeDirty()
{
    _penDirty = true;
    requestPolish();
}

bool FGCanvasPath::onChildAdded(LocalProp *prop)
{
    if (FGCanvasElement::onChildAdded(prop)) {
        return true;
    }

    if ((prop->name() == "cmd") || (prop->name() == "coord") || (prop->name() == "svg")) {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasPath::markPathDirty);
        return true;
    }

    if (prop->name() == "rect") {
        _isRect = true;
        connect(prop, &LocalProp::childAdded, this, &FGCanvasPath::onChildAdded);
        return true;
    }

    // handle rect property changes
    if (prop->parent()->name() == "rect") {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasPath::markPathDirty);
        return true;
    }

    if (prop->name().startsWith("border-")) {
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

    qWarning() << "path saw unrecognized child:" << prop->name() << prop->index();
    return false;
}

bool FGCanvasPath::onChildRemoved(LocalProp* prop)
{
    if (FGCanvasElement::onChildRemoved(prop)) {
        return true;
    }

    const auto name = prop->name();
    if (name == "rect") {
        _isRect = false;
        markPathDirty();
        return true;
    }

    if ((name == "cmd") || (name == "coord") || (name == "svg")) {
        markPathDirty();
        return true;
    }

    if ((name == "cmd-geo") || (name == "coord-geo")) {
        // ignored
        return true;
    }

    if (name.startsWith("stroke")) {
        markStrokeDirty();
        return true;
    }

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

    if (_isRect) {
        rebuildFromRect(commands, coords);
    } else if (_propertyRoot->hasChild("svg")) {
        if (!rebuildFromSVGData(commands, coords)) {
            qWarning() << "failed to parse SVG path data" << _propertyRoot->value("svg", QVariant());
        }
    } else {
        for (QVariant v : _propertyRoot->valuesOfChildren("coord")) {
            coords.push_back(v.toFloat());
        }

        for (QVariant v : _propertyRoot->valuesOfChildren("cmd")) {
            commands.push_back(v.toInt());
        }
    }

    rebuildPathFromCommands(commands, coords);
}

QByteArrayList splitSVGPathData(QByteArray d)
{
    QByteArrayList result;
    size_t pos = 0;
    std::string strData(d.data());
    const char* seperators = "\n\r\t ,";
    size_t startPos = strData.find_first_not_of(seperators, 0);
    for(;;)
    {
        pos = strData.find_first_of(seperators, startPos);
        if (pos == std::string::npos) {
            result.push_back(QByteArray::fromStdString(strData.substr(startPos)));
            break;
        }
        result.push_back(QByteArray::fromStdString(strData.substr(startPos, pos - startPos)));
        startPos = strData.find_first_not_of(seperators, pos);
        if (startPos == std::string::npos) {
            break;
        }
    }

    return result;
}

bool hasComplexBorderRadius(const LocalProp* prop)
{
    for (auto childProp : prop->children()) {
        QByteArray name = childProp->name();
        if (!name.startsWith("border-") || !name.endsWith("-radius")) {
            continue;
        }

        if (name != "border-radius") {
            return true;
        }
    } // of child prop iteration

    return false;
}

bool FGCanvasPath::rebuildFromRect(std::vector<int>& commands, std::vector<float>& coords) const
{
    LocalProp* rectProp = _propertyRoot->getWithPath("rect");
    if (hasComplexBorderRadius(_propertyRoot)) {
        // build a full path
        qWarning() << Q_FUNC_INFO << "implement me";
        _paintType = Path;
    } else {
        float top = rectProp->value("top", 0.0).toFloat();
        float left = rectProp->value("left", 0.0).toFloat();
        float width = rectProp->value("width", 0.0).toFloat();
        float height = rectProp->value("height", 0.0).toFloat();

        if (rectProp->hasChild("right")) {
            width = rectProp->value("right", 0.0).toFloat() - left;
        }

        if (rectProp->hasChild("bottom")) {
            height = rectProp->value("bottom", 0.0).toFloat() - top;
        }

        _rect = QRectF(left, top, width, height);

        if (_propertyRoot->hasChild("border-radius")) {
            // round-rect
            float xR = _propertyRoot->value("border-radius", 0.0).toFloat();
            float yR = xR;
            if (_propertyRoot->hasChild("border-radius[1]")) {
                yR = _propertyRoot->value("border-radius[1]", 0.0).toFloat();
            }

            _roundRectRadius = QSizeF(xR, yR);
            _paintType = RoundRect;
        } else {
            // simple rect
            _paintType = Rect;
        }
    }

    return true;
}

bool FGCanvasPath::rebuildFromSVGData(std::vector<int>& commands, std::vector<float>& coords) const
{
    QByteArrayList tokens = splitSVGPathData(_propertyRoot->value("svg", QVariant()).toByteArray());
    PathCommands currentCommand = PathClose;
    bool isRelative = false;
    int numCoordsTokens = 0;
    const int totalTokens = tokens.size();

    for (int index = 0; index < totalTokens; /* no increment */) {
        const QByteArray& tk = tokens.at(index);
        if ((tk.length() == 1) && std::isalpha(tk.at(0))) {
            // new command token
            const char svgCommand = std::toupper(tk.at(0));
            isRelative = std::islower(tk.at(0));

            switch (svgCommand) {
            case 'Z':
                currentCommand = PathClose;
                numCoordsTokens = 0;
                break;
            case 'M':
                currentCommand = PathMoveTo;
                numCoordsTokens = 2;
                break;
            case 'L':
                currentCommand = PathLineTo;
                numCoordsTokens = 2;
                break;
            case 'H':
                currentCommand = PathHLineTo;
                numCoordsTokens = 1;
                break;
            case 'V':
                currentCommand = PathVLineTo;
                numCoordsTokens = 1;
                break;
            case 'C':
                currentCommand = PathCubicTo;
                numCoordsTokens = 6;
                break;
            case 'S':
                currentCommand = PathSmoothCubicTo;
                numCoordsTokens = 4;
                break;
            case 'Q':
                currentCommand = PathQuadTo;
                numCoordsTokens = 4;
                break;
            case 'T':
                currentCommand = PathSmoothQuadTo;
                numCoordsTokens = 2;
                break;
            case 'A':
                currentCommand = PathShortCWArc;
                numCoordsTokens = 0; // handled specially below
                break;
            default:
                qWarning() << "unrecognized SVG command" << svgCommand;
                return false;
            }
            ++index;
        }

        switch (currentCommand) {
        case PathMoveTo:
            commands.push_back(PathMoveTo | (isRelative ? 1 : 0));
            currentCommand = PathLineTo;
            break;

        case PathClose:
        case PathLineTo:
        case PathHLineTo:
        case PathVLineTo:
        case PathQuadTo:
        case PathCubicTo:
        case PathSmoothQuadTo:
        case PathSmoothCubicTo:
            commands.push_back(currentCommand | (isRelative ? 1 : 0));
            break;

        case PathShortCWArc:
        case PathShortCCWArc:
        case PathLongCWArc:
        case PathLongCCWArc:
        {
            // decode the actual arc type
            coords.push_back(tokens.at(index++).toFloat()); // rx
            coords.push_back(tokens.at(index++).toFloat()); // ry
            coords.push_back(tokens.at(index++).toFloat()); // x-axis rotation

            const bool isLargeArc = (tokens.at(index++).toInt() != 0); // large-angle
            const bool isCCW = (tokens.at(index++).toInt() != 0); // sweep-flag
            if (isLargeArc) {
                commands.push_back(isCCW ? PathLongCCWArc : PathLongCWArc);
            } else {
                commands.push_back(isCCW ? PathShortCCWArc : PathShortCWArc);
            }

            if (isRelative) {
                commands.back() |= 1;
            }

            coords.push_back(tokens.at(index++).toFloat());
            coords.push_back(tokens.at(index++).toFloat());
            break;
        }

        default:
            qWarning() << "invalid path command";
            return false;
        } // of current command switch

        // copy over tokens according to the active command.
        if (index + numCoordsTokens > totalTokens) {
            qWarning() << "insufficent remaining tokens for SVG command" << currentCommand;
            qWarning() << index << numCoordsTokens << totalTokens;
            return false;
        }

        for (int c = 0; c < numCoordsTokens; ++c) {
            coords.push_back(tokens.at(index + c).toFloat());
        }

        index += numCoordsTokens;
    } // of tokens iteration

    return true;
}

void FGCanvasPath::rebuildPathFromCommands(const std::vector<int>& commands, const std::vector<float>& coords) const
{
    QPainterPath newPath;
    const float* coord = coords.data();
    QPointF lastControlPoint; // for smooth cubics / quadric
    size_t currentCoord = 0;

    for (int cmd : commands) {
        bool isRelative = cmd & 0x1;
        const int op = cmd & ~0x1;
        const int cmdIndex = op >> 1;
        const qreal baseX = isRelative ? newPath.currentPosition().x() : 0.0f;
        const qreal baseY = isRelative ? newPath.currentPosition().y() : 0.0f;

        if ((currentCoord + CoordsPerCommand[cmdIndex]) > coords.size()) {
            qWarning() << "insufficient path data" << currentCoord << cmdIndex << CoordsPerCommand[cmdIndex] << coords.size();
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
    } else if (s == "square")  {
        return Qt::SquareCap;
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

#include "fgcanvaspath.moc"
