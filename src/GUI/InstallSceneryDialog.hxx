// InstallSceneryDialog.hxx - part of GUI launcher using Qt5
//
// Written by James Turner, started June 2016.
//
// Copyright (C) 2016 James Turner <zakalawe@mac.com>
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

#ifndef FG_GUI_INSTALL_SCENERY_DIALOG_HXX
#define FG_GUI_INSTALL_SCENERY_DIALOG_HXX

#include <QDialog>
#include <QUrl>

namespace Ui {
    class InstallSceneryDialog;
}

class InstallSceneryThread;

class InstallSceneryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InstallSceneryDialog(QWidget *parent, QString downloadDir);
    ~InstallSceneryDialog();

    QString sceneryPath();
private slots:
    virtual void reject() override;
    virtual void accept() override;

private:
    void updateUi();

    void pickFiles();

    void onThreadFinished();
    void onExtractError(QString file, QString msg);
    void onExtractProgress(int percent);
    void onExtractFile(QString file);

    enum State {
        STATE_START = 0, // awaiting user input on first screen
        STATE_EXTRACTING = 1, // in-progress, showing progress page
        STATE_FINISHED = 2, // catalog added ok, showing summary page
        STATE_EXTRACT_FAILED = 3
    };

    State m_state;
    QString m_downloadDir;

    Ui::InstallSceneryDialog *ui;

    QScopedPointer<InstallSceneryThread> m_thread;
};

#endif // FG_GUI_INSTALL_SCENERY_DIALOG_HXX
