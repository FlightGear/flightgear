// LauncherMainWindow.hxx - GUI launcher dialog using Qt5
//
// Written by James Turner, started October 2015.
//
// Copyright (C) 2018 James Turner <james@flightgear.org>
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

#ifndef LAUNCHER_MAIN_WINDOW_HXX
#define LAUNCHER_MAIN_WINDOW_HXX

#include <QScopedPointer>
#include <QStringList>
#include <QModelIndex>
#include <QTimer>
#include <QUrl>
#include <QQuickView>


class QModelIndex;
class QQmlEngine;
class LaunchConfig;
class ViewCommandLinePage;
class QQuickItem;
class LauncherController;

class LauncherMainWindow : public QQuickView
{
    Q_OBJECT
public:
    LauncherMainWindow(bool inSimMode);
    virtual ~LauncherMainWindow();

    bool execInApp();

    bool wasRejected();

    bool event(QEvent *event) override;

private slots:
    void onQuickStatusChanged(QQuickView::Status status);

private:
    LauncherController* m_controller;

    bool checkQQC2Availability();
};

#endif // of LAUNCHER_MAIN_WINDOW_HXX
