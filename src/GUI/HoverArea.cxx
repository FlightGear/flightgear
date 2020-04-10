#include "HoverArea.hxx"

#include <QDebug>
#include <QQuickWindow>

HoverArea::HoverArea()
{
    connect(this, &QQuickItem::windowChanged, [this](QQuickWindow* win) {
        if (win) {
            win->installEventFilter(this);
        }
    });
}



bool HoverArea::eventFilter(QObject *sender, QEvent *event)
{
    Q_UNUSED(sender)
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        const auto local = mapFromScene(me->pos());
        const bool con = contains(local);
        if (con != m_containsMouse) {
            m_containsMouse = con;
            emit containsMouseChanged(con);
        }
    }

    return false;
}


