#ifndef WINDOWDATA_H
#define WINDOWDATA_H

#include <QObject>
#include <QJsonObject>
#include <QRect>

class QScreen;

class WindowData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QRect windowRect READ windowRect WRITE setWindowRect NOTIFY windowRectChanged)
public:
    explicit WindowData(QObject *parent = nullptr);

    QJsonObject saveState() const;
    bool restoreState(QJsonObject state);

    QRect windowRect() const;
    QScreen* screen() const;

    Qt::WindowState windowState() const
    { return m_state; }

    void setWindowState(Qt::WindowState ws);

    QString title() const
    { return m_title; }
signals:

    void windowRectChanged(QRect windowRect);

public slots:

    void setWindowRect(QRect windowRect);

private:
    QRect m_windowRect;
    Qt::WindowState m_state = Qt::WindowNoState;
    QString m_screenName;
    QString m_title;
};

#endif // WINDOWDATA_H
