// Written by James Turner, started October 2020
//
// Copyright (C) 2020 James Turner <james@flightgear.org>
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

#pragma once

#include <QJSValue>
#include <QObject>
#include <QUrl>

// forward decls
class QAbstractItemModel;
class QJSEngine;
class QQmlEngine;

class LauncherNotificationsController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel* active READ notifications CONSTANT)
public:
    LauncherNotificationsController(QObject* pr, QQmlEngine* qmlEngine);
    ~LauncherNotificationsController();

    static LauncherNotificationsController* instance();

    QAbstractItemModel* notifications() const;

    Q_INVOKABLE QJSValue argsForIndex(int index) const;

    QJSEngine* jsEngine();
public slots:
    void dismissIndex(int index);

    void postNotification(QString id, QUrl source, QJSValue args = {});

signals:


private:
    class NotificationsModel;

    NotificationsModel* _model = nullptr;
    QQmlEngine* _qmlEngine = nullptr;
};
