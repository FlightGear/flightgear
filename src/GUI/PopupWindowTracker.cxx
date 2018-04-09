#include "PopupWindowTracker.hxx"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QWindow>
#include <QDebug>

PopupWindowTracker::PopupWindowTracker(QObject *parent) : QObject(parent)
{
    connect(qGuiApp, &QGuiApplication::applicationStateChanged,
            this, &PopupWindowTracker::onApplicationStateChanged);
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

void PopupWindowTracker::onApplicationStateChanged(Qt::ApplicationState as)
{
    if (m_window && (as != Qt::ApplicationActive)) {
        m_window->close();
        setWindow(nullptr);
    }
}

bool PopupWindowTracker::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_window)
        return false;

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        QRect globalGeometry(m_window->mapToGlobal(QPoint(0,0)), m_window->size());

        if (globalGeometry.contains(me->globalPos())) {
            // click inside the window, process as normal fall through
        } else {
            m_window->close();
            setWindow(nullptr);
            // still fall through
        }
    }

    return false;
}
