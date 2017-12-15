#include "PopupWindowTracker.hxx"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QWindow>

PopupWindowTracker::PopupWindowTracker(QObject *parent) : QObject(parent)
{

}

PopupWindowTracker::~PopupWindowTracker()
{
    if (m_window) {
        qApp->removeEventFilter(this);
    }
}


void PopupWindowTracker::setWindow(QWindow *window)
{
    if (m_window == window)
        return;

    if (m_window) {
        qApp->removeEventFilter(this);
    }

    m_window = window;

    if (m_window) {
        qApp->installEventFilter(this);
    }

    emit windowChanged(m_window);
}

bool PopupWindowTracker::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_window)
        return false;

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        QPoint windowPos = m_window->mapFromGlobal(me->globalPos());
    }

    // also check for app loosing focus

    return false;
}
