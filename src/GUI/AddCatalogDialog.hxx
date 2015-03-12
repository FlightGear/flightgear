// AddCatalogDialog.hxx - part of GUI launcher using Qt5
//
// Written by James Turner, started March 2015.
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

#ifndef FG_GUI_ADDCATALOGDIALOG_HXX
#define FG_GUI_ADDCATALOGDIALOG_HXX

#include <QDialog>
#include <QUrl>

#include <simgear/package/Root.hxx>
#include <simgear/package/Catalog.hxx>

namespace Ui {
class AddCatalogDialog;
}

class AddCatalogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddCatalogDialog(QWidget *parent,
                              simgear::pkg::RootRef root);
    ~AddCatalogDialog();

    simgear::pkg::CatalogRef addedCatalog();

private slots:
    virtual void reject();
    virtual void accept();

    void onUrlTextChanged();
private:
    void startDownload();
    void updateUi();

    // callback from the catalog
    void onCatalogStatusChanged(simgear::pkg::Catalog* cat);

    enum State {
        STATE_START = 0, // awaiting user input on first screen
        STATE_DOWNLOADING = 1, // in-progress, showing progress page
        STATE_FINISHED = 2, // catalog added ok, showing summary page
        STATE_DOWNLOAD_FAILED // download checks failed for some reason

    };

    State m_state;

    Ui::AddCatalogDialog *ui;
    simgear::pkg::RootRef m_packageRoot;
    QUrl m_catalogUrl;
    simgear::pkg::CatalogRef m_result;
};

#endif // FG_GUI_ADDCATALOGDIALOG_HXX
