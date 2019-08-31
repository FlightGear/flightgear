#include "WindowData.h"

#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

#include "jsonutils.h"

WindowData::WindowData(QObject *parent) : QObject(parent)
{

}

QJsonObject WindowData::saveState() const
{
    QJsonObject json;
    json["rect"] = rectToJsonArray(m_windowRect);
    if (!m_screenName.isEmpty()) {
        json["screen"] = m_screenName;
    }
    if (!m_title.isEmpty()) {
        json["title"] = m_title;
    }
    // support frameless option here?
    json["state"] = static_cast<int>(m_state);
    return json;
}

bool WindowData::restoreState(QJsonObject state)
{
    m_windowRect = jsonArrayToRect(state.value("rect").toArray());
    emit windowRectChanged(m_windowRect);

    if (state.contains("screen")) {
        m_screenName = state.value("screen").toString();
    } else {
        m_screenName.clear();
    }

    if (state.contains("title")) {
        m_title = state.value("title").toString();
    }

    if (state.contains("state")) {
        m_state = static_cast<Qt::WindowState>(state.value("state").toInt());
    }

    return true;
}

QRect WindowData::windowRect() const
{
    return m_windowRect;
}

QScreen *WindowData::screen() const
{
    if (m_screenName.isEmpty())
        return nullptr;

    QStringList screenNames;
    Q_FOREACH(auto s, qApp->screens()) {
        if (s->name() == m_screenName) {
            return s;
        }
        screenNames.append(s->name());
    }

    qWarning() << "couldn't find a screen with name:" << m_screenName;
    qWarning() << "Available screens:" << screenNames.join(", ");
    return nullptr;
}

void WindowData::setWindowState(Qt::WindowState ws)
{
    m_state = ws;
}

void WindowData::setWindowRect(QRect windowRect)
{
    if (m_windowRect == windowRect)
        return;

    m_windowRect = windowRect;
    emit windowRectChanged(m_windowRect);
}
