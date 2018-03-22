// LauncherMainWindow.hxx - GUI launcher dialog using Qt5
//
// Written by James Turner, started October 2015.
//
// Copyright (C) 2015 James Turner <zakalawe@mac.com>
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

#include <QMainWindow>
#include <QScopedPointer>
#include <QStringList>
#include <QModelIndex>
#include <QTimer>
#include <QUrl>
#include <QQuickWidget>

namespace Ui
{
    class Launcher;
}

class QModelIndex;
class QQmlEngine;
class LaunchConfig;
class ViewCommandLinePage;
class QQuickItem;
class LauncherController;

class LauncherMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    LauncherMainWindow();
    virtual ~LauncherMainWindow();

    bool execInApp();

    bool wasRejected();


protected:
    virtual void closeEvent(QCloseEvent *event) override;

private slots:
    // run is used when the launcher is invoked before the main app is
    // started
    void onRun();

    // apply is used in-app, where we must set properties and trigger
    // a reset; setting command line options won't help us.
    void onApply();

    void onQuit();

    void onSubsytemIdleTimeout();


    void onRestoreDefaults();
    void onViewCommandLine();

    void onClickToolboxButton();


    void onQuickStatusChanged(QQuickWidget::Status status);

    void onCanFlyChanged();

    void onChangeDataDir();

private:

    LauncherController* m_controller;
    QScopedPointer<Ui::Launcher> m_ui;
    QTimer* m_subsystemIdleTimer;
    bool m_inAppMode = false;
    bool m_runInApp = false;
    bool m_accepted = false;

    ViewCommandLinePage* m_viewCommandLinePage = nullptr;

    void restoreSettings();
    void saveSettings();
};

#endif // of LAUNCHER_MAIN_WINDOW_HXX
