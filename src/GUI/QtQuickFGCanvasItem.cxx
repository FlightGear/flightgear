//
//  QtQuickFGCanvasItem.cxx
//
//  Copyright (C) 2017 James Turner <zakalawe@mac.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include "QtQuickFGCanvasItem.hxx"

#include <QDebug>
#include <QSGSimpleTextureNode>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/events/MouseEvent.hxx>
#include <simgear/debug/logstream.hxx>

#include <Main/globals.hxx>
#include <Scripting/NasalSys.hxx>

namespace sc = simgear::canvas;

namespace {

class CanvasTexture : public QSGDynamicTexture
{
public:
    CanvasTexture(sc::CanvasPtr canvas)
    {
        _manager = globals->get_subsystem<CanvasMgr>();
        setHorizontalWrapMode(QSGTexture::ClampToEdge);
        setVerticalWrapMode(QSGTexture::ClampToEdge);
    }

    bool updateTexture() override
    {
        // no-op, canvas is updated by OSG before QtQuick rendering occurs
        // this would change if we wanted QtQuick to drive Canvas
        // updating, which we don't
        // but do return true so QQ knows to repaint / re-cache
        return true;
    }

    bool hasMipmaps() const override
    {
        return false;
    }

    bool hasAlphaChannel() const override
    {
        return false; // for now, can Canvas have an alpha channel?
    }

    void bind() override
    {
        glBindTexture(GL_TEXTURE_2D, _manager->getCanvasTexId(_canvas));
    }

    int textureId() const override
    {
        return  _manager->getCanvasTexId(_canvas);
    }

    QSize textureSize() const override
    {
        SGPropertyNode* cprops = _canvas->getProps();
        return QSize(cprops->getIntValue("value[0]"),
                     cprops->getIntValue("value[1]"));
    }
private:
    CanvasMgr* _manager;
    sc::CanvasPtr _canvas;

};

} // of anonymous namespace

static int convertQtButtonToCanvas(Qt::MouseButton button)
{
    switch (button) {
    case Qt::LeftButton:    return 0;
    case Qt::MiddleButton:  return 1;
    case Qt::RightButton:   return 2;
    default:
        break;
    }

    qWarning() << Q_FUNC_INFO << "unmapped Qt mouse button" << button;
    return 0;
}

QtQuickFGCanvasItem::QtQuickFGCanvasItem(QQuickItem *parent)
{
}

QtQuickFGCanvasItem::~QtQuickFGCanvasItem()
{
    if ( _canvas ) {
        _canvas->destroy();
    }
}

QSGNode *QtQuickFGCanvasItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    QSGSimpleTextureNode* texNode = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (!texNode) {
        texNode = new QSGSimpleTextureNode;

        CanvasTexture* texture = new CanvasTexture(_canvas);
        texNode->setTexture(texture);
    }

    // or should this simply be the geometry?
    texNode->setRect(QRectF(0.0, 0.0, width(), height()));

    return texNode;
}

sc::MouseEventPtr QtQuickFGCanvasItem::convertMouseEvent(QMouseEvent* event)
{
    sc::MouseEventPtr canvasEvent(new sc::MouseEvent);
    canvasEvent->time = event->timestamp() / 1000.0;
    canvasEvent->screen_pos.set(event->screenPos().x(),
                                event->screenPos().y());
    canvasEvent->client_pos.set(event->pos().x(),
                                event->pos().y());

    // synthesise delta values
    QPointF delta = event->screenPos() - _lastMouseEventScreenPosition;
    canvasEvent->delta.set(delta.x(), delta.y());
    _lastMouseEventScreenPosition = event->screenPos();

    return canvasEvent;
}

void QtQuickFGCanvasItem::mousePressEvent(QMouseEvent *event)
{
    sc::MouseEventPtr canvasEvent = convertMouseEvent(event);
    canvasEvent->button = convertQtButtonToCanvas(event->button());
    canvasEvent->type = sc::Event::MOUSE_DOWN;
    _canvas->handleMouseEvent(canvasEvent);
}

void QtQuickFGCanvasItem::mouseReleaseEvent(QMouseEvent *event)
{
    sc::MouseEventPtr canvasEvent = convertMouseEvent(event);
    canvasEvent->button = convertQtButtonToCanvas(event->button());
    canvasEvent->type = sc::Event::MOUSE_UP;
    _canvas->handleMouseEvent(canvasEvent);
}

void QtQuickFGCanvasItem::mouseMoveEvent(QMouseEvent *event)
{
    sc::MouseEventPtr canvasEvent = convertMouseEvent(event);
    if (event->buttons() == Qt::NoButton) {
        canvasEvent->type = sc::Event::MOUSE_MOVE;
    } else {
        canvasEvent->button = convertQtButtonToCanvas(event->button());
        canvasEvent->type = sc::Event::DRAG;
    }

    _canvas->handleMouseEvent(canvasEvent);
}

void QtQuickFGCanvasItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    sc::MouseEventPtr canvasEvent = convertMouseEvent(event);
    canvasEvent->button = convertQtButtonToCanvas(event->button());
    canvasEvent->type = sc::Event::DBL_CLICK;
    _canvas->handleMouseEvent(canvasEvent);
}


void QtQuickFGCanvasItem::wheelEvent(QWheelEvent *event)
{
    sc::MouseEventPtr canvasEvent(new sc::MouseEvent);
    canvasEvent->time = event->timestamp() / 1000.0;
    canvasEvent->type = sc::Event::WHEEL;

    // TODO - check if using pixelDelta is beneficial at all.
    canvasEvent->delta.set(event->angleDelta().x(),
                           event->angleDelta().y());
}

void QtQuickFGCanvasItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    updateCanvasGeometry();
}

void QtQuickFGCanvasItem::setCanvas(QString canvas)
{
    if (_canvasName == canvas)
        return;

    if (_canvas) {
        // release it
        _canvas->destroy();
        _canvas.clear();
    }
    _canvasName = canvas;

    if (!_canvasName.isEmpty()) {
        CanvasMgr* canvasManager = globals->get_subsystem<CanvasMgr>();
        _canvas = canvasManager->createCanvas("");

        SGPropertyNode* cprops = _canvas->getProps();
        cprops->setBoolValue("render-always", true);

        initCanvasNasalModules();
        updateCanvasGeometry();
    }

    emit canvasChanged(_canvasName);
}

void QtQuickFGCanvasItem::initCanvasNasalModules()
{
#if 0
    SGPropertyNode *nasal = props->getNode("nasal");
    if( !nasal )
      return;

    FGNasalSys *nas = globals->get_subsystem<FGNasalSys>();
    if( !nas )
      SG_LOG( SG_GENERAL,
              SG_ALERT,
              "CanvasWidget: Failed to get nasal subsystem!" );

    const std::string file = std::string("__canvas:")
                           + cprops->getStringValue("name");

    SGPropertyNode *load = nasal->getNode("load");
    if( load )
    {
      const char *s = load->getStringValue();
      nas->handleCommand(module.c_str(), file.c_str(), s, cprops);
    }
#endif
}

void QtQuickFGCanvasItem::updateCanvasGeometry()
{
    SGPropertyNode* cprops = _canvas->getProps();
    cprops->setIntValue("size[0]", width());
    cprops->setIntValue("size[1]", height());
}
