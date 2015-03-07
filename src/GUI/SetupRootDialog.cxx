// SetupRootDialog.cxx - part of GUI launcher using Qt5
//
// Written by James Turner, started December 2014.
//
// Copyright (C) 2014 James Turner <zakalawe@mac.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SetupRootDialog.hxx"

#include <QFileDialog>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QSettings>

#include "ui_SetupRootDialog.h"

#include <Main/globals.hxx>
#include <Main/fg_init.hxx>
#include <Include/version.h>

SetupRootDialog::SetupRootDialog(bool usedDefaultPath) :
    QDialog()
{
    m_ui.reset(new Ui::SetupRootDialog);
    m_ui->setupUi(this);

    connect(m_ui->browseButton, &QPushButton::clicked,
             this, &SetupRootDialog::onBrowse);
    connect(m_ui->downloadButton, &QPushButton::clicked,
            this, &SetupRootDialog::onDownload);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    m_promptState = usedDefaultPath ? DefaultPathCheckFailed : ExplicitPathCheckFailed;
    std::string ver = fgBasePackageVersion(globals->get_fg_root());
    if (!ver.empty()) {
        Q_ASSERT(ver != FLIGHTGEAR_VERSION); // otherwise what are we doing in here?!
        m_promptState = VersionCheckFailed;
    }

    m_ui->versionLabel->setText(tr("FlightGear version %1").arg(FLIGHTGEAR_VERSION));
    m_ui->bigIcon->setPixmap(QPixmap(":/app-icon-large"));
    updatePromptText();
}

bool SetupRootDialog::restoreUserSelectedRoot()
{
    QSettings settings;
    QString path = settings.value("fg-root").toString();
    if (validatePath(path) && validateVersion(path)) {
        qDebug() << "Restoring FG-root:" << path;
        globals->set_fg_root(path.toStdString());
        return true;
    } else {
        return false;
    }
}

bool SetupRootDialog::validatePath(QString path)
{
    // check assorted files exist in the root location, to avoid any chance of
    // selecting an incomplete base package. This is probably overkill but does
    // no harm
    QStringList files = QStringList()
        << "version"
        << "preferences.xml"
        << "Materials/base/materials-base.xml"
        << "gui/menubar.xml"
        << "Timezone/zone.tab";

    QDir d(path);
    if (!d.exists()) {
        return false;
    }

    Q_FOREACH(QString s, files) {
        if (!d.exists(s)) {
            return false;
        }
    }

    return true;
}

bool SetupRootDialog::validateVersion(QString path)
{
    std::string ver = fgBasePackageVersion(SGPath(path.toStdString()));
    return (ver == FLIGHTGEAR_VERSION);
}

SetupRootDialog::~SetupRootDialog()
{

}

void SetupRootDialog::onBrowse()
{
    m_browsedPath = QFileDialog::getExistingDirectory(this,
                                                     tr("Choose FlightGear data folder"));
    if (m_browsedPath.isEmpty()) {
        return;
    }

    if (!validatePath(m_browsedPath)) {
        m_promptState = ChoseInvalidLocation;
        updatePromptText();
        return;
    }

    if (!validateVersion(m_browsedPath)) {
        m_promptState = ChoseInvalidVersion;
        updatePromptText();
        return;
    }

    globals->set_fg_root(m_browsedPath.toStdString());

    QSettings settings;
    settings.setValue("fg-root", m_browsedPath);

    accept(); // we're done
}

void SetupRootDialog::onDownload()
{
    QUrl downloadUrl("http://download.flightgear.org/flightgear/Shared/");
    QDesktopServices::openUrl(downloadUrl);
}

void SetupRootDialog::updatePromptText()
{
    QString t;
    QString curRoot = QString::fromStdString(globals->get_fg_root());
    switch (m_promptState) {
    case DefaultPathCheckFailed:
        t = tr("This copy of FlightGear does not include the base data files. " \
               "Please select a suitable folder containing a previously download set of files.");
        break;

    case ExplicitPathCheckFailed:
        t = tr("The requested location '%1' does not appear to be a valid set of data files for FlightGear").arg(curRoot);
        break;

    case VersionCheckFailed:
    {
        QString curVer = QString::fromStdString(fgBasePackageVersion(globals->get_fg_root()));
        t = tr("Detected incompatible version of the data files: version %1 found, but this is FlightGear %2. " \
               "(At location: '%3') " \
               "Please install or select a matching set of data files.").arg(curVer).arg(QString::fromLatin1(FLIGHTGEAR_VERSION)).arg(curRoot);
        break;
    }

    case ChoseInvalidLocation:
        t = tr("The choosen location (%1) does not appear to contain FlightGear data files. Please try another location.").arg(m_browsedPath);
        break;

    case ChoseInvalidVersion:
    {
        QString curVer = QString::fromStdString(fgBasePackageVersion(m_browsedPath.toStdString()));
        t = tr("The choosen location (%1) contains files for version %2, but this is FlightGear %3. " \
               "Please update or try another location").arg(m_browsedPath).arg(curVer).arg(QString::fromLatin1(FLIGHTGEAR_VERSION));
        break;
    }
    }

    m_ui->promptText->setText(t);
}

