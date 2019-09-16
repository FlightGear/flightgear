// StackController.hxx - manage a stack of QML items
//
// Written by James Turner, started February 2019
//
// Copyright (C) 2019 James Turner <james@flightgear.org>
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


#ifndef FG_GUI_STACK_CONTROLLER_HXX
#define FG_GUI_STACK_CONTROLLER_HXX

#include <QObject>
#include <QUrl>
#include <QVector>

class StackController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl currentPageSource READ currentPageSource NOTIFY currentPageChanged)

    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY currentPageChanged)

    Q_PROPERTY(QString currentPageTitle READ currentPageTitle NOTIFY currentPageChanged)
    Q_PROPERTY(QString previousPageTitle READ previousPageTitle NOTIFY currentPageChanged)

public:
    StackController();

    Q_INVOKABLE void push(QUrl page, QString title);
    Q_INVOKABLE void pop();
    Q_INVOKABLE void replace(QUrl url, QString title);

    QUrl currentPageSource() const;

    bool canGoBack() const;
    QString currentPageTitle() const;
    QString previousPageTitle() const;

signals:
    void currentPageChanged();

private:
    QVector<QUrl> m_stack;
    QVector<QString> m_titles;
};

#endif // FG_GUI_STACK_CONTROLLER_HXX
